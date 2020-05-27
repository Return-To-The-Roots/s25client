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

#include "rttrDefines.h" // IWYU pragma: keep
#include "randomMaps/algorithm/DistanceField.h"
#include "randomMaps/algorithm/GridUtility.h"

#include <queue>
#include <set>
#include <map>
#include <cmath>

DistanceField::DistanceField(bool (*hasProperty)(const Map* map, int index)) : hasProperty_(hasProperty)
{
    
}

std::vector<int> DistanceField::Compute(const Map* map)
{
    return Compute(map, GridUtility::Positions(map->size));
}

std::vector<int> DistanceField::Compute(const Map* map, const std::vector<Position>& subset)
{
    auto size = map->size;
    auto indices = (int)subset.size();
    
    std::vector<int> distance(indices, size.x + size.y);
    std::queue<int> queue;
    std::set<int> processed;
    std::map<int, int> subsetIndexMap;

    for (int i = 0; i < indices; i++)
    {
        int index = subset[i].x + subset[i].y * size.x;
        subsetIndexMap[index] = i; // map position to subset vector index

        if (hasProperty_(map, index))
        {
            distance[i] = 0;
            queue.push(i);
        }
    }
    
    while (!queue.empty())
    {
        int subsetIndex = queue.front();
        int subsetDistance = distance[subsetIndex];
        
        queue.pop();
        
        auto neighbors = GridUtility::Neighbors(subset[subsetIndex], size);
        
        for (auto n: neighbors)
        {
            int mapIndex = n.x + n.y * size.x;
            
            if (!hasProperty_(map, mapIndex))
            {
                auto el = subsetIndexMap.find(mapIndex);
                if (el != subsetIndexMap.end())
                {
                    int i = el->second;
                    
                    distance[i] = min(distance[i], subsetDistance + 1);
                    
                    if (processed.insert(i).second)
                    {
                        queue.push(i);
                    }
                }
            }
        }
    }
    
    return distance;
}

void DistanceField::Add(std::vector<int>& first, const std::vector<int>& second)
{
    for (unsigned i = 0; i < first.size(); i++)
    {
        first[i] += second[i];
    }
}
