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

#ifndef GridUtility_h__
#define GridUtility_h__

#include "gameTypes/MapCoordinates.h"
#include <vector>

namespace rttr {
namespace mapGenerator {

using Positions = std::vector<Position>;

/**
 * Computes the position of the specified index on the grid.
 * @param index index for the position
 * @param size size of the grid
 * @returns grid position calculated for the index based on the specified grid size.
 */
Position GridPosition(int index, const MapExtent& size);

// ToDo: use MakeMapPoint in "world/MapGeometry.h" instead
/**
 * Clamps the position according to the grid boundaries.
 * @param p grid position which might ly outside of the grid
 * @param size size of the grid
 * @returns clamped position within the grid, e.a. between 0 and size - 1 in each dimension.
 */
Position GridClamp(const Position& p, const MapExtent& size);

// ToDo: fix invalid computation due to grid representation
/**
 * Computes the delta (= positive or negative difference) between two positions in both dimensions.
 * @param p1 first position to calulate the distance for
 * @param p2 second position to calulate the distance for
 * @param size size of the grid
 * @returns the delta between both positions for all dimensions.
 */
Position GridDelta(const Position& p1, const Position& p2, const MapExtent& size);

// ToDo: fix invalid computation due to grid representation
/**
 * Computes the distance between two positions on a grid.
 * @param p1 first position to calulate the distance for
 * @param p2 second position to calulate the distance for
 * @param size size of the grid
 * @returns the absolute distance between both positions.
 */
double GridDistance(const Position& p1, const Position& p2, const MapExtent& size);

// ToDo: fix invalid computation due to grid representation
/**
 * Computes the normalized distance (minimum 0 and maximum 1) between two positions on a grid.
 * @param p1 first position to calulate the distance for
 * @param p2 second position to calulate the distance for
 * @param size size of the grid
 * @returns the normalized distance between both positions.
 */
double GridDistanceNorm(const Position& p1, const Position& p2, const MapExtent& size);

/**
 * Generates a vector of all positions for a grid of the specified size.
 * @param size size of the grid
 * @returns a vector containing all grid positions.
 */
Positions GridPositions(const MapExtent& size);

// ToDo: fix invalid computation due to grid representation
/**
 * Computes all neighbor positions of the specified grid position.
 * @param p position to compute neighbor positions for
 * @param size size of the grid
 * @returns all neighbors for the specified position.
 */
Positions GridNeighbors(const Position& p, const MapExtent& size);

// ToDo: fix invalid computation due to grid representation
// ToDo: use MapBase::GetPointsInRadiusWithCenter in "world/MapBase.h" instead
/**
 * Collects all positions of a grid within a radius around the specified position.
 * @param p center point for the collection radius
 * @param size size of the grid
 * @param distance search radius for collecting grid positions
 * @returns all positions within a radius around the specified point
 */
Positions GridCollect(const Position& p, const MapExtent& size, double distance);

/**
 * Collects all positions with the specified property by starting and extending the search from the specified point.
 * The algorithm will search only in the neighborhood of the starting position or any already visited positions
 * the property applies to.
 * @param p starting position for collecting positions
 * @param size size of the grid
 * @param property index based storage of whether or not the property applies for a particular position
 * @returns all positions in the neighborhood of the starting position where the property applies.
 */
Positions GridCollect(const Position& p, const MapExtent& size, const std::vector<bool>& property);

// ToDo: check if required & correct (invalid grid representation)
/**
 * Computes all grid neighbors for a RSU triangle.
 * @param p position of the RSU triangle
 * @param size size of the grid
 * @returns all positions of neighboring LSD triangles for the specified RSU triangle.
 */
Positions GridNeighborsOfRsuTriangle(const Position& p, const MapExtent& size);

// ToDo: check if required & correct (invalid grid representation)
/**
 * Computes all grid neighbors for a LSD triangle.
 * @param p position of the LSD triangle
 * @param size size of the grid
 * @returns all positions of neighboring RSU triangles for the specified LSD triangle.
 */
Positions GridNeighborsOfLsdTriangle(const Position& p, const MapExtent& size);

}}

#endif // GridUtility_h__
