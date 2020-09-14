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

#pragma once

#include "mapGenerator/MapUtility.h"

class RandomConfig;
struct TerrainDesc;
struct MapSettings;
struct Map;

/**
 * Random map generator.
 */
class RandomMapGenerator
{
    RandomConfig& config;

public:
    /**
     * Creates a new RandomMapGenerator with random properties.
     */
    RandomMapGenerator(RandomConfig& config);

    /**
     * Generates a new random map with the specified settings.
     * @param settings settings used for the map generation
     * @param config configuration for the random map generator
     */
    Map Create(MapSettings settings);

private:
    /**
     * Helper to generate random maps (tree placement, water, coastlines, ...).
     */
    MapUtility helper;

    /**
     * Gets the highest possible elevation (height value) for the specified terrain.
     * @param terrain terrain type to evaluate the maximum height for
     * @param textures set of textures used for different height values
     * @return the maximum height value for the terrain
     */
    unsigned GetMaxTerrainHeight(DescIdx<TerrainDesc> terrain);

    /**
     * Gets the minimum height to be considered as specified terrain.
     * @param terrain terrain type to evaluate the minimum height for
     * @param textures set of textures used for different height values
     * @return the minimum height value for the terrain
     */
    unsigned GetMinTerrainHeight(DescIdx<TerrainDesc> terrain);

    /**
     * Create player positions (headquarters) for the specified map.
     * @param settings settings used for map generation
     * @param map map to modify
     */
    void PlacePlayers(const MapSettings& settings, Map& map);

    /**
     * Create standard resoures for each player.
     * @param settings settings used for map generation
     * @param map map to modify
     */
    void PlacePlayerResources(const MapSettings& settings, Map& map);

    /**
     * Create a elevation (hills) for the specified map.
     * @param settings settings used for map generation
     * @param config configuration for the random map generator
     * @param map map to modify
     */
    void CreateHills(const MapSettings& settings, Map& map);

    /**
     * Fill the remaining terrain (apart from the player positions) according to the generated hills.
     * @param settings settings used for map generation
     * @param config configuration for the random map generator
     * @param map map to modify
     */
    void FillRemainingTerrain(const MapSettings& settings, Map& map);

    /// Set the resources (water, fish, coal...) for the map
    void SetResources(const MapSettings& settings, Map& map);
};
