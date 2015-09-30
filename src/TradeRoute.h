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

#ifndef TradeRoute_h__
#define TradeRoute_h__

#include "gameTypes/MapTypes.h"
#include <vector>

class TradeGraph;
class SerializedGameData;
class GameWorldGame;

/// Constants used for Pathfinding
const unsigned char NO_PATH = 0xff;
const unsigned char REACHED_GOAL = 0xdd;

class TradeRoute
{
    /// Reference to the trade graph
    const TradeGraph* const tg;

    /// Start and goal, current posistion in usual map coordinates and TG coordinates
    MapPoint start, goal, current_pos, current_pos_tg;
    /// Current "global" route on the trade graph
    std::vector<unsigned char> global_route;
    unsigned global_pos;
    /// Current "local" route from one main point to another main point
    std::vector<unsigned char> local_route;
    unsigned local_pos;
private:

    /// Recalc local route and returns next direction
    unsigned char RecalcLocalRoute();
    /// Recalc the whole route and returns next direction
    unsigned char RecalcGlobalRoute();

public:

    TradeRoute(const TradeGraph* const tg, const MapPoint start, const MapPoint goal) :
        tg(tg), start(start), goal(goal), current_pos(start), global_pos(0), local_pos(0) { RecalcGlobalRoute(); }
    TradeRoute(SerializedGameData& sgd, const GameWorldGame* const gwg, const unsigned char player);

    void Serialize(SerializedGameData& sgd) const;

    /// Was a route found?
    bool IsValid() const { return !local_route.empty(); }
    /// Gets the next direction the caravane has to take
    unsigned char GetNextDir();
    /// Assigns new start and goal positions and hence, a new route
    void AssignNewGoal(const MapPoint new_goal, const MapPoint current);
};

#endif // TradeRoute_h__