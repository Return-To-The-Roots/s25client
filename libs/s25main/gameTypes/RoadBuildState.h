// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "gameTypes/Direction.h"
#include "gameTypes/MapCoordinates.h"
#include "gameTypes/RoadBuildMode.h"
#include <vector>

struct RoadBuildState
{
    RoadBuildMode mode;

    MapPoint point, start;
    std::vector<Direction> route; /// Directions of the built road
};
