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

#include "RttrForeachPt.h"
#include "mapGenerator/NodeMapUtilities.h"
#include "world/NodeMapBase.h"

#include <cmath>
#include <queue>
#include <set>
#include <stdexcept>

namespace rttr { namespace mapGenerator {

    /**
     * Smoothes the specified nodes with a smoothing kernel of the specified extent (radius).
     *
     * @param iteration number of times to apply smoothing kernel to every node
     * @param radius extent of the smoothing kernel
     * @param nodes map of node values
     */
    template<typename T>
    void Smooth(unsigned iterations, unsigned radius, NodeMapBase<T>& nodes)
    {
        const MapExtent& size = nodes.GetSize();
        std::vector<std::vector<MapPoint>> neighbors(size.x * size.y);

        RTTR_FOREACH_PT(MapPoint, size)
        {
            neighbors[nodes.GetIdx(pt)] = nodes.GetPointsInRadius(pt, radius);
        }

        for(unsigned i = 0; i < iterations; ++i)
        {
            RTTR_FOREACH_PT(MapPoint, size)
            {
                int sum = static_cast<int>(nodes[pt]);
                const auto& neighborPoints = neighbors[nodes.GetIdx(pt)];

                for(const MapPoint& p : neighborPoints)
                {
                    sum += static_cast<int>(nodes[p]);
                }

                nodes[pt] = static_cast<T>(round(static_cast<double>(sum) / (neighborPoints.size() + 1)));
            }
        }
    }

    /**
     * Maps the values to the specified range [minimum, maximum].
     *
     * @param values map of  comparable values
     * @param minimum minimum value to map any values to
     * @param maximum maximum value to map any values to
     */
    template<typename T>
    void Scale(NodeMapBase<T>& values, T minimum, T maximum)
    {
        auto range = GetRange(values);
        auto actualRange = range.GetDifference();
        auto actualMinimum = range.minimum;

        if(actualRange == 0)
        {
            return;
        }

        auto scaledRange = maximum - minimum;

        RTTR_FOREACH_PT(MapPoint, values.GetSize())
        {
            auto normalizer = static_cast<double>(values[pt] - actualMinimum) / actualRange;
            auto offset = round(normalizer * scaledRange);

            values[pt] = static_cast<T>(minimum + offset);
        }
    }

    /**
     * Collects all map points around the specified point for which the evaluator returns 'true'. The function
     * recursively checks neighbors of neighbors but only collects positively evaluated points. Whenever it hits a
     * negative value is stops searching the neighborhood of this specific point. The underlying implementation is
     * breadth-first search.
     *
     * @param map reference to the map to collect map points from
     * @param pt starting point which has to be evaluated with 'true' or and empty vector will be returned
     * @param evaluator evaluator function which returns 'true' or 'false' for any map point
     *
     * @returns a list of map points where every point is connected to at least one other point of the least which
     * has also been evaluated positively.
     */
    template<typename T>
    std::vector<MapPoint> Collect(const MapBase& map, const MapPoint& pt, T&& evaluator)
    {
        std::set<MapPoint, MapPointLess> visited;
        std::vector<MapPoint> body;
        std::queue<MapPoint> searchSpace;

        searchSpace.push(pt);

        while(!searchSpace.empty())
        {
            MapPoint currentPoint = searchSpace.front();
            searchSpace.pop();

            if(evaluator(currentPoint))
            {
                if(visited.insert(currentPoint).second)
                {
                    body.push_back(currentPoint);

                    const auto& neighbors = map.GetNeighbours(currentPoint);
                    for(MapPoint neighbor : neighbors)
                    {
                        searchSpace.push(neighbor);
                    }
                }
            }
        }

        return body;
    }

    /**
     * Updates the specified distance values to the values initially contained by the queue. The queue is being
     * modified throughout the process for performance reasons.
     *
     * @param distances distance map which is being updated
     * @param queue queue with initial elements to used for distance computation
     */
    void UpdateDistances(NodeMapBase<unsigned>& distances, std::queue<MapPoint>& queue);

    /**
     * Computes a map of distance values describing the distance of each grid position to the closest flagged point.
     *
     * @param flaggedPoints contains all flagged points
     * @param size size of  the map
     *
     * @return distance of each grid position to closest flagged point.
     */
    template<class T_Container>
    NodeMapBase<unsigned> DistancesTo(const T_Container& flaggedPoints, const MapExtent& size)
    {
        const unsigned maximumDistance = size.x * size.y;
        std::queue<MapPoint> queue;
        NodeMapBase<unsigned> distances;
        distances.Resize(size, maximumDistance);

        for(const MapPoint& pt : flaggedPoints)
        {
            distances[pt] = 0;
            queue.push(pt);
        }

        UpdateDistances(distances, queue);

        return distances;
    }

    /**
     * Computes a map of distance values describing the distance of each grid position to the closest position for
     * which the evaluator returned `true`. The computation takes place only within the specified area - points outside
     * the area are set to a maximum value of width + height of the map.
     *
     * @param size size of  the map
     * @param area area to compute distance values for
     * @param defaultValue default distance value applied to points outside of the specified area
     * @param evaluator evaluator function takes a MapPoint as input and returns `true` or `false`
     *
     * @return distance of each grid position to closest point which has been evaluted with `true`.
     */
    template<typename T, class T_Container>
    NodeMapBase<unsigned> Distances(const MapExtent& size, const T_Container& area, const unsigned defaultValue,
                                    T&& evaluator)
    {
        const unsigned maximumDistance = size.x * size.y;

        std::queue<MapPoint> queue;
        NodeMapBase<unsigned> distances;
        distances.Resize(size, defaultValue);

        for(const MapPoint& pt : area)
        {
            if(evaluator(pt))
            {
                distances[pt] = 0;
                queue.push(pt);
            } else
            {
                distances[pt] = maximumDistance;
            }
        }

        UpdateDistances(distances, queue);

        return distances;
    }

    /**
     * Computes a map of distance values describing the distance of each grid position to the closest position for
     * which the evaluator returned `true`.
     *
     * @param size size of  the map
     * @param evaluator evaluator function takes a MapPoint as input and returns `true` or `false`
     *
     * @return distance of each grid position to closest point which has been evaluted with `true`.
     */
    template<typename T_Value>
    NodeMapBase<unsigned> Distances(const MapExtent& size, T_Value&& evaluator)
    {
        std::vector<MapPoint> flaggedPoints;
        RTTR_FOREACH_PT(MapPoint, size)
        {
            if(evaluator(pt))
            {
                flaggedPoints.push_back(pt);
            }
        }
        return DistancesTo(flaggedPoints, size);
    }

    /**
     * Computes an upper limit for the specified values. The number of values between the specified minimum and the
     * computed limit is at least as high as the specified coverage of the map.
     *
     * @param values map of comparable values
     * @param coverage percentage of expected map coverage (value between 0 and 1)
     * @param minimum minimum value to consider
     * @param maximum maximum value to consider
     *
     * @returns a value between the specified minimum and the maximum value of the map.
     */
    template<typename T>
    T LimitFor(const NodeMapBase<T>& values, double coverage, T minimum)
    {
        if(coverage < 0 || coverage > 1)
        {
            throw std::invalid_argument("coverage must be between 0 and 1");
        }

        const T maximum = *std::max_element(values.begin(), values.end());

        if(minimum == maximum)
        {
            return maximum;
        }

        const auto nodes = values.GetWidth() * values.GetHeight();
        const auto expectedNodes = static_cast<unsigned>(coverage * nodes);

        unsigned currentNodes = 0;
        unsigned previousNodes = 0;

        T limit = minimum;

        while(currentNodes < expectedNodes && limit <= maximum)
        {
            previousNodes = currentNodes;
            currentNodes =
              static_cast<unsigned>(std::count_if(values.begin(), values.end(), [minimum, limit](const T value) {
                  return value >= minimum && value <= limit;
              }));
            limit++;
        }

        if(expectedNodes - previousNodes < currentNodes - expectedNodes)
        {
            return limit - 2;
        }

        return limit - 1;
    }

    /**
     * Computes an upper limit for the specified values. The number of values between the specified minimum and the
     * computed limit is at least as high as the specified coverage of the map.
     *
     * @param values map of comparable values
     * @param area area of nodes to consider
     * @param coverage percentage of expected map coverage (value between 0 and 1)
     * @param minimum minimum value to consider
     * @param maximum maximum value to consider
     *
     * @returns a value between the specified minimum and the maximum value of the map.
     */
    template<typename T, class T_Area>
    T LimitFor(const NodeMapBase<T>& values, const T_Area& area, double coverage, T minimum)
    {
        if(coverage < 0 || coverage > 1)
        {
            throw std::invalid_argument("coverage must be between 0 and 1");
        }

        const T maximum = GetMaximum(values, area);

        if(minimum >= maximum)
        {
            return maximum;
        }

        const auto nodes = area.empty() ? values.GetWidth() * values.GetHeight() : area.size();
        const auto expectedNodes = static_cast<unsigned>(coverage * nodes);

        unsigned currentNodes = 0;
        unsigned previousNodes = 0;

        T limit = minimum;

        while(currentNodes < expectedNodes && limit <= maximum)
        {
            previousNodes = currentNodes;
            currentNodes = static_cast<unsigned>(
              std::count_if(area.begin(), area.end(), [&values, minimum, limit](const MapPoint& pt) {
                  return values[pt] >= minimum && values[pt] <= limit;
              }));
            limit++;
        }

        if(expectedNodes - previousNodes < currentNodes - expectedNodes)
        {
            return limit - 2;
        }

        return limit - 1;
    }

}} // namespace rttr::mapGenerator
