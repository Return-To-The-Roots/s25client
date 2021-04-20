// Copyright (c) 2005 - 2017 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "gameTypes/Direction.h"
#include "gameTypes/MapCoordinates.h"
#include <utility>
#include <vector>

class SerializedGameData;

/// Represents a path from start to goal
struct TradePath
{
    MapPoint start, goal;
    std::vector<Direction> route;

    TradePath() = default;
    TradePath(MapPoint start, MapPoint goal, std::vector<Direction> route)
        : start(start), goal(goal), route(std::move(route))
    {}
    TradePath(SerializedGameData& sgd);

    void Serialize(SerializedGameData& sgd) const;
};
