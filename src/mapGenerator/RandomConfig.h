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

#include "defines.h" // IWYU pragma: keep
#include "mapGenerator/AreaDesc.h"
#include "gameTypes/MapTypes.h"
#include "Random.h"
#include <vector>
#include <cstdlib>

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
    
    static RandomConfig CreateGreenland();
    static RandomConfig CreateRiverland();
    static RandomConfig CreateRingland();
    static RandomConfig CreateMigration();
    static RandomConfig CreateIslands();
    static RandomConfig CreateContinent();
    static RandomConfig CreateRandom();
    
    /**
     * Generates a random number between min and max.
     * @param min minimum value
     * @param max maximum value
     * @return a new random number
     */
    static int Rand(const int min, const int max)
    {
        return min + rand() % (max - min);
        //return min + RANDOM.Rand(__FILE__, __LINE__, 0, max - min);
    }
    
    /**
     * Generates a random number between min and max.
     * @param min minimum value
     * @param max maximum value
     * @return a new random number
     */
    static double DRand(const double min, const double max)
    {
        return min + static_cast<double>(Rand(0, RAND_MAX)) /
        (static_cast<double>(RAND_MAX/(max - min)));
    }
};

#endif // RandomConfig_h__
