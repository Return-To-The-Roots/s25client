// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "world/TradeRoute.h"
#include "SerializedGameData.h"
#include "world/GameWorld.h"
#include <limits>

TradeRoute::TradeRoute(const GameWorld& world, unsigned char player, const MapPoint& start, const MapPoint& goal)
    : world(world), player(player)
{
    assignNewGoal(start, goal);
}

TradeRoute::TradeRoute(SerializedGameData& sgd, const GameWorld& world, const unsigned char player)
    : world(world), player(player), path(sgd), curPos(sgd.popMapPoint()), curRouteIdx(sgd.popUnsignedInt())
{}

void TradeRoute::serialize(SerializedGameData& sgd) const
{
    path.serialize(sgd);
    helpers::pushPoint(sgd, curPos);
    sgd.pushUnsignedInt(curRouteIdx);
}

/// Gets the next direction the caravane has to take
helpers::OptionalEnum<TradeDirection> TradeRoute::getNextDir()
{
    if(curPos == path.goal)
        return TradeDirection::ReachedGoal;

    if(curRouteIdx >= path.route.size())
        return std::nullopt;

    Direction nextDir;
    // Check if the route is still valid
    if(world.checkTradeRoute(curPos, path.route, curRouteIdx, player))
        nextDir = path.route[curRouteIdx];
    else
    {
        // If not, recalc it
        const auto calculatedNextDir = recalcRoute();
        // Check if we found a valid direction
        if(!calculatedNextDir)
            return calculatedNextDir;              // If not, bail out
        nextDir = toDirection(*calculatedNextDir); // ReachedGoal is not possible
    }

    RTTR_Assert(nextDir == path.route[curRouteIdx]);
    curRouteIdx++;
    curPos = world.getNeighbour(curPos, nextDir);
    return TradeDirection(rttr::enumCast(nextDir));
}

/// Recalc local route and returns next direction
helpers::OptionalEnum<TradeDirection> TradeRoute::recalcRoute()
{
    /// Are we at the goal?
    if(curPos == path.goal)
        return TradeDirection::ReachedGoal;

    path.start = curPos;
    path.route.clear();
    const auto nextDir =
      world.findTradePath(path.start, path.goal, player, std::numeric_limits<unsigned>::max(), false, &path.route);
    curRouteIdx = 0;
    if(nextDir)
        return TradeDirection(rttr::enumCast(*nextDir));
    else
        return std::nullopt;
}

/// Assigns new start and goal positions and hence, a new route
void TradeRoute::assignNewGoal(const MapPoint& start, const MapPoint& newGoal)
{
    curPos = start;
    path.goal = newGoal;
    path.route.clear();
    recalcRoute();
}
