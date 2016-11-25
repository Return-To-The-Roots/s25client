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

#include "mapGenerator/AreaDesc.h"
#include "mapGenerator/Map.h"
#include "mapGenerator/MapSettings.h"
#include "mapGenerator/MapUtility.h"
#include "gameTypes/MapTypes.h"

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
    RandomMapGenerator(bool random = true)
    {
        _textures[0]    = TT_WATER;
        _textures[1]    = TT_WATER;
        _textures[2]    = TT_WATER;
        _textures[3]    = TT_WATER;
        _textures[4]    = TT_DESERT;
        _textures[5]    = TT_STEPPE;
        _textures[6]    = TT_SAVANNAH;
        _textures[7]    = TT_MEADOW1;
        _textures[8]    = TT_MEADOW_FLOWERS;
        _textures[9]    = TT_MEADOW2;
        _textures[10]   = TT_MEADOW2;
        _textures[11]   = TT_MOUNTAINMEADOW;
        _textures[12]   = TT_MOUNTAIN1;
        _textures[13]   = TT_MOUNTAIN1;
        _textures[14]   = TT_MOUNTAIN1;
        _textures[15]   = TT_SNOW;
        _textures[16]   = TT_SNOW;
        _textures[17]   = TT_SNOW;
        _textures[18]   = TT_SNOW;
        _textures[19]   = TT_SNOW;
        _textures[20]   = TT_SNOW;
        _textures[21]   = TT_SNOW;
        _textures[22]   = TT_SNOW;
        _textures[23]   = TT_SNOW;
        _textures[24]   = TT_SNOW;
        
        if (!random)
        {
            return;
        }
        
        const double p1 = DRand(0.0, 0.4);
        const double p2 = DRand(p1, p1 + 1.4);
        const double p3 = DRand(p2, p2 + 1.0);
        const double pHill = DRand(1.5, 5.0);
        const int minHill = rand() % 5;
        
        _areas.push_back(AreaDesc(0.5, 0.5, 0.0, p1,    1.0,  4, 7, 0, 23, 15));
        _areas.push_back(AreaDesc(0.5, 0.5, p1,  p2,    pHill, 18, 5, minHill, 10, 15));
        _areas.push_back(AreaDesc(0.5, 0.5, p1,  p2,    0.5,  0, 0, 0, 17, 18));
        _areas.push_back(AreaDesc(0.5, 0.5, p2,  p3,    0.1, 15, 5, 0,  7, 15));
        _areas.push_back(AreaDesc(0.5, 0.5, 0.0, 2.0, 100.0,  0, 0, 7,  7,  0, 4));
        _areas.push_back(AreaDesc(0.5, 0.5, 0.0, 2.0, 100.0,  8, 0, 5, 10,  4, 15));
    }
    
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
     * Create a new, empty terain for the specified map.
     * @param settings settings used for map generation
     * @param map map to modify
     */
    void CreateEmptyTerrain(const MapSettings& settings, Map* map);
    
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

    /**
     * Generates a random number between min and max.
     * @param min minimum values
     * @param max maximum value
     */
    double DRand(const double min, const double max)
    {
        return min + static_cast<double>(rand()) / (static_cast<double>(RAND_MAX/(max - min)));
    }
 };

#endif // RandomMapGenerator_h__
