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

#ifndef RandomMap_h__
#define RandomMap_h__

#include "mapGenerator/MapSettings.h"
#include "mapGenerator/Map.h"
#include "mapGenerator/RandomUtility.h"

namespace rttr {
namespace mapGenerator {

/**
 * Smoothes the height map with respect to the size of the map and scales the height map to cover the whole
 * range of height values.
 * @param map reference to the map to post process
 */
void PostProcessHeightMap(Map_& map);

/**
 * Improves the texturing of the map by smoothing texture transitions, fixing sharp edges and removing
 * mountain transitions without mountains.
 * @param map reference to the map to post process
 * @param mapping texture mapping instance to access texture related game data
 */
void PostProcessTextures(Map_& map, TextureMapping_& mapping);

/**
 * Creates a new random map with "continent" style.
 * @param rnd random number generator
 * @param map reference to the empty map
 * @param mapping texture mapping instance to access texture related game data
 */
void CreateContinentMap(RandomUtility& rnd, Map_& map, TextureMapping_& mapping);

/**
 * Creates a new random map with "edgecase" style.
 * @param rnd random number generator
 * @param map reference to the empty map
 * @param mapping texture mapping instance to access texture related game data
 */
void CreateEdgecaseMap(RandomUtility& rnd, Map_& map, TextureMapping_& mapping);

/**
 * Creates a new random map with "migration" style.
 * @param rnd random number generator
 * @param map reference to the empty map
 * @param mapping texture mapping instance to access texture related game data
 */
void CreateMigrationMap(RandomUtility& rnd, Map_& map, TextureMapping_& mapping);

/**
 * Creates a new random map with "rivers" style.
 * @param rnd random number generator
 * @param map reference to the empty map
 * @param mapping texture mapping instance to access texture related game data
 */
void CreateRiversMap(RandomUtility& rnd, Map_& map, TextureMapping_& mapping);

/**
 * Creates a new random map with "crazy" (entirely random) style.
 * @param rnd random number generator
 * @param map reference to the empty map
 * @param mapping texture mapping instance to access texture related game data
 */
void CreateCrazyMap(RandomUtility& rnd, Map_& map, TextureMapping_& mapping);

/**
 * Creates a new random map with the specified settings and save it at the given file path.
 * @param filePath path to the output file for the generated random map
 * @param settings parameters for generating the random map
 */
void CreateRandomMap(const std::string& filePath, const MapSettings& settings);

}}

#endif // RandomMap_h__
