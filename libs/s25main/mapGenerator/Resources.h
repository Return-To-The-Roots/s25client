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

#ifndef Resources_h__
#define Resources_h__

#include "mapGenerator/RandomUtility.h"
#include "mapGenerator/MapSettings.h"
#include "mapGenerator/Map.h"

namespace rttr {
namespace mapGenerator {

/**
 * Checks whether or not the map position with the specified index is a harbor position or contains a player's head quarter.
 * @param map map to check for harbor position or head quarter
 * @param index index of the grid position to check
 * @returns true if there's either a harbor position or a HQ placed at the position with the specified index, false otherwise.
 */
bool IsHeadQuarterOrHarborPosition(const Map_& map, int index);

/**
 * Maps a particular texture type to a list of possible tree types to place on the specified texture.
 * @param texture texture to find appropriate trees for
 * @param mapping texture mapping to access texture related game data
 * @returns a list of tree type indices which can be used for the texture.
 */
std::vector<int> GetTreeTypes(const Texture& texture, const TextureMapping_& mapping);

/**
 * Maps a particular texture type to a list of possible tree object infos.
 * @param texture texture to find appropriate trees for
 * @param mapping texture mapping to access texture related game data
 * @returns a list of tree object indices which can be used for the texture.
 */
std::vector<int> GetTreeInfos(const Texture& texture, const TextureMapping_& mapping);

/**
 * Maps a particular texture type to a list of possible animals to place on the specified texture.
 * @param texture texture to find appropriate animals for
 * @param mapping texture mapping to access texture related game data
 * @returns a list of animal indices which can be used for the texture.
 */
Animals GetAnimals(const Texture& texture, const TextureMapping_& mapping);

/**
 * Computes the propability to place a tree on the specified texture, assuming a certain distance from any
 * free zone (e.g. harbor position or head quarter).
 * @param texture type of texture to place a tree on
 * @param mapping texture mapping to access texture related game data
 * @param freeZoneDistance distance to the nearest free zone
 * @returns probability (between 0 and 100) to place a tree on the specified texture.
 */
int GetTreeProbability(const Texture& texture, const TextureMapping_& mapping, int freeZoneDistance);

/**
 * Computes the propability to place granite blocks on the specified texture, assuming a certain distance from any
 * free zone (e.g. harbor position or head quarter).
 * @param texture type of texture to place a granite block on
 * @param mapping texture mapping to access texture related game data
 * @param freeZoneDistance distance to the nearest free zone
 * @returns probability (between 0 and 100) to place a granite block on the specified texture.
 */
int GetStoneProbability(const Texture& texture, const TextureMapping_& mapping, int freeZoneDistance);

/**
 * Computes the propability to place an animal on the specified texture.
 * @param texture type of texture to place an animal on
 * @param mapping texture mapping to access texture related game data
 * @returns probability (between 0 and 100) to place an animal on the specified texture.
 */
int GetAnimalProbability(const Texture& texture, const TextureMapping_& mapping);

/**
 * Places trees on the specified map.
 * @param map map to place trees on
 * @param rnd random number generator
 * @param mapping texture mapping to access texture related game data
 * @param freeZone distance for each grid position to the next "free zone" object (harbor position or HQ)
 */
void PlaceTrees(Map_& map,
                RandomUtility& rnd,
                const TextureMapping_& mapping,
                const std::vector<int>& freeZone);

/**
 * Places granite blocks on the specified map.
 * @param map map to place granite blocks on
 * @param rnd random number generator
 * @param mapping texture mapping to access texture related game data
 * @param freeZone distance for each grid position to the next "free zone" object (harbor position or HQ)
 */
void PlaceStones(Map_& map,
                 RandomUtility& rnd,
                 const TextureMapping_& mapping,
                 const std::vector<int>& freeZone);

/**
 * Places resources on mountains and fish in water.
 * @param map map to place mountain and water resources on
 * @param rnd random number generator
 * @param mapping texture mapping to access texture related game data
 * @param settings map settings specifying the proportion of mountain resources
 */
void PlaceMinesAndFish(Map_& map,
                       RandomUtility& rnd,
                       const TextureMapping_& mapping,
                       const MapSettings& settings);

/**
 * Places animals on the specified map.
 * @param map map to place animals on
 * @param rnd random number generator
 * @param mapping texture mapping to access texture related game data
 */
void PlaceAnimals(Map_& map, RandomUtility& rnd, const TextureMapping_& mapping);

/**
 * Places resources on the map. That includes trees, granite blocks, animals and mountain resources.
 * @param map map to cover with resources
 * @param rnd random number generator used for placing resources
 * @param mapping texture mapping to access texture related game data
 * @param settings map settings including information on resource distribution
 */
void PlaceResources(Map_& map,
                    RandomUtility& rnd,
                    const TextureMapping_& mapping,
                    const MapSettings& settings);

}}

#endif // Resources_h__
