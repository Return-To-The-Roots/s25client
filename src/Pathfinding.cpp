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
#include "defines.h"
#include "Node.h"

#include "nodeObjs/noRoadNode.h"
#include "drivers/VideoDriverWrapper.h"
#include "Random.h"
#include "MapGeometry.h"
#include "buildings/nobHarborBuilding.h"
#include "GameClient.h"
#include "gameData/TerrainData.h"
#include "gameData/GameConsts.h"
#include "pathfinding/RoadPathFinder.h"
#include "pathfinding/FreePathFinder.h"

#include <set>
#include <vector>
#include <limits>

#include <iostream>

///////////////////////////////////////////////////////////////////////////////
// Makros / Defines
#if defined _WIN32 && defined _DEBUG && defined _MSC_VER
#define new new(_NORMAL_BLOCK, THIS_FILE, __LINE__)
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/// Paremter-Struktur für Straßenbaupathfinding
struct Param_RoadPath
{
    /// Straßenbaumodus erlaubt?
    bool boat_road;
};

/// Abbruch-Bedingungen für Straßenbau-Pathfinding
bool IsPointOK_RoadPath(const GameWorldBase& gwb, const MapPoint pt, const unsigned char dir, const void* param)
{
    const Param_RoadPath* prp = static_cast<const Param_RoadPath*>(param);

    // Auch auf unserem Territorium?
    if(!gwb.IsPlayerTerritory(pt))
        return false;

    // Feld bebaubar?
    if(!gwb.RoadAvailable(prp->boat_road, pt, dir))
        return false;

    return true;
}

/// Abbruch-Bedingungen für Straßenbau-Pathfinding for comfort road construction with a possible flag every 2 steps
bool IsPointOK_RoadPathEvenStep(const GameWorldBase& gwb, const MapPoint pt, const unsigned char dir, const void* param)
{
    const Param_RoadPath* prp = static_cast<const Param_RoadPath*>(param);

    // Auch auf unserem Territorium?
    if(!gwb.IsPlayerTerritory(pt))
        return false;

    // Feld bebaubar?
    if(!gwb.RoadAvailable(prp->boat_road, pt, dir))
        return false;
	if(!prp->boat_road && (gwb.CalcBQ(pt, gwb.GetNode(pt).owner-1, true, false) != BQ_FLAG))
		return false;	

    return true;
}

/// Straßenbau-Pathfinding
bool GameWorldViewer::FindRoadPath(const MapPoint start, const MapPoint dest, std::vector<unsigned char>& route, const bool boat_road)
{
    Param_RoadPath prp = { boat_road };
    return GetFreePathFinder().FindPath(start, dest, false, 100, &route, NULL, NULL, IsPointOK_RoadPath, NULL, &prp, false);
}

/// Abbruch-Bedingungen für freien Pfad für Menschen
bool IsPointOK_HumanPath(const GameWorldBase& gwb, const MapPoint pt, const unsigned char dir, const void* param)
{
    // Feld passierbar?
    noBase::BlockingManner bm = gwb.GetNO(pt)->GetBM();
    if(bm != noBase::BM_NOTBLOCKING && bm != noBase::BM_TREE && bm != noBase::BM_FLAG)
        return false;

    return true;
}



/// Zusätzliche Abbruch-Bedingungen für freien Pfad für Menschen, die auch bei der letzen Kante
/// zum Ziel eingehalten werden müssen
bool IsPointToDestOK_HumanPath(const GameWorldBase& gwb, const MapPoint pt, const unsigned char dir, const void* param)
{
    // Feld passierbar?
    // Nicht über Wasser, Lava, Sümpfe gehen
    if(!gwb.IsNodeToNodeForFigure(pt, (dir + 3) % 6))
        return false;

    return true;
}

/// Abbruch-Bedingungen für freien Pfad für Schiffe
bool IsPointOK_ShipPath(const GameWorldBase& gwb, const MapPoint pt, const unsigned char dir, const void* param)
{
    // Ein Meeresfeld?
    for(unsigned i = 0; i < 6; ++i)
    {
        if(!TerrainData::IsUsableByShip(gwb.GetTerrainAround(pt, i)))
            return false;
    }

    return true;
}

/// Zusätzliche Abbruch-Bedingungen für freien Pfad für Schiffe, die auch bei der letzen Kante
/// zum Ziel eingehalten werden müssen
bool IsPointToDestOK_ShipPath(const GameWorldBase& gwb, const MapPoint pt, const unsigned char dir, const void* param)
{
    // Der Übergang muss immer aus Wasser sein zu beiden Seiten
    if(TerrainData::IsUsableByShip(gwb.GetWalkingTerrain1(pt, (dir + 3) % 6)) && TerrainData::IsUsableByShip(gwb.GetWalkingTerrain2(pt, (dir + 3) % 6)))
        return true;
    else
        return false;
}

/// Findet einen Weg für Figuren
unsigned char GameWorldBase::FindHumanPath(const MapPoint start, 
        const MapPoint dest, const unsigned max_route, const bool random_route, unsigned* length, const bool record) const
{
    // Aus Replay lesen?
    if(GAMECLIENT.ArePathfindingResultsAvailable() && !random_route)
    {
        unsigned char dir;
        if(GAMECLIENT.ReadPathfindingResult(&dir, length, NULL))
            return dir;
    }

    unsigned char first_dir = INVALID_DIR;
    GetFreePathFinder().FindPath(start, dest, random_route, max_route, NULL, length, &first_dir, IsPointOK_HumanPath, 
                 IsPointToDestOK_HumanPath, NULL, record);

    if(!random_route)
        GAMECLIENT.AddPathfindingResult(first_dir, length, NULL);

    return first_dir;

}

/// Wegfindung für Menschen im Straßennetz
unsigned char GameWorldGame::FindHumanPathOnRoads(const noRoadNode& start, const noRoadNode& goal, unsigned* length, MapPoint* firstPt, const RoadSegment* const forbidden)
{
    unsigned char first_dir = INVALID_DIR;
    if(GetRoadPathFinder().FindPath(start, goal, true, false, std::numeric_limits<unsigned>::max(), forbidden, length, &first_dir, firstPt))
        return first_dir;
    else
        return 0xFF;
}

/// Wegfindung für Waren im Straßennetz
unsigned char GameWorldGame::FindPathForWareOnRoads(const noRoadNode& start, const noRoadNode& goal, unsigned* length, MapPoint* firstPt, unsigned max)
{
    unsigned char first_dir = INVALID_DIR;
    if(GetRoadPathFinder().FindPath(start, goal, true, true, max, NULL, length, &first_dir, firstPt))
        return first_dir;
    else
        return 0xFF;
}


/// Wegfindung für Schiffe auf dem Wasser
bool GameWorldBase::FindShipPath(const MapPoint start, const MapPoint dest, std::vector<unsigned char>* route, unsigned* length, const unsigned max_length, 
                                 GameWorldBase::CrossBorders* cb)
{
    return GetFreePathFinder().FindPath(start, dest, true, 400, route, length, NULL, IsPointOK_ShipPath, IsPointToDestOK_ShipPath, NULL, false);
}

/// Prüft, ob eine Schiffsroute noch Gültigkeit hat
bool GameWorldGame::CheckShipRoute(const MapPoint start, const std::vector<unsigned char>& route, const unsigned pos, MapPoint* dest)
{
    return GetFreePathFinder().CheckRoute(start, route, pos, IsPointOK_ShipPath, IsPointToDestOK_ShipPath, dest, NULL);
}


/// Abbruch-Bedingungen für freien Pfad für Menschen
bool IsPointOK_TradePath(const GameWorldBase& gwb, const MapPoint pt, const unsigned char dir, const void* param)
{
    // Feld passierbar?
    noBase::BlockingManner bm = gwb.GetNO(pt)->GetBM();
    if(bm != noBase::BM_NOTBLOCKING && bm != noBase::BM_TREE && bm != noBase::BM_FLAG)
        return false;


    unsigned char player = gwb.GetNode(pt).owner;
    // Ally or no player? Then ok
    if(player == 0 || gwb.GetPlayer(*((unsigned char*)param)).IsAlly(player - 1))
        return true;
    else
        return false;
}

bool IsPointToDestOK_TradePath(const GameWorldBase& gwb, const MapPoint pt, const unsigned char dir, const void* param)
{
    // Feld passierbar?
    // Nicht über Wasser, Lava, Sümpfe gehen
    if(!gwb.IsNodeToNodeForFigure(pt, (dir + 3) % 6))
        return false;

    // Not trough hostile territory?
    unsigned char old_player = gwb.GetNode(gwb.GetNeighbour(pt, (dir + 3) % 6)).owner, 
                  new_player = gwb.GetNode(pt).owner;
    // Ally or no player? Then ok
    if(new_player == 0 || gwb.GetPlayer(*((unsigned char*)param)).IsAlly(new_player - 1))
        return true;
    else
    {
        // Old player also evil?
        if(old_player != 0 && !gwb.GetPlayer(*((unsigned char*)param)).IsAlly(old_player - 1))
            return true;
        else
            return false;
    }
}


/// Find a route for trade caravanes
unsigned char GameWorldGame::FindTradePath(const MapPoint start, 
        const MapPoint dest, const unsigned char player, const unsigned max_route, const bool random_route, 
        std::vector<unsigned char>* route, unsigned* length, 
        const bool record) const
{
    //unsigned tt = GetTickCount();
    //static unsigned cc = 0;
    //++cc;

    unsigned char pp = GetNode(dest).owner;
    if(!(pp == 0 || GetPlayer(player).IsAlly(pp - 1)))
        return 0xff;
    bool is_warehouse_at_goal = false;
    if(GetNO(dest)->GetType() == NOP_BUILDING)
    {
        if(GetSpecObj<noBuilding>(dest)->IsWarehouse())
            is_warehouse_at_goal = true;
    }

    if(!IsNodeForFigures(dest) && !is_warehouse_at_goal )
        return 0xff;

    unsigned char first_dir = INVALID_DIR;
    GetFreePathFinder().FindPath(start, dest, random_route, max_route, route, length, &first_dir, IsPointOK_TradePath, 
                 IsPointToDestOK_TradePath, &player, record);

    //if(GetTickCount()-tt > 100)
    //  printf("%u: %u ms; (%u, %u) to (%u, %u)\n", cc, GetTickCount()-tt, start.x, start.y, dest.x, dest.y);

    return first_dir;
}

/// Check whether trade path is still valid
bool GameWorldGame::CheckTradeRoute(const MapPoint start, const std::vector<unsigned char>& route, 
                                    const unsigned pos, const unsigned char player, 
                                    MapPoint* dest) const
{
    return GetFreePathFinder().CheckRoute(start, route, pos, IsPointOK_TradePath, IsPointToDestOK_HumanPath, dest, &player);
}
