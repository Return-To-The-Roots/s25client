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

#ifndef RandomMapGenerator_h__
#define RandomMapGenerator_h__

#include "defines.h" // IWYU pragma: keep

#include "mapGenerator/AreaDesc.h"
#include "mapGenerator/Map.h"
#include "mapGenerator/MapSettings.h"
#include "mapGenerator/MapUtility.h"

#include "gameTypes/MapTypes.h"
#include "Random.h"

#include <vector>
#include <cstdlib>

#define MAXIMUM_HEIGHT 25

/**
 * Random map generator.
 */
class RandomMapGenerator
{
    public:

    /**
     * Creates a new RandomMapGenerator with random area properties.
     * @param random whether or not to generate random area description for map generation
     */
    RandomMapGenerator(bool random = true);

    
    /**
     * Generates a new random map with the specified settings.
     * @param settings settings used for the map generation
     */
    Map* Create(const MapSettings& settings);
    
    protected:
    
    /**
     * Descriptions of different areas used to generate the random map.
     */
    std::vector<AreaDesc> _areas;

    
    /**
     * Textures used for different elevations of the map.
     */
    TerrainType _textures[MAXIMUM_HEIGHT];
    
    /**
     * Generates a random number between min and max.
     * @param min minimum value
     * @param max maximum value
     * @return a new random number
     */
    int Rand(const int min, const int max)
    {
        return min + rand() % (max - min);
        //        return min + RANDOM.Rand(__FILE__, __LINE__, 0, max - min);
    }
    
    /**
     * Generates a random number between min and max.
     * @param min minimum value
     * @param max maximum value
     * @return a new random number
     */
    double DRand(const double min, const double max)
    {
        return min + static_cast<double>(Rand(0, RAND_MAX)) /
        (static_cast<double>(RAND_MAX/(max - min)));
    }

    private:
    
    /**
     * Helper to generate random maps (tree placement, water, coastlines, ...).
     */
    MapUtility _helper;
    
    /**
     * Gets the highest possible elevation (height value) for the specified terrain.
     * @param terrain terrain type to evaluate the maximum height for
     * @return the maximum height value for the terrain
     */
    int GetMaxTerrainHeight(const TerrainType terrain);
    
    /**
     * Gets the minimum height to be considered as specified terrain.
     * @param terrain terrain type to evaluate the minimum height for
     * @return the minimum height value for the terrain
     */
    int GetMinTerrainHeight(const TerrainType terrain);
    
    /**
     * Create player positions (headquarters) for the specified map.
     * @param settings settings used for map generation
     * @param map map to modify
     */
    void PlacePlayers(const MapSettings& settings, Map* map);

    /**
     * Create standard resoures for each player.
     * @param settings settings used for map generation
     * @param map map to modify
     */
    void PlacePlayerResources(const MapSettings& settings, Map* map);
    
    /**
     * Create a elevation (hills) for the specified map.
     * @param settings settings used for map generation
     * @param map map to modify
     */
    void CreateHills(const MapSettings& settings, Map* map);
    
    /**
     * Fill the remaining terrain (apart from the player positions) according to the generated hills.
     * @param settings settings used for map generation
     * @param map map to modify
     */
    void FillRemainingTerrain(const MapSettings& settings, Map* map);
 };

#endif // RandomMapGenerator_h__
