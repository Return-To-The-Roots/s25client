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

#include "mapGenerator/TextureMapping.h"
#include "helpers/containerUtils.h"

#include <cmath>
#include <iostream>

namespace rttr {
namespace mapGenerator {

// NEW WORLD

RttrMapping::RttrMapping(WorldDescription& worldDesc) : worldDesc_(worldDesc)
{
    
}

Textures RttrMapping::GetTextures(const DescIdx<LandscapeDesc>& landscape) const
{
    Textures textures;
    
    for (DescIdx<TerrainDesc> t(0); t.value < worldDesc_.terrain.size(); t.value++)
    {
        if (worldDesc_.get(t).landscape == landscape)
        {
            textures.push_back(t);
        }
    }
    
    return textures;
}

bool RttrMapping::CheckTexture(const Texture& texture, Predicate predicate) const
{
    return predicate(worldDesc_.get(texture));
}

uint8_t RttrMapping::GetLandscapeIndex(const DescIdx<LandscapeDesc>& landscape) const
{
    return worldDesc_.get(landscape).s2Id;
}

uint8_t RttrMapping::GetTerrainIndex(const DescIdx<TerrainDesc>& terrain) const
{
    return worldDesc_.get(terrain).s2Id;
}

// OLD WORLD

int MapRangeToIndex(int value, int minimum, int maximum, size_t size)
{
    double slope = 1. * (size - 1) / (maximum - minimum);
    return static_cast<int>(round(slope * (value - minimum)));
}

void RttrTextureMapping::InitializeTerrains(DescIdx<LandscapeDesc> landscape)
{
    for (DescIdx<TerrainDesc> t(0); t.value < worldDesc_.terrain.size(); t.value++)
    {
        if (worldDesc_.get(t).landscape == landscape)
        {
            terrains_.push_back(t);
        }
    }
    
    // --- Water terrain ---

    auto waterTerrains = FindTerrains([](const auto& desc) {
        return
            desc.kind == TerrainKind::WATER &&
            desc.Is(ETerrain::Shippable);
    });
    
    if (waterTerrains.empty())
    {
        throw std::runtime_error("no water terrain found");
    }
    
    water = waterTerrains.front();
    
    // --- Coast terrain ---
    
    auto coastTerrains = FindTerrains([](const auto& desc) {
        return
            desc.kind == TerrainKind::LAND &&
            desc.Is(ETerrain::Walkable) &&
            desc.GetBQ() == TerrainBQ::FLAG;
    });
    
    if (coastTerrains.empty())
    {
        throw std::runtime_error("no coast terrain found");
    }
    
    coast = coastTerrains.front();
    coast_.push_back(coast);

    // --- Buildable land ordered by humidity ---

    auto buildableTerrains = FindTerrains([](const auto& desc) {
        return desc.kind == TerrainKind::LAND && desc.Is(ETerrain::Buildable);
    });
    
    if (buildableTerrains.empty())
    {
        throw std::runtime_error("no buildable terrain found");
    }
    
    std::sort(buildableTerrains.begin(), buildableTerrains.end(), [this] (auto a, auto b) {
        return worldDesc_.get(a).humidity < worldDesc_.get(b).humidity;
    });
    
    int maxBuildableTerrains = std::min<int>(6, buildableTerrains.size());
    
    for (int i = 0; i < 2; i++)
    {
        coast_.push_back(buildableTerrains[i]);
    }
    
    for (int i = 1; i < maxBuildableTerrains; i++)
    {
        grassland_.push_back(buildableTerrains[i]);
    }
    
    flowers = grassland_.back();
 
    // --- Mountains ---

    auto builableMountainTerrains = FindTerrains([](const auto& desc) {
        return desc.kind == TerrainKind::MOUNTAIN && desc.Is(ETerrain::Buildable);
    });
    
    mountainTransition = *std::max_element(builableMountainTerrains.begin(), builableMountainTerrains.end(), [this](auto a, auto b) {
        return worldDesc_.get(a).humidity < worldDesc_.get(b).humidity;
    });

    auto mountainTerrains = FindTerrains([](const auto& desc) {
        return desc.kind == TerrainKind::MOUNTAIN && desc.Is(ETerrain::Mineable);
    });
    
    if (mountainTerrains.empty())
    {
        throw std::runtime_error("no minable mountain terrain found");
    }
    
    int maxMountainTerrains = std::min<int>(4, mountainTerrains.size());
    
    for (int i = 0; i < maxMountainTerrains; i++)
    {
        mountain_.push_back(mountainTerrains[i]);
    }
    
    auto snowTerrains = FindTerrains([](const auto& desc) {
        return desc.kind == TerrainKind::SNOW;
    });
    
    if (snowTerrains.empty())
    {
        auto lavaTerrains = FindTerrains([](const auto& desc) {
            return desc.kind == TerrainKind::LAVA;
        });
        
        if (lavaTerrains.empty())
        {
            throw std::runtime_error("no lava and no snow terrain found");
        }
        
        mountain_.push_back(lavaTerrains.front());
    }
    else
    {
        mountain_.push_back(snowTerrains.front());
    }
}

RttrTextureMapping::RttrTextureMapping(WorldDescription& worldDesc,
                                       DescIdx<LandscapeDesc> landscape)
    : worldDesc_(worldDesc)
{
    InitializeTerrains(landscape);
}

Textures RttrTextureMapping::MapHeightsToTerrains(Height peak, Height sea, Height mountains)
{
    Textures terrains(peak + 1);

    if (sea + 2 >= mountains)
    {
        throw std::invalid_argument("sea must be lower than mountains by more than 2");
    }
    
    if (sea > peak || mountains > peak)
    {
        throw std::invalid_argument("sea and mountains must be below peak height");
    }
    
    for (Height z = 0; z <= peak; z++)
    {
        if (z <= sea)
        {
            terrains[z] = water;
        }
        
        if (z > sea && z < mountains)
        {
            int index = MapRangeToIndex(z, sea + 1, mountains - 1, grassland_.size());
            terrains[z] = grassland_[index];
        }
        
        if (z >= mountains)
        {
            int index = MapRangeToIndex(z, mountains, peak, mountain_.size());
            terrains[z] = mountain_[index];
        }
    }

    return terrains;
}

Texture RttrTextureMapping::GetCoastTerrain(int distance) const
{
    return coast_[distance];
}

int RttrTextureMapping::GetHumidity(const Texture& texture) const
{
    return worldDesc_.get(texture).humidity;
}

bool RttrTextureMapping::IsCoast(const Texture& texture, int distance) const
{
    return texture == GetCoastTerrain(distance);
}

bool RttrTextureMapping::IsMountain(const Texture& texture) const
{
    return worldDesc_.get(texture).kind == TerrainKind::MOUNTAIN;
}

bool RttrTextureMapping::IsMineableMountain(const Texture& texture) const
{
    TerrainDesc desc = worldDesc_.get(texture);
    
    return
        desc.kind == TerrainKind::MOUNTAIN &&
        desc.Is(ETerrain::Mineable);
}

bool RttrTextureMapping::IsGrassland(const Texture& texture) const
{
    return worldDesc_.get(texture).humidity > Unwrap(GetCoastTerrain(2)).humidity;
}

bool RttrTextureMapping::IsBuildable(const Texture& texture) const
{
    TerrainDesc desc = worldDesc_.get(texture);
    
    return
        desc.kind == TerrainKind::LAND &&
        desc.Is(ETerrain::Buildable);
}

bool RttrTextureMapping::IsWater(const Texture& texture) const
{
    return texture == water;
}

TerrainDesc RttrTextureMapping::Unwrap(const Texture& texture) const
{
    return worldDesc_.get(texture);
}

}}
