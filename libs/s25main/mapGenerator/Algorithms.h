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

#ifndef Algorithms_h__
#define Algorithms_h__

#include "rttrDefines.h"
#include "mapGenerator/ValueMap.h"
#include "world/NodeMapBase.h"

#include <set>
#include <queue>

namespace rttr {
namespace mapGenerator {

/**
 * Compare operator for MapPoint class.
 */
struct MapPoint_compare {
    bool operator() (const MapPoint& p1, const MapPoint& p2) const {
        return (p1.y < p2.y) || (p1.y == p2.y && p1.x < p2.x);
    }
};

/**
 * Smoothes the specified nodes with a smoothing kernel of the specified extent (radius).
 * @param iteration number of times to apply smoothing kernel to every node
 * @param radius extent of the smoothing kernel
 * @param nodes map of node values
 */
template<typename T_Node>
void Smooth(unsigned iterations, unsigned radius, NodeMapBase<T_Node>& nodes)
{
    for (unsigned i = 0; i < iterations; ++i)
    {
        RTTR_FOREACH_PT(MapPoint, nodes.GetSize())
        {
            auto points = nodes.GetPointsInRadius(pt, radius);
            auto sum = nodes[pt];

            for (MapPoint p : points)
            {
                sum += nodes[p];
            }

            nodes[pt] = sum / (points.size() + 1);
        }
    }
}

/**
 * Maps the values to the specified range [minimum, maximum].
 * @param values map of  comparable values
 * @param minimum minimum value to map any values to
 * @param maximum maximum value to map any values to
 */
template<typename T_Value>
void Scale(ValueMap<T_Value>& values, T_Value minimum, T_Value maximum)
{
    auto range = values.GetRange();
    auto actualRange = range.GetDifference();
    auto actualMinimum = range.minimum;
    
    if (actualRange == 0)
    {
        return;
    }
    
    auto scaledRange = maximum - minimum;
    
    RTTR_FOREACH_PT(MapPoint, values.GetSize())
    {
        auto normalizer = static_cast<double>(values[pt] - actualMinimum) / actualRange;
        auto offset = round(normalizer * scaledRange);
        
        values[pt] = static_cast<T_Value>(minimum + offset);
    }
}

/**
 * Collects all map points around the specified point for which the evaluator returns 'true'. The function recursively checks
 * neighbors of neighbors but only collects positively evaluated points. Whenever it hits a negative value is stops searching
 * the neighborhood of this specific point.
 * @param map reference to the map to collect map points from
 * @param pt starting point which has to be evaluated with 'true' or and empty vector will be returned
 * @param evaluator evaluator function which returns 'true' or 'false' for any map point
 * @returns a list of map points where every point is connected to at least one other point of the least which has also
 * been evaluated positively.
 */
template<typename T_Node>
std::vector<MapPoint> Collect(const MapBase& map, const MapPoint& pt, T_Node&& evaluator)
{
    std::set<MapPoint, MapPoint_compare> visited;
    std::vector<MapPoint> body;
    std::queue<MapPoint> searchSpace;
    
    searchSpace.push(pt);
    
    while(!searchSpace.empty())
    {
        MapPoint currentPoint = searchSpace.front();
        searchSpace.pop();

        if (evaluator(currentPoint))
        {
            if (visited.insert(currentPoint).second)
            {
                body.push_back(currentPoint);
                
                auto neighbors = map.GetNeighbours(currentPoint);
                for (MapPoint neighbor : neighbors)
                {
                    searchSpace.push(neighbor);
                }
            }
        }
    }

    return body;
}

/**
 * Computes a map of distance values describing the distance of each grid position to the closest position
 * for which the evaluator returned `true`.
 * @param size size of  the map
 * @param evaluator evaluator function takes a MapPoint as input and returns `true` or `false`
 * @return distance of each grid position to closest point which has been evaluted with `true`.
 */
template<typename T_Value>
ValueMap<unsigned> Distances(const MapExtent& size, T_Value&& evaluator)
{
    const unsigned maximumDistance = size.x + size.y;

    std::queue<MapPoint> queue;
    ValueMap<unsigned> distances;

    distances.Resize(size, maximumDistance);
    
    RTTR_FOREACH_PT(MapPoint, size)
    {
        if (evaluator(pt))
        {
            distances[pt] = 0;
            queue.push(pt);
        }
    }
    
    while (!queue.empty())
    {
        const auto currentPoint = queue.front();
        const auto currentDistance = distances[currentPoint];
        
        queue.pop();
        
        const auto& neighbors = distances.GetNeighbours(currentPoint);
        
        for (MapPoint neighbor : neighbors)
        {
            if (distances[neighbor] > 0)
            {
                if (distances[neighbor] == maximumDistance)
                {
                    queue.push(neighbor);
                }
                
                distances[neighbor] = std::min(distances[neighbor], currentDistance + 1);
            }
        }
    }

    return distances;
}

/**
 * Computes an upper limit for the specified values. The number of values between the specified minimum and
 * the computed limit is at least as high as the specified coverage of the map.
 * @param values map of comparable values
 * @param coverage percentage of expected map coverage (value between 0 and 1)
 * @param minimum minimum value to consider
 * @returns a value between the specified minimum and the maximum value of the map.
 */
template<typename T_Value>
T_Value LimitFor(const ValueMap<T_Value>& values, float coverage, T_Value minimum)
{
    if (coverage < 0 || coverage > 1)
    {
        throw std::invalid_argument("coverage must be between 0 and 1");
    }
    
    const T_Value maximum = values.GetMaximum();
    const MapExtent size = values.GetSize();
    
    const unsigned expectedNodes = static_cast<unsigned>(coverage * size.x * size.y);

    unsigned currentNodes = 0;
    
    T_Value limit = minimum;
    
    while (currentNodes < expectedNodes && limit <= maximum)
    {
        currentNodes = 0;

        RTTR_FOREACH_PT(MapPoint, size)
        {
            if (values[pt] >= minimum && values[pt] <= limit)
            {
                currentNodes++;
            }
        }
        
        limit++;
    }
    
    return limit;
}

}}

#endif // Algorithms_h__
