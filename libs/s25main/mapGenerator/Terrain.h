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

#pragma once

#include "mapGenerator/Algorithms.h"
#include "mapGenerator/Map.h"
#include "mapGenerator/RandomUtility.h"
#include <functional>

namespace rttr { namespace mapGenerator {

    /**
     * Restructures the specified height map by elevating landscape preferably around the specified focus area.
     * Nodes closer to the focus area experience more elevation than nodes further away from the focus area.
     *
     * @param map reference to the map to manipulate
     * @param predicate predicate defining the focus of elevation
     * @param weight factor influencing the strength of elevation towards the focused area (default: 2 - quadratic drop
     * with distance)
     */
    void Restructure(Map& map, std::function<bool(const MapPoint&)> predicate, double weight = 2.);

    /**
     * Resets the sea level to "0" by setting all sea nodes to "0" height and scaling the remaining nodes to a range
     * between "1" and maximum height of the map.
     *
     * @param map reference to the map
     * @param seaLevel maximum height sea can reach
     */
    void ResetSeaLevel(Map& map, RandomUtility& rnd, unsigned seaLevel);

}} // namespace rttr::mapGenerator
