// Copyright (c) 2005 - 2015 Settlers Freaks (sf-team at siedler25.org)
//
// This file is part of Return To The Roots.
//
// Return To The Roots is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 2 of the License, or
// (at your option) any later version.
//
// Return To The Roots is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with Return To The Roots. If not, see <http://www.gnu.org/licenses/>.

#include "defines.h" // IWYU pragma: keep
#include "world/GameWorldViewer.h"
#include "world/GameWorldBase.h"
#include "drivers/VideoDriverWrapper.h"
#include "buildings/nobMilitary.h"
#include "GameClient.h"
#include "gameTypes/MapTypes.h"
#include "nodeObjs/noShip.h"
#include "notifications/NodeNote.h"
#include "notifications/PlayerNodeNote.h"
#include <boost/lambda/lambda.hpp>
#include <boost/lambda/if.hpp>
#include <boost/lambda/bind.hpp>

GameWorldViewer::GameWorldViewer(unsigned player, GameWorldBase& gwb): player_(player), gwb(gwb)
{
    tr.GenerateOpenGL(*this);
    SubscribeToEvents();
}

void GameWorldViewer::SubscribeToEvents()
{
    namespace bl = boost::lambda;
    using bl::_1;
    // Notify renderer about altitude changes
    evAltitudeChanged = gwb.GetNotifications().subscribe<NodeNote>(
        bl::if_(bl::bind(&NodeNote::type, _1) == NodeNote::Altitude)
        [bl::bind(&TerrainRenderer::AltitudeChanged, &tr, bl::bind(&NodeNote::pt, _1), boost::cref(*this))]
    );
    // And visibility changes
    evVisibilityChanged = gwb.GetNotifications().subscribe<PlayerNodeNote>(
        bl::if_(bl::bind(&PlayerNodeNote::type, _1) == PlayerNodeNote::Visibility)
        [bl::bind(&GameWorldViewer::VisibilityChanged, this, bl::bind(&PlayerNodeNote::pt, _1), bl::bind(&PlayerNodeNote::player, _1))]
    );
}

const GameClientPlayer& GameWorldViewer::GetPlayer() const
{
    return GetWorld().GetPlayer(player_);
}

unsigned GameWorldViewer::GetAvailableSoldiersForAttack(const MapPoint pt) const
{
    // Ist das angegriffenne ein normales Gebäude?
    const nobBaseMilitary* attacked_building = GetWorld().GetSpecObj<nobBaseMilitary>(pt);
    if(attacked_building->GetBuildingType() >= BLD_BARRACKS && attacked_building->GetBuildingType() <= BLD_FORTRESS)
    {
        // Wird es gerade eingenommen?
        if(static_cast<const nobMilitary*>(attacked_building)->IsCaptured())
            // Dann darf es nicht angegriffen werden
            return 0;
    }

    // Militärgebäude in der Nähe finden
    unsigned total_count = 0;

    sortedMilitaryBlds buildings = GetWorld().LookForMilitaryBuildings(pt, 3);
    for(sortedMilitaryBlds::iterator it = buildings.begin(); it != buildings.end(); ++it)
    {
        // Muss ein Gebäude von uns sein und darf nur ein "normales Militärgebäude" sein (kein HQ etc.)
        if((*it)->GetPlayer() == player_ && (*it)->GetBuildingType() >= BLD_BARRACKS && (*it)->GetBuildingType() <= BLD_FORTRESS)
            total_count += static_cast<nobMilitary*>(*it)->GetNumSoldiersForAttack(pt, player_);
    }

    return total_count;
}

BuildingQuality GameWorldViewer::GetBQ(const MapPoint& pt) const
{
    return GetWorld().GetBQ(pt, player_);
}

Visibility GameWorldViewer::GetVisibility(const MapPoint pt) const
{
    /// Replaymodus und FoW aus? Dann alles sichtbar
    if(GAMECLIENT.IsReplayModeOn() && GAMECLIENT.IsReplayFOWDisabled())
        return VIS_VISIBLE;

    // Spieler schon tot? Dann auch alles sichtbar?
    if(GetPlayer().isDefeated())
        return VIS_VISIBLE;

    return GetWorld().CalcWithAllyVisiblity(pt, player_);
}

bool GameWorldViewer::IsOwner(const MapPoint& pt) const
{
    return GetWorld().GetNode(pt).owner == player_ + 1;
}

const MapNode& GameWorldViewer::GetNode(const MapPoint& pt) const
{
    return GetWorld().GetNode(pt);
}

MapPoint GameWorldViewer::GetNeighbour(const MapPoint pt, const Direction dir) const
{
    return GetWorld().GetNeighbour(pt, dir);
}

void GameWorldViewer::RecalcAllColors()
{
    tr.UpdateAllColors(*this);
}

/// liefert sichtbare Straße, im FoW entsprechend die FoW-Straße
unsigned char GameWorldViewer::GetVisibleRoad(const MapPoint pt, unsigned char dir, const Visibility visibility) const
{
    if(visibility == VIS_VISIBLE)
        return GetWorld().GetRoad(pt, dir, true);
    else if(visibility == VIS_FOW)
        return GetWorld().GetNode(pt).fow[GetYoungestFOWNodePlayer(pt)].roads[dir];
    else
        return 0; // No road
}

/// Return a ship at this position owned by the given player. Prefers ships that need instructions.
noShip* GameWorldViewer::GetShip(const MapPoint pt) const
{
    noShip* ship = NULL;

    for (unsigned i = 0; i < 7; ++i)
    {
        MapPoint curPt;

        if (i == 6)
            curPt = pt;
        else
            curPt = GetWorld().GetNeighbour(pt, i);

        const std::list<noBase*>& figures = GetWorld().GetFigures(curPt);
        for(std::list<noBase*>::const_iterator it = figures.begin(); it != figures.end(); ++it)
        {
            if((*it)->GetGOT() != GOT_SHIP)
                continue;
            noShip* tmp = static_cast<noShip*>(*it);

            if (tmp->GetPlayer() == player_ && tmp->GetPos() == pt || tmp->GetDestinationForCurrentMove() == pt)
            {
                if(tmp->IsWaitingForExpeditionInstructions())
                    return tmp;
                ship = tmp;
            }
        }
    }

    return ship;
}

/// Gibt die verfügbar Anzahl der Angreifer für einen Seeangriff zurück
unsigned GameWorldViewer::GetAvailableSoldiersForSeaAttackCount(const MapPoint pt) const
{
    if(GetWorld().GetGGS().getSelection(AddonId::SEA_ATTACK) == 2) //deactivated by addon?
        return 0;
    return unsigned(GetWorld().GetAvailableSoldiersForSeaAttack(player_, pt).size());
}

void GameWorldViewer::ChangePlayer(unsigned player)
{
    if(player == player_)
        return;
    player_ = player;
    RecalcAllColors();
}

void GameWorldViewer::VisibilityChanged(const MapPoint& pt, unsigned player)
{
    // If visibility changed for us, or our team mate if shared view is on -> Update renderer
    if(player == player_ || (GetWorld().GetGGS().team_view && GetWorld().GetPlayer(player_).IsAlly(player)))
        tr.VisibilityChanged(pt, *this);
}

/// Get the "youngest" FOWObject of all players who share the view with the local player
const FOWObject* GameWorldViewer::GetYoungestFOWObject(const MapPoint pos) const
{
    return GetWorld().GetNode(pos).fow[GetYoungestFOWNodePlayer(pos)].object;
}

/// Gets the youngest fow node of all visible objects of all players who are connected
/// with the local player via team view
unsigned char GameWorldViewer::GetYoungestFOWNodePlayer(const MapPoint pos) const
{
    unsigned char youngest_player = player_;
    unsigned youngest_time = GetWorld().GetNode(pos).fow[player_].last_update_time;

    // Shared team view enabled?
    if(GetWorld().GetGGS().team_view)
    {
        // Then check if team members have a better (="younger", see our economy) fow object
        for(unsigned i = 0; i <  GetWorld().GetPlayerCount(); ++i)
        {
            if(!GetWorld().GetPlayer(i).IsAlly(player_))
                continue;
            // Has the player FOW at this point at all?
            const MapNode::FoWData& name = GetWorld().GetNode(pos).fow[i];
            if(name.visibility == VIS_FOW)
            {
                // Younger than the youngest or no object at all?
                if(name.last_update_time > youngest_time)
                {
                    // Then take it
                    youngest_time = name.last_update_time;
                    // And remember its owner
                    youngest_player = i;
                }
            }
        }
    }

    return youngest_player;
}
