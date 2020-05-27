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
#include "randomMaps/terrain/TextureFitter.h"
#include "randomMaps/algorithm/GridUtility.h"

void TextureFitter::SmoothTextures(std::vector<TextureType>& rsu,
                                    std::vector<TextureType>& lsd,
                                    const MapExtent& size,
                                    bool ignoreWater)
{
    const int nodes = size.x * size.y;
    
    std::vector<TextureType> originalRsu(rsu);
    
    TextureType texture;
    TextureType textureRight;
    TextureType textureLeft;
    TextureType textureMiddle;

    for (int index = 0; index < nodes; ++index)
    {
        texture = rsu[index];

        if (ignoreWater && texture == Water)
        {
            continue;
        }

        auto pos = GridUtility::GetPosition(index, size);
        auto neighbors = GridUtility::NeighborsOfRsuTriangle(pos, size);

        textureRight  = lsd[neighbors[0].x + neighbors[0].y * size.x];
        textureLeft   = lsd[neighbors[1].x + neighbors[1].y * size.x];
        textureMiddle = lsd[neighbors[2].x + neighbors[2].y * size.x];
        
        if (textureRight == textureLeft &&
            texture != textureRight && texture != textureLeft)
        {
            rsu[index] = textureRight;
            continue;
        }
        
        else if (textureMiddle == textureLeft &&
            texture != textureMiddle && texture != textureLeft)
        {
            rsu[index] = textureMiddle;
            continue;
        }

        if (textureMiddle == textureRight &&
            texture != textureMiddle && texture != textureRight)
        {
            rsu[index] = textureMiddle;
        }
    }
}

void TextureFitter::SmoothMountains(std::vector<TextureType>& rsu,
                                    std::vector<TextureType>& lsd,
                                    const MapExtent& size)
{
    for(int x = 0; x < size.x; x++)
    {
        for(int y = 0; y < size.y; y++)
        {
            int index = x + y * size.x;
            
            if (rsu[index] == GrassToMountain || lsd[index] == GrassToMountain)
            {
                auto hasMountainNeighbor = false;
                auto neighbors = GridUtility::Collect(Position(x,y), size, 2.0);
                
                for (auto p: neighbors)
                {
                    int nidx = p.x + p.y * size.x;
                    
                    if (Texture::IsMountain(rsu[nidx]) || Texture::IsMountain(lsd[nidx]))
                    {
                        hasMountainNeighbor = true;
                        break;
                    }
                }
                
                if (!hasMountainNeighbor)
                {
                    rsu[index] = GrassFlower;
                    lsd[index] = GrassFlower;
                }
            }
        }
    }
}
