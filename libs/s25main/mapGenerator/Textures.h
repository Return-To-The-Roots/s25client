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

#ifndef Textures_h__
#define Textures_h__

#include "mapGenerator/Map.h"

namespace rttr {
namespace mapGenerator {

// NEW WORLD

/**
 * Increases height values of mountains to get steeper mountains.
 * @param map map to increase height of mountain for
 */
void IncreaseMountains(Map& map);


// OLD WORLD


/**
 * Checks whether or not the specified position is surounded by water within a 4-tile
 * radius. If there's less than 40% water, the specified position can be considered non-
 * coastline, because it's either near a tiny river or just not close to water at all.
 * @param position position to check for water neighborhood
 * @param water vectors of boolean values specifying whether or not a position for
 * the specified index is covered by water or not.
 * @param size size of the map
 * @return true if at least 40% of positions within a 4-tile radius are covered by water, false otherwise.
 */
bool Has40PercentWaterTilesInNeighborhood(const Position& position,
                                          const WaterMap& water,
                                          const MapExtent& size); // NOT REQUIRED

/**
 * Computes the coastline for the speicifed island.
 * @param island all positions covered by the island
 * @param water vectors of boolean values specifying whether or not a position for
 * the specified index is covered by water or not.
 * @param size size of the map
 * @return the coastline of the island.
 */
Coast FindCoast(const Island& island, const WaterMap& water, const MapExtent& size); // NOT REQUIRED

// ToDo: interpolation between z-values of triangle corners
/**
 * Creates textures for the specified map based on the "z" values of the map.
 * @param map reference to the map to apply the textures for
 * @param mapping utility to map height values to textures
 */
void CreateTextures(Map_& map, TextureMapping_& mapping);

/**
 * Replaces textures nearby water to generate smooth coast landscape.
 * @param map reference to the map to alter
 * @param mapping texture mapping to access texture related game data
 */
void AddCoastTextures(Map_& map, TextureMapping_& mapping);

/**
 * Replaces textures nearby mountains to generate smooth transition between grass and mountain.
 * @param map reference to the map to alter
 * @param mapping texture mapping to access texture related game data
 */
void AddMountainTransition(Map_& map, TextureMapping_& mapping);

/**
 * Smoothes rough edges between different textures on the map.
 * @param map reference to the map
 * @param mapping utility to map height values to textures
 * @param ignoreWater whether or not to ignore water textures for smoothing
 */
void SmoothTextures(Map_&, TextureMapping_& mapping, bool ignoreWater = false);

/**
 * Increases height values of mountains to get steeper mountains.
 * @param map map to increase height of mountain for
 * @param mapping texture mapping to access texture related game data
 */
void IncreaseMountains(Map_& map, TextureMapping_& mapping); // DONE

}}

#endif // Textures_h__
