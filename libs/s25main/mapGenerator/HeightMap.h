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

#ifndef HeightMap_h__
#define HeightMap_h__

#include "mapGenerator/RandomUtility.h"
#include "mapGenerator/Types.h"

#include <set>

namespace rttr {
namespace mapGenerator {

/**
 * Generates a new height map with random values between minimum and maximum height.
 * @param size size of the map
 * @param height absolute minimum and maximum height values
 * @param rnd random number generator
 * @return a new height map with random values.
 */
HeightMap CreateRandomHeightMap(const MapExtent& size, const Range& height, RandomUtility& rnd);

/**
 * Calculates the sea level assuming the sea should coverage a certain percentage of the map.
 * @param z height map for all nodes of the map
 * @param coverage a values between 0.0 (0%) and 1.0 (100%) defining the map coverage of the sea
 * @return proposed height value for the sea level based on the specified coverage.
 */
Height SeaLevel(const HeightMap& z, double coverage);

/**
 * Calculates a minimum height value to consider landscape as mountains.
 * @param z height map for all nodes of the map
 * @param coverage a values between 0.0 (0%) and 1.0 (100%) defining the map coverage of the mountain area
 * @return proposed minimum height value for mountain area.
 */
Height MountainLevel(const HeightMap& z, double coverage);

/**
 * Calculates a minimum height value to consider landscape as mountains within the specified area.
 * @param z height map for all nodes of the map
 * @param coverage a values between 0.0 (0%) and 1.0 (100%) defining the map coverage of the mountain area
 * @param subset area to consider for computing the minimum height value to consider landscape for mountains
 * @return proposed minimum height value for mountain area.
 */
Height MountainLevel(const HeightMap& z, double coverage, const std::set<int>& subset);

/**
 * Scales the height map so that the minimum height value equals the minimum overall height and the
 * maximum height value equals the maximum overall height.
 * @param z height map to scale
 * @param height minimum and maximum overall height values
 */
void ScaleToFit(HeightMap& z, const Range& height);

/**
 * Computes a height map from the specified distance map using the specified minimum and maximum height values
 * for scaling.
 * @param distance distance map to map to a height map
 * @param height minimum and maximum height values
 * @return a new height map generated from the specified distance map.
 */
HeightMap HeightFromDistance(const std::vector<int>& distance, const Range& height);

/**
 * Smooths the specified height map by averaging over height values within the specified radius.
 * @param iterations number of smoothing iterations over the whole map
 * @param radius maximum distance of nodes to consider for determining the new height values at a position
 * @param z height map to smooth
 * @param size overall map size
 */
void Smooth(int iterations, double radius, HeightMap& z, const MapExtent& size);

}}

#endif // HeightMap_h__
