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
#include "randomMaps/resources/Trees.h"
#include "randomMaps/algorithm/DistanceField.h"
#include "randomMaps/algorithm/GridUtility.h"

#include <cmath>

std::vector<int> Trees::GetTreeTypes(TextureType texture)
{
    switch (texture)
    {
        case Coast:           return { 0xF0, 0xF1, 0xF2, 0x30, 0x31, 0x70, 0x71 };
        case CoastToGreen1:   return { 0xF2, 0xF3, 0xF4, 0x32, 0x33, 0x34, 0x35 };
        case CoastToGreen2:   return { 0xB0, 0xB1, 0x30, 0xF5, 0x36, 0x37, 0xB2 };
        case Grass1:          return { 0x71, 0x72, 0xB2, 0x33, 0xB4, 0xF6, 0xF5 };
        case Grass2:          return { 0x73, 0x74, 0xB5, 0x36, 0xB7, 0xB0, 0xF6 };
        case Grass3:          return { 0x76, 0x77, 0xB6, 0x32, 0xB3, 0xB4, 0xF7 };
        case GrassFlower:     return { 0x71, 0x73, 0xB0, 0x34, 0xB2, 0xB7, 0xF0 };
        case GrassToMountain: return { 0x70, 0xB5, 0xB0, 0x37, 0x32, 0xB1, 0xB0 };
        case Mountain1:       return { 0x70, 0xB5, 0xB0, 0x37, 0x32, 0xB1, 0xB0 };
        case Mountain2:       return { 0x70, 0xB5, 0xB0, 0x37, 0x32, 0xB1, 0xB0 };
        case Mountain3:       return { 0x70, 0xB5, 0xB0, 0x37, 0x32, 0xB1, 0xB0 };
        case Mountain4:       return { 0x70, 0xB5, 0xB0, 0x37, 0x32, 0xB1, 0xB0 };
        default:              return {};
    }
}

std::vector<int> Trees::GetTreeInfos(TextureType texture)
{
    switch (texture)
    {
        case Coast:           return { 0xC4, 0xC4, 0xC4, 0xC5, 0xC5, 0xC5, 0xC5 };
        case CoastToGreen1:   return { 0xC4, 0xC4, 0xC4, 0xC5, 0xC5, 0xC4, 0xC4 };
        case CoastToGreen2:   return { 0xC4, 0xC4, 0xC4, 0xC4, 0xC5, 0xC5, 0xC5 };
        case Grass1:          return { 0xC4, 0xC4, 0xC4, 0xC6, 0xC5, 0xC5, 0xC5 };
        case Grass2:          return { 0xC4, 0xC4, 0xC4, 0xC6, 0xC5, 0xC5, 0xC5 };
        case Grass3:          return { 0xC4, 0xC4, 0xC4, 0xC6, 0xC5, 0xC5, 0xC5 };
        case GrassToMountain: return { 0xC4, 0xC4, 0xC4, 0xC6, 0xC6, 0xC5, 0xC5 };
        case GrassFlower:     return { 0xC4, 0xC4, 0xC4, 0xC6, 0xC5, 0xC5, 0xC5 };
        case Mountain1:       return { 0xC4, 0xC4, 0xC4, 0xC6, 0xC6, 0xC5, 0xC5 };
        case Mountain2:       return { 0xC4, 0xC4, 0xC4, 0xC6, 0xC6, 0xC5, 0xC5 };
        case Mountain3:       return { 0xC4, 0xC4, 0xC4, 0xC6, 0xC6, 0xC5, 0xC5 };
        case Mountain4:       return { 0xC4, 0xC4, 0xC4, 0xC6, 0xC6, 0xC5, 0xC5 };
        default:              return {};
    }
}

int Trees::GetProbability(TextureType texture, int distance)
{
    int prob;
    
    switch (texture)
    {
        case Coast:           prob = 17; break;
        case CoastToGreen1:   prob = 19; break;
        case CoastToGreen2:   prob = 21; break;
        case Grass1:          prob = 22; break;
        case Grass2:          prob = 25; break;
        case Grass3:          prob = 27; break;
        case GrassFlower:     prob = 35; break;
        case GrassToMountain: prob = 28; break;
        case Mountain1:       prob = 24; break;
        case Mountain2:       prob = 18; break;
        case Mountain3:       prob = 13; break;
        case Mountain4:       prob =  9; break;
        default:
            return 0;
    }
    
    if (distance < 4)
    {
        return 0;
    }
    
    if (distance < 20)
    {
        return std::min(prob, distance * 2);
    }

    return prob;
}

void Trees::Generate(Map& map, const std::vector<int>& freeZone)
{
    auto size = map.size.x * map.size.y;
    auto textures = map.textureRsu;
    
    TextureType texture;
    
    for (int i = 0; i < size; ++i)
    {
        texture = textures[i];
        
        auto treeTypes = GetTreeTypes(texture);
        auto treeInfos = GetTreeInfos(texture);
        auto prob      = GetProbability(texture, freeZone[i]);
        
        if (rnd_.ByChance(prob) && !treeTypes.empty())
        {
            int index = rnd_.Index(treeTypes.size());
            
            map.objectType[i] = treeTypes[index];
            map.objectInfo[i] = treeInfos[index];
        }
    }
}
