// $Id: GameWorldViewer.cpp 9357 2014-04-25 15:35:25Z FloSoft $
//
// Copyright (c) 2005 - 2011 Settlers Freaks (sf-team at siedler25.org)
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
#include "main.h"
#include "GameWorld.h"
#include "VideoDriverWrapper.h"
#include "glArchivItem_Map.h"
#include "noTree.h"
#include "nobUsual.h"
#include "nobMilitary.h"
#include "noBuildingSite.h"
#include "CatapultStone.h"
#include "GameClient.h"
#include "SoundManager.h"
#include "MapGeometry.h"
#include "MapConsts.h"
#include "dskGameInterface.h"
#include "FOWObjects.h"
#include "noShip.h"

#include "Settings.h"

#include "GameServer.h"

GameWorldViewer::GameWorldViewer() : scroll(false), sx(0), sy(0), view(GameWorldView(this, 0, 0, VideoDriverWrapper::inst().GetScreenWidth(), VideoDriverWrapper::inst().GetScreenHeight()))
{
    MoveTo(0, 0);
}

unsigned GameWorldViewer::GetAvailableSoldiersForAttack(const unsigned char player_attacker, const MapCoord x, const MapCoord y)
{
    // Ist das angegriffenne ein normales Gebäude?
    nobBaseMilitary* attacked_building = GetSpecObj<nobBaseMilitary>(x, y);
    if(attacked_building->GetBuildingType() >= BLD_BARRACKS && attacked_building->GetBuildingType() <= BLD_FORTRESS)
    {
        // Wird es gerade eingenommen?
        if(static_cast<nobMilitary*>(attacked_building)->IsCaptured())
            // Dann darf es nicht angegriffen werden
            return 0;
    }

    // Militärgebäude in der Nähe finden
    std::list<nobBaseMilitary*> buildings;
    LookForMilitaryBuildings(buildings, x, y, 3);

    unsigned total_count = 0;

    for(std::list<nobBaseMilitary*>::iterator it = buildings.begin(); it != buildings.end(); ++it)
    {
        // Muss ein Gebäude von uns sein und darf nur ein "normales Militärgebäude" sein (kein HQ etc.)
        if((*it)->GetPlayer() == player_attacker && (*it)->GetBuildingType() >= BLD_BARRACKS && (*it)->GetBuildingType() <= BLD_FORTRESS)
            total_count += static_cast<nobMilitary*>(*it)->GetSoldiersForAttack(x, y, player_attacker);

    }

    return total_count;
}



///////////////////////////////////////////////////////////////////////////////
/**
 *
 *
 *  @author OLiver
 */
void GameWorldViewer::MouseDown(const MouseCoords& mc)
{
    sx = mc.x;
    sy = mc.y;

    scroll = true;
}

///////////////////////////////////////////////////////////////////////////////
/**
 *
 *
 *  @author OLiver
 */
void GameWorldViewer::MouseUp()
{
    scroll = false;
}

///////////////////////////////////////////////////////////////////////////////
/**
 *
 *
 *  @author OLiver
 */
void GameWorldViewer::MouseMove(const MouseCoords& mc)
{
    // Scrollen
    if(scroll)
    {
        if(SETTINGS.interface.revert_mouse)
            MoveTo( ( sx - mc.x) * 2,  ( sy - mc.y) * 2);
        else
            MoveTo(-( sx - mc.x) * 2, -( sy - mc.y) * 2);
        VideoDriverWrapper::inst().SetMousePos(sx, sy);
    }
}

// Höhe wurde Verändert: TerrainRenderer Bescheid sagen, damit es entsprechend verändert werden kann
void GameWorldViewer::AltitudeChanged(const MapCoord x, const MapCoord y)
{
    tr.AltitudeChanged(x, y, this);
}

void GameWorldViewer::VisibilityChanged(const MapCoord x, const MapCoord y)
{
    tr.VisibilityChanged(x, y, this);
}


Visibility GameWorldViewer::GetVisibility(const MapCoord x, const MapCoord y) const
{
    /// Replaymodus und FoW aus? Dann alles sichtbar
    if(GameClient::inst().IsReplayModeOn() && GameClient::inst().IsReplayFOWDisabled())
        return VIS_VISIBLE;

    // Spieler schon tot? Dann auch alles sichtbar?
    if(GameClient::inst().GetLocalPlayer()->isDefeated())
        return VIS_VISIBLE;

    return CalcWithAllyVisiblity(x, y, GameClient::inst().GetPlayerID());
}


void GameWorldViewer::RecalcAllColors()
{
    tr.UpdateAllColors(this);
}

/// liefert sichtbare StraÃe, im FoW entsprechend die FoW-StraÃe
unsigned char GameWorldViewer::GetVisibleRoad(const MapCoord x, const MapCoord y, unsigned char dir, const Visibility visibility) const
{
    if(visibility == VIS_VISIBLE)
        // Normal die sichtbaren StraÃen zurückliefern
        return GetRoad(x, y, dir, true);
//      return GetPointRoad(x,y,dir,true);
    else if(visibility == VIS_FOW)
        // entsprechende FoW-StraÃe liefern
//      return GetPointFOWRoad(x,y,dir,GetYoungestFOWNodePlayer(Point<MapCoord>(x,y)));
        return GetNode(x, y).fow[GetYoungestFOWNodePlayer(Point<MapCoord>(x, y))].roads[dir];
    else
        // Unsichtbar -> keine StraÃe zeichnen
        return 0;
}


/// Return a ship at this position owned by the given player. Prefers ships that need instructions.
noShip* GameWorldViewer::GetShip(const MapCoord x, const MapCoord y, const unsigned char player) const
{
    noShip* ship = NULL;

    for (unsigned i = 0; i < 7; ++i)
    {
        Point<MapCoord> pa;

        if (i == 6)
        {
            pa = Point<MapCoord>(x, y);
        }
        else
        {
            pa.x = GetXA(x, y, i);
            pa.y = GetYA(x, y, i);
        }

        for(list<noBase*>::iterator it = GetFigures(pa.x, pa.y).begin(); it.valid(); ++it)
        {
            if((*it)->GetGOT() == GOT_SHIP)
            {
                noShip* tmp = static_cast<noShip*>(*it);

                if (tmp->GetPlayer() == player)
                {
                    if (((tmp->GetX() == x) && (tmp->GetY() == y)) || (tmp->GetDestinationForCurrentMove() == Point<MapCoord>(x, y)))
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
        const MapCoord x, const MapCoord y) const
{
    if(GameClient::inst().GetGGS().getSelection(ADDON_SEA_ATTACK) == 2) //deactivated by addon?
        return 0;
    std::list<GameWorldBase::PotentialSeaAttacker> attackers;
    GetAvailableSoldiersForSeaAttack(player_attacker, x, y, &attackers);
    return unsigned(attackers.size());
}

/// Get the "youngest" FOWObject of all players who share the view with the local player
const FOWObject* GameWorldViewer::GetYoungestFOWObject(const Point<MapCoord> pos) const
{
    return GetNode(pos.x, pos.y).fow[GetYoungestFOWNodePlayer(pos)].object;
}


/// Gets the youngest fow node of all visible objects of all players who are connected
/// with the local player via team view
unsigned char GameWorldViewer::GetYoungestFOWNodePlayer(const Point<MapCoord> pos) const
{
    unsigned char local_player = GameClient::inst().GetPlayerID();
    unsigned char youngest_player = local_player;
    unsigned youngest_time = GetNode(pos.x, pos.y).fow[local_player].last_update_time;

    // Shared team view enabled?
    if(GameClient::inst().GetGGS().team_view)
    {
        // Then check if team members have a better (="younger", see our economy) fow object
        for(unsigned i = 0; i < GameClient::inst().GetPlayerCount(); ++i)
        {
            if(GameClient::inst().GetPlayer(i)->IsAlly(local_player))
            {
                // Has the player FOW at this point at all?
                if(GetNode(pos.x, pos.y).fow[i].visibility == VIS_FOW)
                {
                    // Younger than the youngest or no object at all?
                    if(GetNode(pos.x, pos.y).fow[i].last_update_time > youngest_time)
                    {
                        // Then take it
                        youngest_time = GetNode(pos.x, pos.y).fow[i].last_update_time;
                        // And remember its owner
                        youngest_player = i;
                    }
                }
            }
        }
    }

    return youngest_player;
}

