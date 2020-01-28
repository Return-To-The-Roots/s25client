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
#include "randomMaps/resources/MinesAndFish.h"
#include "randomMaps/algorithm/GridUtility.h"

void MinesAndFish::GenerateMine(std::vector<unsigned char>& resources,
                                int index, const MapSettings& settings)
{
    int randomNumber = rnd_.Rand(1, 100);
    int ratio = settings.ratioGold;
    
    if (randomNumber < ratio)
    {
        resources[index] = libsiedler2::R_Gold + rnd_.Rand(0, 8);
        return;
    }
    
    ratio += settings.ratioCoal;
    if (randomNumber < ratio)
    {
        resources[index] = libsiedler2::R_Coal + rnd_.Rand(0, 8);
        return;
    }

    ratio += settings.ratioIron;
    if (randomNumber < ratio)
    {
        resources[index] = libsiedler2::R_Iron + rnd_.Rand(0, 8);
        return;
    }
    
    ratio += settings.ratioGranite;
    if (randomNumber < ratio)
    {
        resources[index] = libsiedler2::R_Granite + rnd_.Rand(0, 8);
    }
}

void MinesAndFish::Generate(Map& map, const MapSettings& settings)
{
    auto& rsu = map.textureRsu;
    auto& lsd = map.textureLsd;
    auto& resources = map.resource;
    auto size = map.size.x * map.size.y;
    
    for (int i = 0; i < size; ++i)
    {
        if (Texture::IsMountain(rsu[i]) ||
            Texture::IsMountain(lsd[i]))
        {
            GenerateMine(resources, i, settings);
        }
        else if (Texture::IsWater(rsu[i]) ||
                 Texture::IsWater(lsd[i]))
        {
            resources[i] = libsiedler2::R_Fish;
        }
    }
}
