// Copyright (c) 2017 - 2017 Settlers Freaks (sf-team at siedler25.org)
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

namespace rttr { namespace mapGenerator {

    void UpdateDistances(ValueMap<unsigned>& distances, std::queue<MapPoint>& queue)
    {
        const unsigned maximumDistance = distances.GetWidth() * distances.GetHeight();

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
                    if(distances[neighbor] == maximumDistance)
                    {
                        queue.push(neighbor);
                    }

                    distances[neighbor] = std::min(distances[neighbor], currentDistance + 1);
                }
            }
        }
    }

}} // namespace rttr::mapGenerator
