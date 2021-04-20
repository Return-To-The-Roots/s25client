// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "helpers/OptionalEnum.h"
#include "world/TradePath.h"
#include "gameTypes/MapCoordinates.h"
#include "gameTypes/TradeDirection.h"

class SerializedGameData;
class GameWorld;

/// active route for trading. Has a state and supports automatic recalculation of the path
class TradeRoute
{
    const GameWorld& world;
    const unsigned char player;
    TradePath path;
    MapPoint curPos;
    unsigned curRouteIdx;

    helpers::OptionalEnum<TradeDirection> RecalcRoute();

public:
    TradeRoute(const GameWorld& world, unsigned char player, const MapPoint& start, const MapPoint& goal);
    TradeRoute(SerializedGameData& sgd, const GameWorld& world, unsigned char player);

    void Serialize(SerializedGameData& sgd) const;

    /// Gets the next direction the caravane has to take, TradeDirection::ReachedGoal or boost::none
    helpers::OptionalEnum<TradeDirection> GetNextDir();
    /// Returns the current position. This is assumed to be the position currently walking to and reached by the time
    /// GetNextDir should be called
    MapPoint GetCurPos() const { return curPos; }

    /// Returns true, if this is a valid route
    bool IsValid() const { return !path.route.empty() || path.start == path.goal; }

    /// Assigns new start and goal positions and hence, a new route
    void AssignNewGoal(const MapPoint& start, const MapPoint& newGoal);

    const TradePath& GetTradePath() const { return path; }
};
