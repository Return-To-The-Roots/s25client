// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "mapGenerator/Algorithms.h"
#include "helpers/mathFuncs.h"

namespace rttr::mapGenerator {

void updateDistances(NodeMapBase<unsigned>& distances, std::queue<MapPoint>& queue)
{
    while(!queue.empty())
    {
        const auto currentPoint = queue.front();
        const auto currentDistance = distances[currentPoint];

        queue.pop();

        const auto& neighbors = distances.getNeighbours(currentPoint);

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

void flattenForCastleBuilding(NodeMapBase<uint8_t>& heightMap, MapPoint pos)
{
    const auto& neighbors = heightMap.getNeighbours(pos);
    const auto& farNeighbors = heightMap.getPointsInRadius(pos, 2);

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

} // namespace rttr::mapGenerator
