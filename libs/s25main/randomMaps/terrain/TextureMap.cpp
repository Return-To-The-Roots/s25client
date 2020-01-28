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
#include "randomMaps/terrain/TextureMap.h"
#include "randomMaps/algorithm/Filter.h"
#include "randomMaps/algorithm/GridUtility.h"

void TextureMap::FixGrassland(std::vector<TextureType>& textures,
                              const MapExtent& size,
                              TextureType neighbor,
                              TextureType replace)
{
    for (int x = 0; x < size.x; x++)
    {
        for (int y = 0; y < size.y; y++)
        {
            auto index = x + y * size.x;
            auto texture = textures[index];
            
            if (!Texture::IsMountain(texture) &&
                !Texture::IsWater(texture) &&
                 texture > replace)
            {
                auto neighbors = GridUtility::Collect(Position(x,y), size, 1.5);
                for (auto n = neighbors.begin(); n != neighbors.end(); ++n)
                {
                    if (textures[n->x + n->y * size.x] == neighbor)
                    {
                        textures[index] = replace;
                        break;
                    }
                }
            }
        }
    }
}

std::vector<TextureType> TextureMap::Create(const std::vector<unsigned char>& heightMap,
                                            const std::vector<bool>& waterMap,
                                            const MapExtent& size,
                                            unsigned char seaLevel,
                                            unsigned char mountainLevel)
{
    int tiles = size.x * size.y;
    std::vector<TextureType> textures(tiles);
    
    for (int i = 0; i < tiles; ++i)
    {
        textures[i] = waterMap[i] ? Water : translator_.GetTexture(heightMap[i], seaLevel, mountainLevel);
    }

    for (int i = 0; i < tiles; ++i)
    {
        if (Texture::IsMountain(textures[i]))
        {
            auto position = GridUtility::GetPosition(i, size);
            auto neighbors = GridUtility::Collect(position, size, 1.5);
            for (auto neighbor : neighbors)
            {
                if (Texture::IsGrass(textures[neighbor.x + neighbor.y * size.x]))
                {
                    textures[i] = GrassToMountain;
                }
            }
        }
    }
    
    FixGrassland(textures, size, Water, Coast);
    FixGrassland(textures, size, Coast, CoastToGreen1);
    FixGrassland(textures, size, CoastToGreen1, CoastToGreen2);

    return textures;
}
