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
#include "randomMaps/waters/RiverBrush.h"

#include <algorithm>
#include <set>

bool RiverBrush::IsAnyOf(const std::vector<TextureType>& textures,
                         unsigned char texture)
{
    for (auto t: textures)
        if (texture == t)
            return true;

    return false;
}


void RiverBrush::Paint(std::vector<Tile>& river, Map* map, unsigned char seaLevel)
{
    auto mountainsAndWater = {
        TextureType::GrassToMountain,
        TextureType::Mountain1,
        TextureType::Mountain2,
        TextureType::Mountain3,
        TextureType::Mountain4,
        TextureType::MountainPeak,
        TextureType::Water
    };
    
    auto size = map->size;
    
    std::set<int> rsu;
    std::set<int> lsd;
    
    for (auto tile = river.begin(); tile != river.end(); tile++)
    {
        int indexRsu = tile->IndexRsu(size);
        int indexLsd = tile->IndexLsd(size);
        
        rsu.insert(indexRsu);
        lsd.insert(indexLsd);

        map->z[indexRsu] = std::max(int(seaLevel), map->z[indexRsu] - 1);
        map->z[indexLsd] = std::max(int(seaLevel), map->z[indexLsd] - 1);
        
        map->textureRsu[indexRsu] = TextureType::Water;
        map->textureLsd[indexLsd] = TextureType::Water;
    }

    std::vector<Tile> coast;
    for (auto tile = river.begin(); tile != river.end(); tile++)
    {
        auto neighbors = tile->Neighbors(size);
        for (auto c = neighbors.begin(); c != neighbors.end(); c++)
        {
            coast.push_back(*c);
            
            int indexRsu = c->IndexRsu(size);
            int indexLsd = c->IndexLsd(size);
            
            if (!IsAnyOf(mountainsAndWater, map->textureRsu[indexRsu]))
            {
                map->textureRsu[indexRsu] = TextureType::Coast;
            }

            if (!IsAnyOf(mountainsAndWater, map->textureLsd[indexLsd]))
            {
                map->textureLsd[indexLsd] = TextureType::Coast;
            }
        }
    }
    
    std::vector<Tile> transition;
    for (auto tile = coast.begin(); tile != coast.end(); tile++)
    {
        auto neighbors = tile->Neighbors(size);
        for (auto c = neighbors.begin(); c != neighbors.end(); c++)
        {
            transition.push_back(*c);

            int indexRsu = c->IndexRsu(size);
            int indexLsd = c->IndexLsd(size);
            
            if (!IsAnyOf(mountainsAndWater, map->textureRsu[indexRsu]) &&
                map->textureRsu[indexRsu] != TextureType::Coast)
            {
                map->textureRsu[indexRsu] = TextureType::CoastToGreen1;
            }

            if (!IsAnyOf(mountainsAndWater, map->textureLsd[indexLsd]) &&
                map->textureLsd[indexLsd] != TextureType::Coast)
            {
                map->textureLsd[indexLsd] = TextureType::CoastToGreen1;
            }
        }
    }
    
    for (auto tile = transition.begin(); tile != transition.end(); tile++)
    {
        auto neighbors = tile->Neighbors(size);
        for (auto c = neighbors.begin(); c != neighbors.end(); c++)
        {
            int indexRsu = c->IndexRsu(size);
            int indexLsd = c->IndexLsd(size);
            
            if (!IsAnyOf(mountainsAndWater, map->textureRsu[indexRsu]) &&
                map->textureRsu[indexRsu] != TextureType::Coast &&
                map->textureRsu[indexRsu] != TextureType::CoastToGreen1)
            {
                map->textureRsu[indexRsu] = TextureType::CoastToGreen2;
            }

            if (!IsAnyOf(mountainsAndWater, map->textureLsd[indexLsd]) &&
                map->textureLsd[indexLsd] != TextureType::Coast &&
                map->textureLsd[indexLsd] != TextureType::CoastToGreen1)
            {
                map->textureLsd[indexLsd] = TextureType::CoastToGreen2;
            }
        }
    }
}
