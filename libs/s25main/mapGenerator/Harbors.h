// Copyright (c) 2005 - 2017 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "mapGenerator/Map.h"
#include "mapGenerator/Rivers.h"

namespace rttr { namespace mapGenerator {

    /**
     * Places a harbor position on the map.
     * @param map reference to the map to place the harbor position on
     * @param position position for the harbor
     */
    void PlaceHarborPosition(Map& map, const MapPoint& position);

    /**
     * Finds all connected nodes which are part of a coastline. A coast node must fulfill following three criteria:
     * 1) a coast node is surrounded by at least one land & one water texture
     * 2) a coast node is at least 5 edges away from the next river
     * 3) a coast node has a neighbor node which is fully surrounded by water
     *
     * @param map reference to the map to look up coastlines for
     * @param rivers all rivers already placed on the map
     */
    std::vector<std::vector<MapPoint>> FindCoastlines(const Map& map, const std::vector<River>& rivers);

    /**
     * Places harbors on the specified map in suitable positions based on the given parameters.
     *
     * @param map reference to the map to add harbors positions to
     * @param rivers all rivers already placed on the map to avoid harbor positions for tiny rivers
     * @param coastSize minimum number of nodes a coastline requires to be considered for harbor positions
     * @param nodesPerHarbor number of coast nodes required per harbor position
     */
    void PlaceHarbors(Map& map, const std::vector<River>& rivers, int coastSize = 32, int nodesPerHarbor = 64);

}} // namespace rttr::mapGenerator
