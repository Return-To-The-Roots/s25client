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
#include <limits>

/// Findet einen Weg für Figuren
helpers::OptionalEnum<Direction> GameWorldBase::findHumanPath(const MapPoint start, const MapPoint dest,
                                                              const unsigned maxRoute, const bool randomRoute,
                                                              unsigned* length, std::vector<Direction>* route) const
{
    Direction firstDir{};
    if(getFreePathFinder().findPath(start, dest, randomRoute, maxRoute, route, length, &firstDir,
                                    PathConditionHuman(*this)))
        return firstDir;
    else
        return std::nullopt;
}

/// Wegfindung für Menschen im Straßennetz
RoadPathDirection GameWorld::findHumanPathOnRoads(const noRoadNode& start, const noRoadNode& goal, unsigned* length,
                                                  MapPoint* firstPt, const RoadSegment* const forbidden)
{
    RoadPathDirection firstDir;
    if(getRoadPathFinder().findPath(start, goal, false, std::numeric_limits<unsigned>::max(), forbidden, length,
                                    &firstDir, firstPt))
        return firstDir;
    else
        return RoadPathDirection::None;
}

/// Wegfindung für Waren im Straßennetz
RoadPathDirection GameWorld::findPathForWareOnRoads(const noRoadNode& start, const noRoadNode& goal, unsigned* length,
                                                    MapPoint* firstPt, unsigned max)
{
    RoadPathDirection firstDir;
    if(getRoadPathFinder().findPath(start, goal, true, max, nullptr, length, &firstDir, firstPt))
        return firstDir;
    else
        return RoadPathDirection::None;
}

bool GameWorldBase::findShipPathToHarbor(const MapPoint start, HarborId harborId, SeaId seaId,
                                         std::vector<Direction>* route, unsigned* length) const
{
    const MapPoint coastalPoint = getCoastalPoint(harborId, seaId);
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
    auto& shipPathData = getShipPathData();
    // If we start from a harbor we can get the route directly w/o the costly pathfinding
    for(const auto startHbId : helpers::idRange<HarborId>(getNumHarborPoints()))
    {
        if(getCoastalPoint(startHbId, seaId) == start)
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
    if(!findShipPath(start, coastalPoint, std::numeric_limits<unsigned>::max(), &newRoute, length))
        return false;
    shipPathData.addToCache(start, coastalPoint, newRoute);
    if(route)
        *route = std::move(newRoute);

    return true;
}

bool GameWorldBase::findShipPath(const MapPoint start, const MapPoint dest, unsigned maxDistance,
                                 std::vector<Direction>* route, unsigned* length) const
{
    return getFreePathFinder().findPath(start, dest, true, maxDistance, route, length, nullptr,
                                        PathConditionShip(*this));
}

/// Prüft, ob eine Schiffsroute noch Gültigkeit hat
bool GameWorld::checkShipRoute(const MapPoint start, const std::vector<Direction>& route, const unsigned pos,
                               MapPoint* dest) const
{
    return getFreePathFinder().checkRoute(start, route, pos, PathConditionShip(*this), dest);
}

/// Find a route for trade caravanes
helpers::OptionalEnum<Direction> GameWorld::findTradePath(const MapPoint start, const MapPoint dest,
                                                          unsigned char player, unsigned maxRoute, bool randomRoute,
                                                          std::vector<Direction>* route, unsigned* length) const
{
    unsigned char owner = getNode(dest).owner;
    if(owner != 0 && !getPlayer(player).isAlly(owner - 1))
        return std::nullopt;

    RTTR_Assert(getNO(dest)->getType() == NodalObjectType::Flag); // Goal should be the flag of a wh

    if(!PathConditionHuman(*this).isNodeOk(dest))
        return std::nullopt;

    Direction firstDir{};
    if(getFreePathFinder().findPath(start, dest, randomRoute, maxRoute, route, length, &firstDir,
                                    PathConditionTrade(*this, player)))
        return firstDir;
    else
        return std::nullopt;
}

bool GameWorld::checkTradeRoute(const MapPoint start, const std::vector<Direction>& route, unsigned pos,
                                unsigned char player, MapPoint* dest) const
{
    return getFreePathFinder().checkRoute(start, route, pos, PathConditionTrade(*this, player), dest);
}
