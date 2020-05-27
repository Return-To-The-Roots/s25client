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
#include "randomMaps/elevation/Smoother.h"
#include "randomMaps/algorithm/GridUtility.h"

#include <cmath>

Smoother::Smoother(MapExtent size) : size_(size)
{
    switch ((size.x + size.y) / 2)
    {
        case 64:  iterations_ = 10; distance_ = 2; break;
        case 128: iterations_ = 14; distance_ = 2; break;
        case 256: iterations_ = 12; distance_ = 3; break;
        case 512: iterations_ = 15; distance_ = 3; break;
        case 1024:iterations_ = 17; distance_ = 3; break;
        default:  iterations_ = 18; distance_ = 3; break;
    }
}

Smoother::Smoother(MapExtent size, int iterations, double distance)
    : size_(size), iterations_(iterations), distance_(distance)
{
    
}

void Smoother::Smooth(std::vector<unsigned char>& heightMap)
{
    std::vector<bool> mask(heightMap.size(), true);
    Smooth(heightMap, mask);
}

void Smoother::Smooth(std::vector<unsigned char>& heightMap, const std::vector<bool>& mask)
{
    const int tiles = size_.x * size_.y;
    const int width = size_.x;
    
    std::vector<double> normHeight(heightMap.size());
    
    auto maximum = *std::max_element(heightMap.begin(), heightMap.end());
    auto minimum = *std::min_element(heightMap.begin(), heightMap.end());
    auto dz = maximum - minimum;
    
    for (int i = 0; i < tiles; i++)
    {
        normHeight[i] = double(heightMap[i] - minimum) / dz;
    }
    
    for (int it = 0; it < iterations_; ++it)
    {
        for (int i = 0; i < tiles; i++)
        {
            if (mask[i])
            {
                auto p = Position(i % width, i / width);
                auto neighbors = GridUtility::Collect(p, size_, distance_);
                auto sum = 0.0;
                
                for (auto n = neighbors.begin(); n != neighbors.end(); ++n)
                {
                    sum += normHeight[n->x + n->y * width];
                }
                
                int k = neighbors.size();
                if (k > 0)
                {
                    normHeight[i] = (normHeight[i] + sum / k) / 2;
                }
            }
        }
    }
    
    for (int i = 0; i < tiles; i++)
    {
        if (mask[i])
        {
            heightMap[i] = minimum + char(round(normHeight[i] * dz));
        }
    }
}
