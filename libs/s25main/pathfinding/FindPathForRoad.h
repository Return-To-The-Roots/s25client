// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "gameTypes/Direction.h"
#include "gameTypes/MapCoordinates.h"
#include <vector>

class GameWorldBase;
class GameWorldViewer;

namespace detail {
template<class T_WorldOrViewer>
std::vector<Direction> FindPathForRoad(const T_WorldOrViewer& world, MapPoint startPt, MapPoint endPt, bool isBoatRoad,
                                       unsigned maxLen);
}

/// Wrapper for finding a path for a road
/// Takes a world or worldView instance that has a IsRoadAvailable function to call for each Node
/// Returns the road directions as a vector or an empty vector if no possible road was found
inline std::vector<Direction> FindPathForRoad(const GameWorldBase& world, const MapPoint startPt, const MapPoint endPt,
                                              bool isBoatRoad, unsigned maxLen = 100)
{
    return detail::FindPathForRoad(world, startPt, endPt, isBoatRoad, maxLen);
}
inline std::vector<Direction> FindPathForRoad(const GameWorldViewer& world, const MapPoint startPt,
                                              const MapPoint endPt, bool isBoatRoad, unsigned maxLen = 100)
{
    return detail::FindPathForRoad(world, startPt, endPt, isBoatRoad, maxLen);
}
