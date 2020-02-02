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

#ifndef HeadQuarters_h__
#define HeadQuarters_h__

#include "mapGenerator/Map.h"
#include "mapGenerator/RandomUtility.h"

namespace rttr {
namespace mapGenerator {

// NEW WORLD

/**
 * Finds the most suitable position for a HQ in the specified area of the map.
 * To find the most suitable position for the entire map just leave the area empty.
 * The resulting HQ positions are sorted by quality (highest quality first).
 * @param map map to search for suitable HQ positions
 * @param area area within the HQ position should be
 * @return all suitable HQ positions witihn the specified area (or the entire map if the area is empty).
 */
std::vector<MapPoint> FindHqPositions(const Map& map, const std::vector<MapPoint>& area);

/**
 * Tries to place a number of headquarters on the specified map.
 * @param map map to place head quarters for all players on
 * @param rnd random number generated used for retrying HQ placement on failures
 * @param number number of HQs to place - equal to the number of players
 * @param retries number of retries to place valid HQs on this map
 * @return false if no valid HQ position was found for at least one player, true otherwise
 */
bool PlaceHeadQuarters(Map& map, RandomUtility& rnd, int number, int retries = 10);

/**
 * Tries to place a head quarter (HQ) for a single player within the specified area.
 * @param map reference to the map to place the HQ on
 * @param index player index for the HQ
 * @param area area to place the HQ in
 * @returns true if a HQ was placed fro the specified player, false otherwise.
 */
bool PlaceHeadQuarter(Map& map, int index, const std::vector<MapPoint>& area);

// OLD WORLD

/**
 * Finds the most suitable position for a HQ in the specified area of the map.
 * @param map map to search of suitable HQ position for
 * @param mapping texture mapping for accessing texture related game data
 * @param area area within the HQ position should be
 * @return most suitable HQ position witihn the specified area
 */
Position FindHeadQuarterPosition(const Map_& map,
                                 const TextureMapping_& mapping,
                                 const std::vector<Position>& area);

/**
 * Resets all HQ positions.
 * @param map reference to the map to remove HQ positions from
 */
void ResetHeadQuarters(Map_& map);

/**
 * Places a number of headquarters.
 * @param map map to place the HQs on
 * @param mapping texture mapping for accessing texture related game data
 * @param rnd random number generator
 * @param number number of HQs to place - equal to the number of players
 * @param retries number of retries to place valid HQs on this map
 * @return false if no valid HQ position was found for at least one player, true otherwise
 */
bool PlaceHeadQuarters(Map_& map,
                       TextureMapping_& mapping,
                       RandomUtility& rnd,
                       int number,
                       int retries = 10);

/**
 * Places the header quater for a single player within the specified area.
 * @param map map to place the HQ on
 * @param mapping texture mapping for accessing texture related game data
 * @param index index of the HQ which correlates with the player number
 * @param area area of possible HQ positions
 * @return false if no valid HQ position was found, true otherwise
 */
bool PlaceHeadQuarter(Map_& map,
                      TextureMapping_& mapping,
                      int index,
                      const std::vector<Position>& area);

}}

#endif // HeadQuarters_h__
