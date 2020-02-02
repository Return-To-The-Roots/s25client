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

#include "mapGenerator/Resources.h"
#include "mapGenerator/Textures.h"
#include "mapGenerator/DistanceByProperty.h"
#include "helpers/containerUtils.h"

namespace rttr {
namespace mapGenerator {

using namespace libsiedler2;

bool IsHeadQuarterOrHarborPosition(const Map_& map, int index)
{
    return
        helpers::contains(map.harborsLsd, index) ||
        helpers::contains(map.harborsRsu, index) ||
        IsHeadQuarter(map, index);
}

std::vector<int> GetTreeTypes(const Texture& texture, const TextureMapping_& mapping)
{
    if (mapping.IsGrassland(texture))
    {
        return {
            0x32, 0x33, 0x34, 0x36,
            0x71, 0x72, 0x73, 0x74, 0x76, 0x77,
            0xB0, 0xB2, 0xB3, 0xB4, 0xB5, 0xB6, 0xB7,
            0xF0, 0xF5, 0xF6, 0xF7
        };
    }
    
    if (mapping.IsMountain(texture))
    {
        return { 0x70, 0xB5, 0xB0, 0x37, 0x32, 0xB1, 0xB0 };
    }
    
    if (mapping.IsCoast(texture))
    {
        return { 0xF0, 0xF1, 0xF2, 0x30, 0x31, 0x70, 0x71 };
    }
    
    if (mapping.IsCoast(texture, 1))
    {
        return { 0xF2, 0xF3, 0xF4, 0x32, 0x33, 0x34, 0x35 };
    }

    if (mapping.IsCoast(texture, 2))
    {
        return { 0xB0, 0xB1, 0x30, 0xF5, 0x36, 0x37, 0xB2 };
    }
    
    return {};
}

std::vector<int> GetTreeInfos(const Texture& texture, const TextureMapping_& mapping)
{
    if (mapping.IsGrassland(texture))
    {
        return { 0xC4, 0xC4, 0xC4, 0xC6, 0xC5, 0xC5, 0xC5 };
    }
    
    if (mapping.IsMountain(texture))
    {
        return { 0xC4, 0xC4, 0xC4, 0xC6, 0xC6, 0xC5, 0xC5 };
    }
    
    if (mapping.IsCoast(texture))
    {
        return { 0xC4, 0xC4, 0xC4, 0xC5, 0xC5, 0xC5, 0xC5 };
    }
    
    if (mapping.IsCoast(texture, 1))
    {
        return { 0xC4, 0xC4, 0xC4, 0xC5, 0xC5, 0xC4, 0xC4 };
    }

    if (mapping.IsCoast(texture, 2))
    {
        return { 0xC4, 0xC4, 0xC4, 0xC4, 0xC5, 0xC5, 0xC5 };
    }
    
    return {};
}

Animals GetAnimals(const Texture& texture, const TextureMapping_& mapping)
{
    if (mapping.water == texture)
    {
        return { A_Duck, A_Duck2 };
    }
    
    if (mapping.IsGrassland(texture))
    {
        return { A_Rabbit, A_Sheep, A_Deer, A_Fox };
    }
    
    if (mapping.IsMountain(texture))
    {
        return { A_Deer, A_Fox };
    }
    
    if (mapping.IsCoast(texture))
    {
        return { A_Fox };
    }
    
    if (mapping.IsCoast(texture, 1))
    {
        return { A_Fox, A_Rabbit };
    }

    if (mapping.IsCoast(texture, 2))
    {
        return { A_Rabbit, A_Sheep };
    }
    
    return {};
}

int GetTreeProbability(const Texture& texture, const TextureMapping_& mapping, int freeZoneDistance)
{
    int prob = mapping.GetHumidity(texture) / 2 + 10;
    
    if (texture == mapping.GetCoastTerrain())
    {
        return 5;
    }
    
    if (freeZoneDistance < 4 || texture == mapping.water)
    {
        return 0;
    }
    
    if (freeZoneDistance < 20)
    {
        return std::min(prob, freeZoneDistance * 2);
    }

    return prob;
}

int GetStoneProbability(const Texture& texture, const TextureMapping_& mapping, int freeZoneDistance)
{
    if (freeZoneDistance < 10 || texture == mapping.water)
    {
        return 0;
    }
    
    return 2;
}

int GetAnimalProbability(const Texture& texture, const TextureMapping_& mapping)
{
    return mapping.GetHumidity(texture) / 10 + 2;
}

void PlaceTrees(Map_& map,
                RandomUtility& rnd,
                const TextureMapping_& mapping,
                const std::vector<int>& freeZone)
{
    auto size = map.size.x * map.size.y;
    auto textures = map.textureRsu;
    
    Texture texture;
    
    for (int i = 0; i < size; ++i)
    {
        texture = textures[i];
        
        auto treeTypes = GetTreeTypes(texture, mapping);
        auto treeInfos = GetTreeInfos(texture, mapping);
        auto prob      = GetTreeProbability(texture, mapping, freeZone[i]);
        
        if (rnd.ByChance(prob) && !treeTypes.empty())
        {
            int index = rnd.Index(treeTypes.size());
            
            map.objectType[i] = treeTypes[index];
            map.objectInfo[i] = treeInfos[index];
        }
    }
}

void PlaceStones(Map_& map,
                 RandomUtility& rnd,
                 const TextureMapping_& mapping,
                 const std::vector<int>& freeZone)
{
    auto size = map.size.x * map.size.y;
    auto textures = map.textureRsu;
    auto stoneTypes = std::vector<int>{ 0xCC, 0xCD };

    for (int i = 0; i < size; ++i)
    {
        auto prob = GetStoneProbability(textures[i], mapping, freeZone[i]);
        
        if (rnd.ByChance(prob))
        {
            auto position = GridPosition(i, map.size);
            auto neighbors = GridCollect(position, map.size, rnd.DRand(0.0, 2.0));
            
            for (auto neighbor : neighbors)
            {
                auto index = neighbor.x + neighbor.y * map.size.x;
                if (mapping.IsBuildable(textures[index]))
                {
                    auto type = rnd.Index(stoneTypes.size());
                    
                    map.objectType[index] = rnd.Rand(1, 6);
                    map.objectInfo[index] = stoneTypes[type];
                }
            }
        }
    }
}

void PlaceMinesAndFish(Map_& map,
                       RandomUtility& rnd,
                       const TextureMapping_& mapping,
                       const MapSettings& settings)
{
    auto& rsu = map.textureRsu;
    auto& lsd = map.textureLsd;
    auto& resources = map.resource;
    auto size = map.size.x * map.size.y;
    
    for (int i = 0; i < size; ++i)
    {
        if (mapping.IsMountain(rsu[i]) || mapping.IsMountain(lsd[i]))
        {
            int randomNumber = rnd.Rand(1, 100);
            int ratio = settings.ratioGold;
            
            if (randomNumber < ratio)
            {
                resources[i] = libsiedler2::R_Gold + rnd.Rand(0, 8);
                continue;
            }
            
            ratio += settings.ratioCoal;
            if (randomNumber < ratio)
            {
                resources[i] = libsiedler2::R_Coal + rnd.Rand(0, 8);
                continue;
            }

            ratio += settings.ratioIron;
            if (randomNumber < ratio)
            {
                resources[i] = libsiedler2::R_Iron + rnd.Rand(0, 8);
                continue;
            }
            
            ratio += settings.ratioGranite;
            if (randomNumber < ratio)
            {
                resources[i] = libsiedler2::R_Granite + rnd.Rand(0, 8);
            }
        }
        else if (mapping.water == rsu[i] || mapping.water == lsd[i])
        {
            resources[i] = libsiedler2::R_Fish;
        }
    }
}

void PlaceAnimals(Map_& map, RandomUtility& rnd, const TextureMapping_& mapping)
{
    auto size = map.size.x * map.size.y;
    auto textures = map.textureRsu;
    auto& animalsOnMap = map.animal;

    Texture texture;
    
    for (int i = 0; i < size; ++i)
    {
        texture = textures[i];
        
        auto animals = GetAnimals(texture, mapping);
        auto prob = GetAnimalProbability(texture, mapping);
        
        if (rnd.ByChance(prob) && !animals.empty())
        {
            animalsOnMap[i] = animals[rnd.Index(animals.size())];
        }
    }
}

void PlaceResources(Map_& map,
                    RandomUtility& rnd,
                    const TextureMapping_& mapping,
                    const MapSettings& settings)
{
    auto isHqOrHarbor = [map] (int index) { return IsHeadQuarterOrHarborPosition(map, index); };
    const auto& freeZone = DistanceByProperty(map.size, isHqOrHarbor);
    
    PlaceTrees(map, rnd, mapping, freeZone);
    PlaceStones(map, rnd, mapping, freeZone);
    PlaceMinesAndFish(map, rnd, mapping, settings);
    PlaceAnimals(map, rnd, mapping);
}

}}
