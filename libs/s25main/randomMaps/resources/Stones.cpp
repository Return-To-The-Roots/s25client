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
#include "randomMaps/resources/Stones.h"
#include "randomMaps/algorithm/DistanceField.h"
#include "randomMaps/algorithm/GridUtility.h"

int Stones::GetProbability(TextureType texture, int distance)
{
    int prob;
    
    switch (texture)
    {
        case Coast:           prob = 2; break;
        case CoastToGreen1:   prob = 2; break;
        case CoastToGreen2:   prob = 2; break;
        case Grass1:          prob = 2; break;
        case Grass2:          prob = 2; break;
        case Grass3:          prob = 2; break;
        case GrassFlower:     prob = 2; break;
        case GrassToMountain: prob = 2; break;
        case Mountain1:       prob = 2; break;
        case Mountain2:       prob = 2; break;
        case Mountain3:       prob = 2; break;
        case Mountain4:       prob = 2; break;
        default:
            return 0;
    }
    
    if (distance < 10)
    {
        return 0;
    }
    
    return prob;
}

void Stones::Generate(Map& map, const std::vector<int>& freeZone)
{
    auto size = map.size.x * map.size.y;
    auto textures = map.textureRsu;
    auto stoneTypes = std::vector<int>{ 0xCC, 0xCD };

    for (int i = 0; i < size; ++i)
    {
        auto prob = GetProbability(textures[i], freeZone[i]);
        
        if (rnd_.ByChance(prob))
        {
            auto position = GridUtility::GetPosition(i, map.size);
            auto neighbors = GridUtility::Collect(position, map.size, rnd_.DRand(0.0, 2.0));
            
            for (auto neighbor : neighbors)
            {
                auto index = neighbor.x + neighbor.y * map.size.x;
                if (Texture::IsBuildable(textures[index]))
                {
                    auto type = rnd_.Index(stoneTypes.size());
                    
                    map.objectType[index] = rnd_.Rand(1, 6);
                    map.objectInfo[index] = stoneTypes[type];
                }
            }
        }
    }
}
