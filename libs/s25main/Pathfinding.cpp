// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "GamePlayer.h"
#include "helpers/EnumRange.h"
#include "helpers/IdRange.h"
#include "pathfinding/FreePathFinder.h"
#include "pathfinding/FreePathFinderImpl.h"
#include "pathfinding/PathConditionHuman.h"
#include "pathfinding/PathConditionShip.h"
#include "pathfinding/PathConditionTrade.h"
#include "pathfinding/RoadPathFinder.h"
#include "pathfinding/ShipPathData.h"
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
        return std::nullopt;
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

bool GameWorldBase::FindShipPathToHarbor(const MapPoint start, HarborId harborId, SeaId seaId,
                                         std::vector<Direction>* route, unsigned* length) const
{
    const MapPoint coastalPoint = GetCoastalPoint(harborId, seaId);
    RTTR_Assert(coastalPoint.isValid());

    // already arrived?
    if(start == coastalPoint)
    {
        if(length)
            *length = 0;
        if(route)
            route->clear();
        return true;
    }
    auto& shipPathData = GetShipPathData();
    // If we start from a harbor we can get the route directly w/o the costly pathfinding
    for(const auto startHbId : helpers::idRange<HarborId>(GetNumHarborPoints()))
    {
        if(GetCoastalPoint(startHbId, seaId) == start)
        {
            auto hbRoute = shipPathData.getHarborConnection(startHbId, harborId, seaId);
            if(length)
                *length = hbRoute.size();
            if(route)
                *route = std::move(hbRoute);
        }
    }

    // Try cache first
    bool reversed = false;
    if(const auto* cachedPath = shipPathData.findCachedPath(start, coastalPoint, reversed))
    {
        if(route)
        {
            *route = *cachedPath;
            if(reversed)
            {
                std::reverse(route->begin(), route->end());
                for(auto& d : *route)
                    d = d + 3u; // reverse direction
            }
        }
        if(length)
            *length = cachedPath->size();
        return true;
    }

    // Not in cache -> compute full path and add
    std::vector<Direction> newRoute;
    if(!FindShipPath(start, coastalPoint, std::numeric_limits<unsigned>::max(), &newRoute, length))
        return false;
    shipPathData.addToCache(start, coastalPoint, newRoute);
    if(route)
        *route = std::move(newRoute);

    return true;
}

bool GameWorldBase::FindShipPath(const MapPoint start, const MapPoint dest, unsigned maxDistance,
                                 std::vector<Direction>* route, unsigned* length) const
{
    return GetFreePathFinder().FindPath(start, dest, true, maxDistance, route, length, nullptr,
                                        PathConditionShip(*this));
}

/// Prüft, ob eine Schiffsroute noch Gültigkeit hat
bool GameWorld::CheckShipRoute(const MapPoint start, const std::vector<Direction>& route, const unsigned pos,
                               MapPoint* dest) const
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
        return std::nullopt;

    RTTR_Assert(GetNO(dest)->GetType() == NodalObjectType::Flag); // Goal should be the flag of a wh

    if(!PathConditionHuman(*this).IsNodeOk(dest))
        return std::nullopt;

    Direction first_dir{};
    if(GetFreePathFinder().FindPath(start, dest, random_route, max_route, route, length, &first_dir,
                                    PathConditionTrade(*this, player)))
        return first_dir;
    else
        return std::nullopt;
}

bool GameWorld::CheckTradeRoute(const MapPoint start, const std::vector<Direction>& route, unsigned pos,
                                unsigned char player, MapPoint* dest) const
{
    return GetFreePathFinder().CheckRoute(start, route, pos, PathConditionTrade(*this, player), dest);
}
