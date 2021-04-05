// Copyright (c) 2017 - 2020 Settlers Freaks (sf-team at siedler25.org)
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

#include "mapGenerator/Algorithms.h"
#include "helpers/mathFuncs.h"

namespace rttr { namespace mapGenerator {

    void UpdateDistances(NodeMapBase<unsigned>& distances, std::queue<MapPoint>& queue)
    {
        while(!queue.empty())
        {
            const auto currentPoint = queue.front();
            const auto currentDistance = distances[currentPoint];

            queue.pop();

            const auto& neighbors = distances.GetNeighbours(currentPoint);

            for(const MapPoint& neighbor : neighbors)
            {
                if(distances[neighbor] > 0)
                {
                    if(distances[neighbor] == unsigned(-1))
                    {
                        queue.push(neighbor);
                    }

                    distances[neighbor] = std::min(distances[neighbor], currentDistance + 1);
                }
            }
        }
    }

    void FlattenForCastleBuilding(NodeMapBase<uint8_t>& heightMap, MapPoint pos)
    {
        const auto& neighbors = heightMap.GetNeighbours(pos);
        const auto& farNeighbors = heightMap.GetPointsInRadius(pos, 2);

        const auto compareHeight = [&heightMap](const MapPoint& p1, const MapPoint& p2) {
            return heightMap[p1] < heightMap[p2];
        };

        const auto lowestPoint = *std::min_element(neighbors.begin(), neighbors.end(), compareHeight);
        const auto lowestHeight = std::min(heightMap[pos], heightMap[lowestPoint]);

        for(const MapPoint& farNeighbor : farNeighbors)
        {
            // Max diff of 2
            heightMap[farNeighbor] =
              helpers::clamp(heightMap[farNeighbor], std::max(0, lowestHeight - 2), lowestHeight + 2);
        }

        heightMap[pos] = lowestHeight;
        SetValues(heightMap, neighbors, lowestHeight);
    }

}} // namespace rttr::mapGenerator
