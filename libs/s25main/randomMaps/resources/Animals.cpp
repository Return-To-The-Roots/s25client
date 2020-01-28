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
#include "randomMaps/resources/Animals.h"
#include "randomMaps/algorithm/DistanceField.h"
#include "randomMaps/algorithm/GridUtility.h"

#include <cmath>

using namespace libsiedler2;

std::vector<int> Animals::GetAnimals(TextureType texture)
{
    switch (texture)
    {
        case Water:           return { A_Duck, A_Duck2 };
        case Coast:           return { A_Duck, A_Fox };
        case CoastToGreen1:   return { A_Fox, A_Rabbit };
        case CoastToGreen2:   return { A_Rabbit, A_Sheep };
        case Grass1:          return { A_Rabbit, A_Sheep, A_Deer };
        case Grass2:          return { A_Rabbit, A_Sheep, A_Deer, A_Fox };
        case Grass3:          return { A_Rabbit, A_Sheep, A_Deer, A_Fox, A_Stag };
        case GrassFlower:     return { A_Rabbit, A_Sheep, A_Deer, A_Fox, A_Stag };
        case GrassToMountain: return { A_Rabbit, A_Sheep, A_Deer, A_Fox, A_Stag };
        case Mountain1:       return { A_Rabbit, A_Fox, A_Stag };
        case Mountain2:       return { A_Fox, A_Stag };
        case Mountain3:       return { A_Fox, A_Stag };
        case Mountain4:       return { A_Fox, A_Stag };
        default:              return {};
    }
}

int Animals::GetProbability(TextureType texture)
{
    switch (texture)
    {
        case Water:           return 3;
        case Coast:           return 3;
        case CoastToGreen1:   return 5;
        case CoastToGreen2:   return 6;
        case Grass1:          return 7;
        case Grass2:          return 8;
        case Grass3:          return 7;
        case GrassFlower:     return 9;
        case GrassToMountain: return 8;
        case Mountain1:       return 4;
        case Mountain2:       return 3;
        case Mountain3:       return 2;
        case Mountain4:       return 1;
        default:              return 0;
    }
}

void Animals::Generate(Map& map)
{
    auto size = map.size.x * map.size.y;
    auto textures = map.textureRsu;
    auto& animalsOnMap = map.animal;

    TextureType texture;
    
    for (int i = 0; i < size; ++i)
    {
        texture = textures[i];
        
        auto animals = GetAnimals(texture);
        auto prob = GetProbability(texture);
        
        if (rnd_.ByChance(prob) && !animals.empty())
        {
            animalsOnMap[i] = animals[rnd_.Index(animals.size())];
        }
    }
}
