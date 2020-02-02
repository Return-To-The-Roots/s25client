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

#include "mapGenerator/HeightMap.h"
#include "mapGenerator/GridUtility.h"

#include <cmath>

namespace rttr {
namespace mapGenerator {

HeightMap CreateRandomHeightMap(const MapExtent& size,
                                const Range& height,
                                RandomUtility& rnd)
{
    const int nodes = size.x * size.y;
    HeightMap z(nodes);
    
    for (int i = 0; i < nodes; ++i)
    {
        z[i] = rnd.Rand(height.min, height.max);
    }
    
    return z;
}

Height SeaLevel(const HeightMap& z, double coverage)
{
    Height seaLevel = 0;
    
    int nodes = z.size();
    int count = 0;
    
    double ratio = 0.;
    
    while (ratio < coverage && count != nodes)
    {
        auto isLower = [seaLevel](auto h) { return h <= seaLevel; };
        
        count = std::count_if(z.begin(), z.end(), isLower);
        
        ratio = static_cast<double>(count) / nodes;
        
        seaLevel++;
    }
    
    return seaLevel == 0 ? 0 : seaLevel - 1;
}

Height MountainLevel(const HeightMap& z, double coverage)
{
    std::set<int> indices;

    for (int i = 0; i < (int)z.size(); ++i)
    {
        indices.insert(i);
    }
    
    return MountainLevel(z, coverage, indices);
}

Height MountainLevel(const HeightMap& z, double coverage, const std::set<int>& subset)
{
    int nodes = subset.size();
    
    Height level = 0;
    
    double ratio = 1.;
    while (ratio > coverage)
    {
        auto isHigher = [&z, level] (auto i) { return z[i] >= level; };
        
        int count = std::count_if(subset.begin(), subset.end(), isHigher);

        ratio = static_cast<double>(count) / nodes;
        level++;
    }
    
    return level == 0 ? 0 : level - 1;
}

void ScaleToFit(HeightMap& z, const Range& height)
{
    auto range = std::minmax_element(z.begin(), z.end());
    
    int minimum = *range.first;
    int maximum = *range.second;
    
    int nodes = z.size();

    int maximumRange = height.Difference();
    int actualRange = maximum - minimum;
    
    if (actualRange != 0)
    {
        for (int i = 0; i < nodes; ++i)
        {
            auto normalizer = static_cast<double>(z[i] - minimum) / actualRange;
            auto heightOffset = round(normalizer * maximumRange);
            
            z[i] = static_cast<Height>(height.min + heightOffset);
        }
    }
}

HeightMap HeightFromDistance(const std::vector<int>& distance, const Range& height)
{
    int nodes = distance.size();
    
    HeightMap z(nodes, height.min);
    
    auto range = std::minmax_element(distance.begin(), distance.end());
    
    int minimum = *range.first;
    int maximum = *range.second;

    int distanceRange = maximum - minimum;
    if (distanceRange != 0)
    {
        int heightRange = height.Difference();

        for (int i = 0; i < nodes; ++i)
        {
            auto normalizer = static_cast<double>(distance[i] - minimum) / distanceRange;
            auto heightOffset = round(normalizer * heightRange);
            
            z[i] = static_cast<Height>(height.min + heightOffset);
        }
    }
    
    return z;
}

void Smooth(int iterations, double radius, HeightMap& z, const MapExtent& size)
{
    const int nodes = size.x * size.y;
    const int width = size.x;
    
    std::vector<double> normHeight(z.size());
    
    auto bounds = std::minmax_element(z.begin(), z.end());
    
    Height minimum = *bounds.first;
    Height maximum = *bounds.second;
    
    auto dz = maximum - minimum;
    
    for (int i = 0; i < nodes; i++)
    {
        normHeight[i] = static_cast<double>(z[i] - minimum) / dz;
    }
    
    for (int it = 0; it < iterations; ++it)
    {
        for (int i = 0; i < nodes; i++)
        {
            auto position = GridPosition(i, size);
            auto neighbors = GridCollect(position, size, radius);
            auto sum = 0.0;
            
            for (auto neighbor : neighbors)
            {
                sum += normHeight[neighbor.x + neighbor.y * width];
            }
            
            int k = neighbors.size();
            if (k > 0)
            {
                normHeight[i] = (normHeight[i] + sum / k) / 2;
            }
        }
    }
    
    for (int i = 0; i < nodes; i++)
    {
        z[i] = minimum + static_cast<char>(round(normHeight[i] * dz));
    }}

}}
