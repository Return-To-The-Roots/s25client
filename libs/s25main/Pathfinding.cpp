// Copyright (c) 2005 - 2017 Settlers Freaks (sf-team at siedler25.org)
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

#include "GamePlayer.h"
#include "helpers/EnumRange.h"
#include "pathfinding/FreePathFinder.h"
#include "pathfinding/FreePathFinderImpl.h"
#include "pathfinding/PathConditionHuman.h"
#include "pathfinding/PathConditionShip.h"
#include "pathfinding/PathConditionTrade.h"
#include "pathfinding/RoadPathFinder.h"
#include "world/GameWorld.h"
#include "gameTypes/ShipDirection.h"
#include "gameData/GameConsts.h"

/// Findet einen Weg für Figuren
helpers::OptionalEnum<Direction> GameWorldBase::FindHumanPath(const MapPoint start, const MapPoint dest,
                                                              const unsigned max_route, const bool random_route,
                                                              unsigned* length, std::vector<Direction>* route) const
{
    Direction first_dir{};
    if(GetFreePathFinder().FindPath(start, dest, random_route, max_route, route, length, &first_dir,
                                    PathConditionHuman(*this)))
        return first_dir;
    else
        return boost::none;
}

/// Wegfindung für Menschen im Straßennetz
RoadPathDirection GameWorld::FindHumanPathOnRoads(const noRoadNode& start, const noRoadNode& goal, unsigned* length,
                                                  MapPoint* firstPt, const RoadSegment* const forbidden)
{
    RoadPathDirection first_dir;
    if(GetRoadPathFinder().FindPath(start, goal, false, std::numeric_limits<unsigned>::max(), forbidden, length,
                                    &first_dir, firstPt))
        return first_dir;
    else
        return RoadPathDirection::None;
}

/// Wegfindung für Waren im Straßennetz
RoadPathDirection GameWorld::FindPathForWareOnRoads(const noRoadNode& start, const noRoadNode& goal, unsigned* length,
                                                    MapPoint* firstPt, unsigned max)
{
    RoadPathDirection first_dir;
    if(GetRoadPathFinder().FindPath(start, goal, true, max, nullptr, length, &first_dir, firstPt))
        return first_dir;
    else
        return RoadPathDirection::None;
}

bool GameWorldBase::FindShipPathToHarbor(const MapPoint start, unsigned harborId, unsigned seaId,
                                         std::vector<Direction>* route, unsigned* length)
{
    // Find the distance to the furthest harbor from the target harbor and take that as maximum
    unsigned maxDistance = 0;

    for(const auto dir : helpers::EnumRange<ShipDirection>{})
    {
        const std::vector<HarborPos::Neighbor>& neighbors = GetHarborNeighbors(harborId, dir);
        for(const HarborPos::Neighbor& neighbor : neighbors)
        {
            if(IsHarborAtSea(neighbor.id, seaId) && neighbor.distance > maxDistance)
                maxDistance = neighbor.distance;
        }
    }
    // Add a few fields reserve
    maxDistance += 6;
    return FindShipPath(start, GetCoastalPoint(harborId, seaId), maxDistance, route, length);
}

bool GameWorldBase::FindShipPath(const MapPoint start, const MapPoint dest, unsigned maxDistance,
                                 std::vector<Direction>* route, unsigned* length)
{
    return GetFreePathFinder().FindPath(start, dest, true, maxDistance, route, length, nullptr,
                                        PathConditionShip(*this));
}

/// Prüft, ob eine Schiffsroute noch Gültigkeit hat
bool GameWorld::CheckShipRoute(const MapPoint start, const std::vector<Direction>& route, const unsigned pos,
                               MapPoint* dest)
{
    return GetFreePathFinder().CheckRoute(start, route, pos, PathConditionShip(*this), dest);
}

/// Find a route for trade caravanes
helpers::OptionalEnum<Direction> GameWorld::FindTradePath(const MapPoint start, const MapPoint dest,
                                                          unsigned char player, unsigned max_route, bool random_route,
                                                          std::vector<Direction>* route, unsigned* length) const
{
    unsigned char owner = GetNode(dest).owner;
    if(owner != 0 && !GetPlayer(player).IsAlly(owner - 1))
        return boost::none;

    RTTR_Assert(GetNO(dest)->GetType() == NodalObjectType::Flag); // Goal should be the flag of a wh

    if(!PathConditionHuman(*this).IsNodeOk(dest))
        return boost::none;

    Direction first_dir{};
    if(GetFreePathFinder().FindPath(start, dest, random_route, max_route, route, length, &first_dir,
                                    PathConditionTrade(*this, player)))
        return first_dir;
    else
        return boost::none;
}

bool GameWorld::CheckTradeRoute(const MapPoint start, const std::vector<Direction>& route, unsigned pos,
                                unsigned char player, MapPoint* dest) const
{
    return GetFreePathFinder().CheckRoute(start, route, pos, PathConditionTrade(*this, player), dest);
}
