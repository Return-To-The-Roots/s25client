// Copyright (c) 2005 - 2017 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "mapGenerator/Map.h"
#include "mapGenerator/RandomUtility.h"

namespace rttr { namespace mapGenerator {

    using River = std::set<MapPoint, MapPointLess>;

    /**
     * Creates a small stream of water for the specified map with the specified initial direction, length and split
     * rate.
     *
     * @param rnd random number generator to to create a random flow
     * @param map reference to the map to place the stream on
     * @param source source of the stream
     * @param direction initial direction of the stream
     * @param length length of the ditch in triangles
     * @param splitRate chance of the ditch to split up into two streams (0 by default, a number between 0 and 100)
     *
     * @returns all nodes the stream (incl. split up streams) is covering.
     */
    River CreateStream(RandomUtility& rnd, Map& map, const MapPoint& source, Direction direction, unsigned length,
                       unsigned splitRate = 0);

}} // namespace rttr::mapGenerator
