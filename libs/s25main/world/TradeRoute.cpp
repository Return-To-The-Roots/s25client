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

#include "world/TradeRoute.h"
#include "SerializedGameData.h"
#include "world/GameWorldGame.h"
#include "gameData/GameConsts.h"

TradeRoute::TradeRoute(const GameWorldGame& gwg, unsigned char player, const MapPoint& start, const MapPoint& goal)
    : gwg(gwg), player(player)
{
    AssignNewGoal(start, goal);
}

TradeRoute::TradeRoute(SerializedGameData& sgd, const GameWorldGame& gwg, const unsigned char player)
    : gwg(gwg), player(player), path(sgd), curPos(sgd.PopMapPoint()), curRouteIdx(sgd.PopUnsignedInt())
{}

void TradeRoute::Serialize(SerializedGameData& sgd) const
{
    path.Serialize(sgd);
    helpers::pushPoint(sgd, curPos);
    sgd.PushUnsignedInt(curRouteIdx);
}

/// Gets the next direction the caravane has to take
helpers::OptionalEnum<TradeDirection> TradeRoute::GetNextDir()
{
    if(curPos == path.goal)
        return TradeDirection::ReachedGoal;

    if(curRouteIdx >= path.route.size())
        return boost::none;

    Direction nextDir;
    // Check if the route is still valid
    if(gwg.CheckTradeRoute(curPos, path.route, curRouteIdx, player))
        nextDir = path.route[curRouteIdx];
    else
    {
        // If not, recalc it
        const auto calculatedNextDir = RecalcRoute();
        // Check if we found a valid direction
        if(!calculatedNextDir)
            return calculatedNextDir;              // If not, bail out
        nextDir = toDirection(*calculatedNextDir); // ReachedGoal is not possible
    }

    RTTR_Assert(nextDir == path.route[curRouteIdx]);
    curRouteIdx++;
    curPos = gwg.GetNeighbour(curPos, nextDir);
    return TradeDirection(rttr::enum_cast(nextDir));
}

/// Recalc local route and returns next direction
helpers::OptionalEnum<TradeDirection> TradeRoute::RecalcRoute()
{
    /// Are we at the goal?
    if(curPos == path.goal)
        return TradeDirection::ReachedGoal;

    path.start = curPos;
    path.route.clear();
    const auto nextDir =
      gwg.FindTradePath(path.start, path.goal, player, std::numeric_limits<unsigned>::max(), false, &path.route);
    curRouteIdx = 0;
    if(nextDir)
        return TradeDirection(rttr::enum_cast(*nextDir));
    else
        return boost::none;
}

/// Assigns new start and goal positions and hence, a new route
void TradeRoute::AssignNewGoal(const MapPoint& start, const MapPoint& newGoal)
{
    curPos = start;
    path.goal = newGoal;
    path.route.clear();
    RecalcRoute();
}
