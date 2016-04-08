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

///////////////////////////////////////////////////////////////////////////////
// Header
#include "defines.h" // IWYU pragma: keep
#include "world/GameWorldViewer.h"
#include "drivers/VideoDriverWrapper.h"
#include "buildings/nobMilitary.h"
#include "GameClient.h"
#include "gameTypes/MapTypes.h"
#include "nodeObjs/noShip.h"

// Include last!
#include "DebugNew.h" // IWYU pragma: keep
class FOWObject;

GameWorldViewer::GameWorldViewer()
{
}

unsigned GameWorldViewer::GetAvailableSoldiersForAttack(const unsigned char player_attacker, const MapPoint pt)
{
    // Ist das angegriffenne ein normales Gebäude?
    nobBaseMilitary* attacked_building = GetSpecObj<nobBaseMilitary>(pt);
    if(attacked_building->GetBuildingType() >= BLD_BARRACKS && attacked_building->GetBuildingType() <= BLD_FORTRESS)
    {
        // Wird es gerade eingenommen?
        if(static_cast<nobMilitary*>(attacked_building)->IsCaptured())
            // Dann darf es nicht angegriffen werden
            return 0;
    }

    // Militärgebäude in der Nähe finden
    unsigned total_count = 0;

    sortedMilitaryBlds buildings = LookForMilitaryBuildings(pt, 3);
    for(sortedMilitaryBlds::iterator it = buildings.begin(); it != buildings.end(); ++it)
    {
        // Muss ein Gebäude von uns sein und darf nur ein "normales Militärgebäude" sein (kein HQ etc.)
        if((*it)->GetPlayer() == player_attacker && (*it)->GetBuildingType() >= BLD_BARRACKS && (*it)->GetBuildingType() <= BLD_FORTRESS)
            total_count += static_cast<nobMilitary*>(*it)->GetNumSoldiersForAttack(pt, player_attacker);
    }

    return total_count;
}

// Höhe wurde Verändert: TerrainRenderer Bescheid sagen, damit es entsprechend verändert werden kann
void GameWorldViewer::AltitudeChanged(const MapPoint pt)
{
    GameWorldBase::AltitudeChanged(pt);
    tr.AltitudeChanged(pt, *this);
}

void GameWorldViewer::VisibilityChanged(const MapPoint pt)
{
    tr.VisibilityChanged(pt, *this);
}


Visibility GameWorldViewer::GetVisibility(const MapPoint pt) const
{
    /// Replaymodus und FoW aus? Dann alles sichtbar
    if(GAMECLIENT.IsReplayModeOn() && GAMECLIENT.IsReplayFOWDisabled())
        return VIS_VISIBLE;

    // Spieler schon tot? Dann auch alles sichtbar?
    if(GAMECLIENT.GetLocalPlayer().isDefeated())
        return VIS_VISIBLE;

    return CalcWithAllyVisiblity(pt, GAMECLIENT.GetPlayerID());
}


void GameWorldViewer::RecalcAllColors()
{
    tr.UpdateAllColors(*this);
}

/// liefert sichtbare Straße, im FoW entsprechend die FoW-Straße
unsigned char GameWorldViewer::GetVisibleRoad(const MapPoint pt, unsigned char dir, const Visibility visibility) const
{
    if(visibility == VIS_VISIBLE)
        // Normal die sichtbaren Straßen zurückliefern
        return GetRoad(pt, dir, true);
//      return GetPointRoad(x,y,dir,true);
    else if(visibility == VIS_FOW)
        // entsprechende FoW-Straße liefern
//      return GetPointFOWRoad(x,y,dir,GetYoungestFOWNodePlayer(MapPoint(x,y)));
        return GetNode(pt).fow[GetYoungestFOWNodePlayer(pt)].roads[dir];
    else
        // Unsichtbar -> keine Straße zeichnen
        return 0;
}


/// Return a ship at this position owned by the given player. Prefers ships that need instructions.
noShip* GameWorldViewer::GetShip(const MapPoint pt, const unsigned char player) const
{
    noShip* ship = NULL;

    for (unsigned i = 0; i < 7; ++i)
    {
        MapPoint pa;

        if (i == 6)
        {
            pa = pt;
        }
        else
        {
            pa = GetNeighbour(pt, i);
        }

        const std::list<noBase*>& figures = GetFigures(pa);
        for(std::list<noBase*>::const_iterator it = figures.begin(); it != figures.end(); ++it)
        {
            if((*it)->GetGOT() == GOT_SHIP)
            {
                noShip* tmp = static_cast<noShip*>(*it);

                if (tmp->GetPlayer() == player)
                {
                    if ((tmp->GetPos() == pt) || (tmp->GetDestinationForCurrentMove() == pt))
                    {
                        if (tmp->IsWaitingForExpeditionInstructions())
                        {
                            return(tmp);
                        }

                        ship = tmp;
                    }
                }
            }
        }
    }

    return(ship);
}

/// Gibt die verfügbar Anzahl der Angreifer für einen Seeangriff zurück
unsigned GameWorldViewer::GetAvailableSoldiersForSeaAttackCount(const unsigned char player_attacker,
        const MapPoint pt) const
{
    if(GAMECLIENT.GetGGS().getSelection(AddonId::SEA_ATTACK) == 2) //deactivated by addon?
        return 0;
    return unsigned(GetAvailableSoldiersForSeaAttack(player_attacker, pt).size());
}

/// Get the "youngest" FOWObject of all players who share the view with the local player
const FOWObject* GameWorldViewer::GetYoungestFOWObject(const MapPoint pos) const
{
    return GetNode(pos).fow[GetYoungestFOWNodePlayer(pos)].object;
}


/// Gets the youngest fow node of all visible objects of all players who are connected
/// with the local player via team view
unsigned char GameWorldViewer::GetYoungestFOWNodePlayer(const MapPoint pos) const
{
    unsigned char local_player = GAMECLIENT.GetPlayerID();
    unsigned char youngest_player = local_player;
    unsigned youngest_time = GetNode(pos).fow[local_player].last_update_time;

    // Shared team view enabled?
    if(GAMECLIENT.GetGGS().team_view)
    {
        // Then check if team members have a better (="younger", see our economy) fow object
        for(unsigned i = 0; i < GAMECLIENT.GetPlayerCount(); ++i)
        {
            if(!GAMECLIENT.GetPlayer(i).IsAlly(local_player))
                continue;
            // Has the player FOW at this point at all?
            const MapNode::FoWData& name = GetNode(pos).fow[i];
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

