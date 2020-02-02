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

#include "rttrDefines.h"

#include "mapGenerator/Textures.h"
#include "mapGenerator/DistanceByProperty.h"
#include "mapGenerator/GridUtility.h"

#include <stdexcept>

namespace rttr {
namespace mapGenerator {

// NEW WORLD

void IncreaseMountains(Map& map)
{
    auto isMountain = [] (const TerrainDesc& desc) {
        return
            desc.kind == TerrainKind::MOUNTAIN ||
            desc.kind == TerrainKind::SNOW ||
            desc.kind == TerrainKind::LAVA;
    };
    
    auto& z = map.z;
    auto maximum = map.height.max;
    
    RTTR_FOREACH_PT(MapPoint, map.size)
    {
        if (map.CheckTexture(Triangle(true, pt), isMountain))
        {
            z[pt] = std::min(z[pt] + 1, maximum);
        }

        if (map.CheckTexture(Triangle(false, pt), isMountain))
        {
            z[pt] = std::min(z[pt] + 1, maximum);
        }
    }
}

// OLD WORLD

bool Has40PercentWaterTilesInNeighborhood(const Position& position,
                                          const WaterMap& water,
                                          const MapExtent& size)
{
    auto neighbors = GridCollect(position, size, 4.0);
    auto waterNeighbors = 0;
    
    for (auto neighbor: neighbors)
    {
        if (water[neighbor.x + neighbor.y * size.x])
        {
            waterNeighbors++;
        }
    }
    
    return static_cast<double>(waterNeighbors) / neighbors.size() >= 0.4;
}

Coast FindCoast(const Island& island, const WaterMap& water, const MapExtent& size)
{
    Coast coast;
    
    for (auto vertex: island)
    {
        auto neighbors = GridNeighbors(vertex, size);
        
        for (auto neighbor: neighbors)
        {
            if (water[neighbor.x + neighbor.y * size.x])
            {
                if (Has40PercentWaterTilesInNeighborhood(vertex, water, size))
                {
                    coast.push_back(CoastNode(vertex,neighbor));
                }
                
                break;
            }
        }
    }
    
    return coast;
}

void CreateTextures(Map_& map, TextureMapping_& mapping)
{
    const auto textures = mapping.MapHeightsToTerrains(map.height.max, map.sea, map.mountains);
    
    const int nodes = map.size.x * map.size.y;
    
    auto& rsu = map.textureRsu;
    auto& lsd = map.textureLsd;
    auto& z   = map.z;
    
    for (int i = 0; i < nodes; ++i)
    {
        Texture texture = textures[z[i]];
        
        rsu[i] = texture;
        lsd[i] = texture;
        
        if (texture == mapping.water)
        {
            z[i] = map.sea;
        }
    }
}

void AddCoastTextures(Map_& map, TextureMapping_& mapping)
{
    const int nodes = map.size.x * map.size.y;
    
    auto isWater = [&map, &mapping] (int index) { return IsWater(map, mapping, index); };
    const auto distance = DistanceByProperty(map.size, isWater);
    const Texture water = mapping.water;
    
    auto& rsu = map.textureRsu;
    auto& lsd = map.textureLsd;
    
    int d;
    for (int i = 0; i < nodes; ++i)
    {
        d = distance[i];
        
        if (d >= 0 && d <= 3)
        {
            if (rsu[i] != water)
            {
                rsu[i] = mapping.GetCoastTerrain(d > 0 ? d - 1 : 0);
            }
            
            if (lsd[i] != water)
            {
                lsd[i] = mapping.GetCoastTerrain(d > 0 ? d - 1 : 0);
            }
        }
    }
}

void AddMountainTransition(Map_& map, TextureMapping_& mapping)
{
    auto isMountain = [&map, &mapping] (int index) {
        
        return
            mapping.IsMineableMountain(map.textureRsu[index]) ||
            mapping.IsMineableMountain(map.textureLsd[index]);
    };
    
    const auto distance = DistanceByProperty(map.size, isMountain);
    const Texture transition = mapping.mountainTransition;
    const int nodes = map.size.x * map.size.y;
    
    auto& rsu = map.textureRsu;
    auto& lsd = map.textureLsd;
    
    for (int i = 0; i < nodes; ++i)
    {
        if (distance[i] <= 2)
        {
            if (!mapping.IsMountain(rsu[i]))
            {
                rsu[i] = transition;
            }
            
            if (!mapping.IsMountain(lsd[i]))
            {
                lsd[i] = transition;
            }
        }
    }
}

void SmoothTextures(Map_& map, TextureMapping_& mapping, bool ignoreWater)
{
    const Texture water = mapping.water;
    const auto size = map.size;
    const int nodes = size.x * size.y;
    
    auto& rsu = map.textureRsu;
    auto& lsd = map.textureLsd;
    
    Texture texture;
    Texture textureRight;
    Texture textureLeft;
    Texture textureMiddle;

    for (int index = 0; index < nodes; ++index)
    {
        if (ignoreWater && rsu[index] == water)
        {
            continue;
        }

        texture = rsu[index];

        auto pos = GridPosition(index, size);
        auto neighbors = GridNeighborsOfRsuTriangle(pos, size);

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

void IncreaseMountains(Map_& map, TextureMapping_& mapping)
{
    const int nodes = map.size.x * map.size.y;
    const unsigned char maximumHeight = map.height.max;
    
    const auto& rsu = map.textureRsu;
    const auto& lsd = map.textureLsd;
    
    auto& z = map.z;
    
    for (int i = 0; i < nodes; ++i)
    {
        if (mapping.IsMountain(rsu[i]))
        {
            z[i] = std::min(static_cast<unsigned char>(z[i] + 1), maximumHeight);
        }
        
        if (mapping.IsMountain(lsd[i]))
        {
            z[i] = std::min(static_cast<unsigned char>(z[i] + 1), maximumHeight);
        }
    }
}


}}
