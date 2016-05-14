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
#include "GamePlayer.h"
#include "world/GameWorldGame.h"
#include "pathfinding/RoadPathFinder.h"
#include "pathfinding/FreePathFinder.h"
#include "pathfinding/FreePathFinderImpl.h"
#include "pathfinding/PathConditions.h"
#include "gameData/GameConsts.h"

/// Param for road-build pathfinding
struct Param_RoadPath
{
    /// Boat or normal road
    bool boat_road;
};

bool IsPointOK_RoadPath(const GameWorldBase& gwb, const MapPoint pt, const unsigned char  /*dir*/, const void* param)
{
    const Param_RoadPath* prp = static_cast<const Param_RoadPath*>(param);

    // Auch auf unserem Territorium?
    if(!gwb.IsPlayerTerritory(pt))
        return false;

    // Feld bebaubar?
    if(!gwb.RoadAvailable(prp->boat_road, pt))
        return false;

    return true;
}

/// Condition for comfort road construction with a possible flag every 2 steps
bool IsPointOK_RoadPathEvenStep(const GameWorldBase& gwb, const MapPoint pt, const unsigned char  /*dir*/, const void* param)
{
    const Param_RoadPath* prp = static_cast<const Param_RoadPath*>(param);

    // Auch auf unserem Territorium?
    if(!gwb.IsPlayerTerritory(pt))
        return false;

    // Feld bebaubar?
    if(!gwb.RoadAvailable(prp->boat_road, pt))
        return false;
    if(!prp->boat_road && gwb.GetBQ(pt, gwb.GetNode(pt).owner - 1) == BQ_NOTHING)
        return false;

    return true;
}

/// Findet einen Weg für Figuren
unsigned char GameWorldBase::FindHumanPath(const MapPoint start, const MapPoint dest, const unsigned max_route, const bool random_route, unsigned* length) const
{
    unsigned char first_dir = INVALID_DIR;
    GetFreePathFinder().FindPath(start, dest, random_route, max_route, NULL, length, &first_dir, PathConditionHuman(*this));
    return first_dir;
}

/// Wegfindung für Menschen im Straßennetz
unsigned char GameWorldGame::FindHumanPathOnRoads(const noRoadNode& start, const noRoadNode& goal, unsigned* length, MapPoint* firstPt, const RoadSegment* const forbidden)
{
    unsigned char first_dir = INVALID_DIR;
    if(GetRoadPathFinder().FindPath(start, goal, false, std::numeric_limits<unsigned>::max(), forbidden, length, &first_dir, firstPt))
        return first_dir;
    else
        return INVALID_DIR;
}

/// Wegfindung für Waren im Straßennetz
unsigned char GameWorldGame::FindPathForWareOnRoads(const noRoadNode& start, const noRoadNode& goal, unsigned* length, MapPoint* firstPt, unsigned max)
{
    unsigned char first_dir = INVALID_DIR;
    if(GetRoadPathFinder().FindPath(start, goal, true, max, NULL, length, &first_dir, firstPt))
        return first_dir;
    else
        return INVALID_DIR;
}

/// Wegfindung für Schiffe auf dem Wasser
bool GameWorldBase::FindShipPath(const MapPoint start, const MapPoint dest, std::vector<unsigned char>* route, unsigned* length)
{
    return GetFreePathFinder().FindPath(start, dest, true, 400, route, length, NULL, PathConditionShip(*this));
}

/// Prüft, ob eine Schiffsroute noch Gültigkeit hat
bool GameWorldGame::CheckShipRoute(const MapPoint start, const std::vector<unsigned char>& route, const unsigned pos, MapPoint* dest)
{
    return GetFreePathFinder().CheckRoute(start, route, pos, PathConditionShip(*this), dest);
}

/// Find a route for trade caravanes
unsigned char GameWorldGame::FindTradePath(const MapPoint start, const MapPoint dest, unsigned char player, unsigned max_route, bool random_route, 
        std::vector<unsigned char>* route, unsigned* length) const
{
    unsigned char owner = GetNode(dest).owner;
    if(owner != 0 && !GetPlayer(player).IsAlly(owner -1))
        return INVALID_DIR;
    
    RTTR_Assert(GetNO(dest)->GetType() == NOP_FLAG); // Goal should be the flag of a wh

    if(!IsNodeForFigures(dest))
        return INVALID_DIR;

    unsigned char first_dir = INVALID_DIR;
    GetFreePathFinder().FindPath(start, dest, random_route, max_route, route, length, &first_dir, PathConditionTrade(*this, player));

    return first_dir;
}

bool GameWorldGame::CheckTradeRoute(const MapPoint start, const std::vector<unsigned char>& route, unsigned pos, unsigned char player, MapPoint* dest) const
{
    return GetFreePathFinder().CheckRoute(start, route, pos, PathConditionTrade(*this, player), dest);
}
