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
#include <stdint.h>

/**
 * Random map configuration.
 */
class RandomConfig
{
public:
    enum MapType{
        /** Greenland maps are covered by grass and mountains with very few lakes. */
        Greenland,
        /** Riverland maps are close to greenland maps but with plenty of water. */
        Riverland,
        /** Ringland maps usually covered by water apart from a ring-shaped piece of land. */
        Ringland,
        /** On migration style maps players are starting on their own little island.
         *  The main resources (mountains), however, are available only on one large
         *  island in the center of the map. */
        Migration,
        /** Each player starts on its own island which contains all relevant resources
         *  (trees, stone piles, mountains). */
        Islands,
        /** Continent maps are big islands surrounded by water.
         *  Each player starts on the same big island. */
        Continent,
        /** full random map */
        Random
    };
    RandomConfig(MapType mapType);
    RandomConfig(MapType mapType, uint64_t seed);

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
private:
    void Init(MapType mapType, uint64_t seed);
    void CreateGreenland();
    void CreateRiverland();
    void CreateRingland();
    void CreateMigration();
    void CreateIslands();
    void CreateContinent();
    void CreateRandom();
    
};

#endif // RandomConfig_h__
