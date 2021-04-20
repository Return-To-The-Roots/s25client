// Copyright (c) 2005 - 2017 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "mapGenerator/Map.h"
#include "mapGenerator/RandomUtility.h"

namespace rttr { namespace mapGenerator {

    using Island = std::set<MapPoint, MapPointLess>;

    /**
     * Creates a new island at the specified position on the map.
     *
     * @param map reference to the map to place the island on (manipulates textures and z-values of the map)
     * @param rnd random number generator
     * @param size number of nodes the island should cover (in case there's not sufficient water the island will be
     * smaller)
     * @param minLandDist minimum distance of the island to land textures
     * @param mountainCoverage preferred mountain coverage for the island in percentage (between 0 and 1)
     *
     * @returns a vector of nodes the new island covers.
     */
    Island CreateIsland(Map& map, RandomUtility& rnd, unsigned size, unsigned minLandDist, double mountainCoverage);

}} // namespace rttr::mapGenerator
