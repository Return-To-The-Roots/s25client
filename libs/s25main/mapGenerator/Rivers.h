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

#ifndef Rivers_h__
#define Rivers_h__

#include "mapGenerator/Map.h"
#include "mapGenerator/RandomUtility.h"

namespace rttr { namespace mapGenerator {

    using River = std::set<MapPoint, MapPointLess>;

    /**
     * Creates a small stream of water for the specified map with the specified initial direction, length and split rate.
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
    River CreateStream(RandomUtility& rnd, Map& map, const MapPoint& source, Direction direction, unsigned length, unsigned splitRate = 0);

}} // namespace rttr::mapGenerator

#endif // Rivers_h__
