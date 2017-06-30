// Copyright (c) 2005 - 2015 Settlers Freaks (sf-team at siedler25.org)
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

#ifndef RandomConfig_h__
#define RandomConfig_h__

#include "mapGenerator/AreaDesc.h"
#include "gameTypes/MapTypes.h"
#include <vector>

/**
 * Random map configuration.
 */
struct RandomConfig
{
    /**
     * Description of different areas to use for random map generation.
     */
    std::vector<AreaDesc> areas;
    
    /**
     * Mapping of elevation (= height values) to terrain. The number of elements defines the
     * maximum height value used to generate the random map.
     * For example, following statement will assign water textures to landscape with the 
     * height of "0": textures[0] = TT_WATER;
     */
    std::vector<TerrainType> textures;
    
    /**
     * Creates a new configuration for greenland random maps.
     * Greendland maps are covered by grass and mountains with very few lakes.
     * @return a new configuration for greenland random maps
     */
    static RandomConfig CreateGreenland();

    /**
     * Creates a new configuration for riverland random maps. Riverland maps are
     * close to greenland maps but with plenty of water.
     * @return a new configuration for riverland random maps
     */
    static RandomConfig CreateRiverland();
    
    /**
     * Creates a new configuration for ringland random maps. Ringland maps usually
     * covered by water apart from a ring-shaped piece of land.
     * @return a new configuration for ringland random maps
     */
    static RandomConfig CreateRingland();
    
    /**
     * Creates a new configuration for migration random maps. On migration style maps
     * players are starting on their own little island. The main resources (mountains),
     * however, are available only on one large island in the center of the map.
     * @return a new configuration for migration random maps
     */
    static RandomConfig CreateMigration();
    
    /**
     * Creates a new configuration for islands random maps. Each player starts on its
     * own island which contains all relevant resources (trees, stone piles, mountains).
     * @return a new configuration for islands random maps
     */
    static RandomConfig CreateIslands();
    
    /**
     * Creates a new configuration for continental random maps. Continent maps are big
     * islands surrounded by water. Each player starts on the same big island.
     * @return a new configuration for continental random maps
     */
    static RandomConfig CreateContinent();
    
    /**
     * Creates a new configuration for full random maps.
     * @return a new configuration for full random maps
     */
    static RandomConfig CreateRandom();
    
    /**
     * Generates a random number between 0 and max-1.
     * @param max maximum value
     * @return a new random number
     */
     int Rand(const int max);
    
    /**
     * Generates a random number between min and max-1.
     * @param min minimum value
     * @param max maximum value
     * @return a new random number
     */
    int Rand(const int min, const int max);
    
    /**
     * Generates a random number between min and max.
     * @param min minimum value
     * @param max maximum value
     * @return a new random number
     */
    double DRand(const double min, const double max);
};

#endif // RandomConfig_h__
