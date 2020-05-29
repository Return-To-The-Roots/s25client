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

#ifndef Harbors_h__
#define Harbors_h__

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
     * Places harbors on the specified map in suitable positions based on the given parameters.
     *
     * @param map map to place harbors on
     * @param minimumIslandSize minimum size of an island to consider it for harbor placement
     * @param minimumCoastSize minimum size of a coastline of an island to consider it for harbor placement
     * @param rivers all rivers already placed on the map
     * @param maximumIslandHarbors maximum number of harbors per island (default: no limit, -1)
     */
    void PlaceHarbors(Map& map, int minimumIslandSize, int minimumCoastSize, const std::vector<River>& rivers,
                      int maximumIslandHarbors = -1);

}} // namespace rttr::mapGenerator

#endif // Harbors_h__
