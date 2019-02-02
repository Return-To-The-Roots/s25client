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

#ifndef TradeRoute_h__
#define TradeRoute_h__

#include "world/TradePath.h"
#include "gameTypes/MapCoordinates.h"

class SerializedGameData;
class GameWorldGame;

/// Constants used for Pathfinding
const unsigned char REACHED_GOAL = 0xDD;

/// active route for trading. Has a state and supports automatic recalculation of the path
class TradeRoute
{
    const GameWorldGame& gwg;
    const unsigned char player;
    TradePath path;
    MapPoint curPos;
    unsigned curRouteIdx;

    unsigned char RecalcRoute();

public:
    TradeRoute(const GameWorldGame& gwg, unsigned char player, const MapPoint& start, const MapPoint& goal);
    TradeRoute(SerializedGameData& sgd, const GameWorldGame& gwg, unsigned char player);

    void Serialize(SerializedGameData& sgd) const;

    /// Gets the next direction the caravane has to take, REACHED_GOAL or INVALID_DIR
    unsigned char GetNextDir();
    /// Returns the current position. This is assumed to be the position currently walking to and reached by the time GetNextDir should be
    /// called
    MapPoint GetCurPos() const { return curPos; }

    /// Returns true, if this is a valid route
    bool IsValid() const { return !path.route.empty() || path.start == path.goal; }

    /// Assigns new start and goal positions and hence, a new route
    void AssignNewGoal(const MapPoint& start, const MapPoint& newGoal);

    const TradePath& GetTradePath() const { return path; }
};

#endif // TradeRoute_h__
