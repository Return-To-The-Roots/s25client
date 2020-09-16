// Copyright (c) 2016 - 2017 Settlers Freaks (sf-team at siedler25.org)
//
// This file is part of Return To The Roots.
//
// Return To The Roots is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation,  either version 2 of the License,  or
// (at your option) any later version.
//
// Return To The Roots is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with Return To The Roots. If not,  see <http://www.gnu.org/licenses/>.

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
