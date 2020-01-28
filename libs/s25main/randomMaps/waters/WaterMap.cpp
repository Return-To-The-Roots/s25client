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
#include "randomMaps/waters/WaterMap.h"

std::vector<bool> WaterMap::Create(std::vector<unsigned char>& heightMap,
                                   const MapExtent& size,
                                   unsigned char seaLevel)
{
    std::vector<bool> waterMap (heightMap.size());
    auto n = size.x * size.y;
    
    for (int i = 0; i < n; ++i)
    {
        waterMap[i] = (heightMap[i] <= seaLevel);
        if (waterMap[i])
        {
            heightMap[i] = seaLevel;
        }
    }
    
    return waterMap;
}

std::vector<bool> WaterMap::For(const Map& map)
{
    auto rsu = map.textureRsu;
    auto lsd = map.textureLsd;
    
    int tiles = map.size.x * map.size.y;
    std::vector<bool> waterMap(tiles);
    
    for (int i = 0; i < tiles; i++)
    {
        waterMap[i] =
            Texture::IsWater(rsu[i]) &&
            Texture::IsWater(lsd[i]);
    }
    
    return waterMap;
}
