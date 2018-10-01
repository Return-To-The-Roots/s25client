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

#include "rttrDefines.h" // IWYU pragma: keep
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
    sgd.PushMapPoint(curPos);
    sgd.PushUnsignedInt(curRouteIdx);
}

/// Gets the next direction the caravane has to take
unsigned char TradeRoute::GetNextDir()
{
    if(curPos == path.goal)
        return REACHED_GOAL;

    if(curRouteIdx >= path.route.size())
        return INVALID_DIR;

    unsigned char nextDir;
    // Check if the route is still valid
    if(gwg.CheckTradeRoute(curPos, path.route, curRouteIdx, player))
        nextDir = path.route[curRouteIdx].toUInt();
    else
    {
        // If not, recalc it
        nextDir = RecalcRoute();
        // Check if we found a valid direction
        if(nextDir >= 6)
            return nextDir; // If not, bail out
    }

    RTTR_Assert(nextDir < 6);
    RTTR_Assert(nextDir == path.route[curRouteIdx].toUInt());
    curRouteIdx++;
    curPos = gwg.GetNeighbour(curPos, Direction::fromInt(nextDir));
    return nextDir;
}

/// Recalc local route and returns next direction
unsigned char TradeRoute::RecalcRoute()
{
    /// Are we at the goal?
    if(curPos == path.goal)
        return REACHED_GOAL;

    path.start = curPos;
    path.route.clear();
    unsigned char nextDir = gwg.FindTradePath(path.start, path.goal, player, std::numeric_limits<unsigned>::max(), false, &path.route);
    curRouteIdx = 0;
    return nextDir;
}

/// Assigns new start and goal positions and hence, a new route
void TradeRoute::AssignNewGoal(const MapPoint& start, const MapPoint& newGoal)
{
    curPos = start;
    path.goal = newGoal;
    path.route.clear();
    RecalcRoute();
}
