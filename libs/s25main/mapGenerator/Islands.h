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

#ifndef Islands_h__
#define Islands_h__

#include "mapGenerator/Map.h"
#include "mapGenerator/RandomUtility.h"

namespace rttr { namespace mapGenerator {

    using Island = std::set<MapPoint, MapPointLess>;

    /**
     * Finds all islands on the map which consist of at least the specified minimum number of nodes.
     *
     * @param textures reference to the texture map to search for islands
     * @param minNodes minimum number of nodes of an island
     *
     * @returns vector of islands which are themselves a vector of points on the map.
     */
    std::vector<std::vector<MapPoint>> FindIslands(const TextureMap& textures, unsigned minNodes);

    /**
     * Creates a new island at the specified position on the map.
     *
     * @param map reference to the map to place the island on (manipulates textures and z-values of the map)
     * @param rnd random number generator
     * @param distanceToLand minimum distance of the island to land textures
     * @param size number of nodes the island should cover (in case there's not sufficient water the island will be smaller)
     * @param radius radius of the brush to paint the island with
     * @param mountainCoverage preferred mountain coverage for the island in percentage (between 0 and 1)
     *
     * @returns a vector of nodes the new island covers.
     */
    Island CreateIsland(Map& map, RandomUtility& rnd, unsigned distanceToLand, unsigned size, unsigned radius, double mountainCoverage);

}} // namespace rttr::mapGenerator

#endif // Islands_h__
