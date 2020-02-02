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

#ifndef DistanceField_h__
#define DistanceField_h__

#include "mapGenerator/Map.h"
#include "mapGenerator/GridUtility.h"
#include "gameTypes/MapCoordinates.h"
#include "world/MapGeometry.h"

#include <queue>
#include <set>
#include <map>
#include <cmath>

namespace rttr {
namespace mapGenerator {

/**
 * Evalutor to detect head quarter positions on a map.
 * @param map reference to the map
 * @param index index of the position to check
 * @return 'true' if the position with the specified index is a HQ, 'false' otherwise.
 */
bool IsHeadQuarter(const Map_& map, int index);

// ToDo: check usage and update computation
/**
 * Evalutor to detect water area on a map.
 * @param map reference to the map
 * @param mapping texture mapping to access texture related game data
 * @param index index of the position to check
 * @return 'true' if the position with the specified index is water, 'false' otherwise.
 */
bool IsWater (const Map_& map, const TextureMapping_& mapping, int index);

// ToDo: check usage and update computation
/**
 * A position on an island is considered as coastland when any neighboring position contains water.
 * @param map reference to the map
 * @param mapping texture mapping to access texture related game data
 * @param index index of the position to check
 * @return 'true' if the position with the specified index is part of a coast, 'false' otherwise.
 */
bool IsCoastLand(const Map_& map, const TextureMapping_& mapping, int index);

// ToDo: check usage - neighbors for nodes or textures? possibly update computation
/**
 * Computes a vector field describing the distance of each grid position (by index) to the closest position
 * for which the evaluator returned true.
 * @param size absolute size of the current map in x- and y-direction
 * @param evaluator function used evaluate whether or not the property is given for an index of a position
 * @return distance of each grid position to closest point with the evaluated property being true.
 */
template<typename T>
std::vector<int> DistanceByProperty(const MapExtent& size, T&& evaluator)
{
    const int indices = static_cast<int>(size.x * size.y);
    const int maximumDistance = size.x + size.y;
    
    // initialize all distances with maximum distance based on the size of the map
    std::vector<int> distance(indices, maximumDistance);
    std::queue<int> queue;
    
    // initialize distances for positions where the property applies with "0"
    // and add them to the queue for further processing ...
    for (int index = 0; index < indices; index++)
    {
        if (evaluator(index))
        {
            distance[index] = 0;
            queue.push(index);
        }
    }
    
    while (!queue.empty())
    {
        const int currentIndex = queue.front();
        const int currentDistance = distance[currentIndex];
        
        queue.pop();
        
        const auto position = GridPosition(currentIndex, size);
        const auto neighbors = GridNeighbors(position, size);
        
        for (Position neighbor: neighbors)
        {
            const int neighborIndex = neighbor.x + neighbor.y * size.x;
            
            if (distance[neighborIndex] > 0)
            {
                if (distance[neighborIndex] == maximumDistance)
                {
                    queue.push(neighborIndex);
                }
                
                distance[neighborIndex] = std::min(distance[neighborIndex], currentDistance + 1);
            }
        }
    }

    return distance;
}

// ToDo: check usage - neighbors for nodes or textures? possibly update computation
/**
 * Computes a vector field describing the distance of each grid position (by index) to the closest position
 * for which the evaluator returned true. This field is only being computed for the specified subset of positions.
 * @param subset subset of positions to calulate the distance for
 * @param size absolute size of the current map in x- and y-direction
 * @param evaluator function used evaluate whether or not the property is given
 * @return distance for each position of the subset to closest point with the evaluated property being true.
 */
template<typename T>
std::vector<int> DistanceByProperty(const std::vector<Position>& subset,
                                    const MapExtent& size,
                                    T&& evaluator)
{
    const int indices = static_cast<int>(subset.size());
    const int maximumDistance = size.x + size.y;
    
    // initialize all distances with maximum distance based on the size of the map
    std::vector<int> distance(indices, maximumDistance);
    std::queue<int> queue;

    // stores a mapping of map indices to subset indices
    std::map<int, int> subsetIndexMap;

    // initialize distances for positions where the property applies with "0"
    // and add them to the queue for further processing ...
    for (int i = 0; i < indices; i++)
    {
        const int index = subset[i].x + subset[i].y * size.x;
        subsetIndexMap[index] = i; // map position to subset vector index

        if (evaluator(index))
        {
            distance[i] = 0;
            queue.push(i);
        }
    }
    
    while (!queue.empty())
    {
        const int currentIndex = queue.front();
        const int currentDistance = distance[currentIndex];
        
        queue.pop();
        
        const auto position = subset[currentIndex];
        const auto neighbors = GridNeighbors(position, size);
        
        for (Position neighbor: neighbors)
        {
            const int neighborIndex = neighbor.x + neighbor.y * size.x;
            
            auto itSubsetIndex = subsetIndexMap.find(neighborIndex);
            if ( itSubsetIndex != subsetIndexMap.end())
            {
                int subsetIndex = itSubsetIndex->second;
                if (distance[subsetIndex] > 0)
                {
                    if (distance[subsetIndex] == maximumDistance)
                    {
                        queue.push(subsetIndex);
                    }

                    distance[subsetIndex] = std::min(distance[subsetIndex], currentDistance + 1);
                }
            }
        }
    }
    
    return distance;
}

}}

#endif // DistanceField_h__
