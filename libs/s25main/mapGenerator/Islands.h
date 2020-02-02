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
#include "mapGenerator/Tile.h"

namespace rttr {
namespace mapGenerator {

using IslandCoverage = std::set<Tile, TileCompare>;

/**
 * Creates a new water map for the specified height map and sea level.
 * @param heightMap height map used to calculate the water map from
 * @param seaLevel maximum height the sea can reach
 * @returns a water map based on the specified height map and sea level.
 */
WaterMap CreateWaterMap(HeightMap& heightMap, Height seaLevel);

/**
 * Creates a new water map for the specified map based on its textures.
 * @param map map to generate a water map for
 * @param mapping texture mapping to access texture related game data
 * @returns a water map based on the textures of the specified map.
 */
WaterMap CreateWaterMap(const Map_& map, TextureMapping_& mapping);

/**
 * Find the next not-exlcuded position on the grid.
 * @param excludedPositions positions excluded from the search stored by position index
 * @param size size of the grid
 * @returns the next position which is not excluded from the search.
 */
Position FindNextPosition(const std::vector<bool>& excludedPositions, const MapExtent& size);

/**
 * Finds and returns all islands based on the specified water map.
 * @param waterMap specifies which positions on the grid are water and land
 * @param size size of the grid
 * @param minimumTilesPerIsland minimum number of tiles covered by land to consider it an island
 * @returns all islands on the map which are matching the specified criteria.
 */
Islands FindIslands(const std::vector<bool>& waterMap, const MapExtent& size, unsigned minimumTilesPerIsland);

/**
 * Finds the next, most suitable location in the sea to place an island on.
 * @param map pointer to the actual map to find a suitable island position for
 * @param landDistance distance of each map position to the next land mass
 * @returns the most suitable position for placing an island.
 */
Position FindSuitableIslandLocation(const Map_& map, const std::vector<int>& landDistance);

/**
 * Generates an island on the specified map which covers the specified positions.
 * @param coverage textures covered by the island
 * @param map map to place the island on
 * @param mapping texture mapping to access texture related game data
 */
void PlaceIsland(const IslandCoverage& coverage,
                 Map_& map,
                 TextureMapping_& mapping);

/**
 * Creates  a new island at the most suitable position on the specified map.
 * @param islandSize size of the island
 * @param islandLength length of the island (same unit as islandSize)
 * @param distanceToLand minimum distance of the island to other land area masses
 * @param map map to place the new island on
 * @param mapping texture mapping to access texture related game data
 */
Island CreateIsland(RandomUtility& rnd,
                    int islandSize,
                    int islandLength,
                    int distanceToLand,
                    Map_& map,
                    TextureMapping_& mapping);

}}

#endif // Islands_h__
