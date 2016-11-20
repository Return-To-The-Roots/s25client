// Copyright (c) 2005 - 2015 Settlers Freaks (sf-team at siedler25.org)
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

#include "mapGenerator/Generator.h"
#include "mapGenerator/ObjectGenerator.h"
#include "mapGenerator/VertexUtility.h"

#define _USE_MATH_DEFINES
#include <math.h>
#include <queue>
#include <list>
#include <vector>
#include <algorithm>
#include <stdlib.h>
#include <stdexcept>

Map* Generator::Create(const MapSettings& settings)
{
    // generate a new random map
    Map* map = GenerateMap(settings);

    // smooth textures for visual appeal
    SmoothTextures(map);

    return map;
}

void Generator::SetHill(Map* map, const Vec2& center, int z)
{
    std::vector<int> neighbors = VertexUtility::GetNeighbors(center.x, center.y, map->width, map->height, z);
    for (std::vector<int>::iterator it = neighbors.begin(); it != neighbors.end(); ++it)
    {
        const int x2 = *it % map->width, y2 = *it / map->width;
        const double d = VertexUtility::Distance(center.x, center.y, x2, y2, map->width, map->height);
        map->vertex[*it].z = std::max((int)(z - d), map->vertex[*it].z);
    }
}

unsigned int Generator::ComputeWaterSize(Map* map, const Vec2& position, const unsigned int max)
{
    const int width = map->width;
    const int height = map->height;
    
    std::queue<Vec2> searchRoom;
    std::list<Vec2> water;
    
    searchRoom.push(Vec2(position.x, position.y));
    
    while (!searchRoom.empty() && water.size() < max)
    {
        Vec2 pos = searchRoom.front(); searchRoom.pop();
        const int index = VertexUtility::GetIndexOf(pos.x, pos.y, width, height);
        
        if (ObjectGenerator::IsTexture(map->vertex[index].texture, TT_WATER)
            && std::find(water.begin(), water.end(), pos) == water.end())
        {
            water.push_back(pos);
            
            searchRoom.push(Vec2(pos.x+1, pos.y));
            searchRoom.push(Vec2(pos.x, pos.y+1));
            searchRoom.push(Vec2(pos.x-1, pos.y));
            searchRoom.push(Vec2(pos.x, pos.y-1));
        }
    }
    
    return water.size();
}


void Generator::SmoothTextures(Map* map)
{
    const int waterId = ObjectGenerator::GetTextureId(TT_WATER);
    const int width = map->width;
    const int height = map->height;
    
    // fixed broken textures
    for (int x = 0; x < width; x++)
    {
        for (int y = 0; y < height; y++)
        {
            const int index = VertexUtility::GetIndexOf(x, y, width, height);
            const int texLeft = map->vertex[VertexUtility::GetIndexOf(x - 1, y, width, height)].texture.second;
            const int texBottom = map->vertex[VertexUtility::GetIndexOf(x, y + 1, width, height)].texture.second;
            const int tex = map->vertex[index].texture.first;
            
            if (tex != texLeft && tex != texBottom && texLeft == texBottom && texBottom != waterId)
            {
                map->vertex[index].texture.first = texBottom;
            }
        }
    }
    
    for (int x = 0; x < width; x++)
    {
        for (int y = 0; y < height; y++)
        {
            const int index = VertexUtility::GetIndexOf(x, y, width, height);
            const int texRight = map->vertex[VertexUtility::GetIndexOf(x + 1, y, width, height)].texture.first;
            const int texTop = map->vertex[VertexUtility::GetIndexOf(x, y - 1, width, height)].texture.first;
            const int tex = map->vertex[index].texture.second;

            if (tex != texTop && tex != texRight && texTop == texRight && texTop != waterId)
            {
                map->vertex[index].texture.second = texTop;
            }
        }
    }
    
    // increase elevation of mountains to visually outline height of mountains
    for (int x = 0; x < width; x++)
    {
        for (int y = 0; y < height; y++)
        {
            const int index = VertexUtility::GetIndexOf(x, y, width, height);
            if (ObjectGenerator::IsTexture(map->vertex[index].texture, TT_MOUNTAIN1) ||
                ObjectGenerator::IsTexture(map->vertex[index].texture, TT_SNOW))
            {
                map->vertex[index].z = (int)(1.33 * map->vertex[index].z);
            }
        }
    }

    // replace mountain-meadow without mountain by meadow
    for (int x = 0; x < width; x++)
    {
        for (int y = 0; y < height; y++)
        {
            const int index = VertexUtility::GetIndexOf(x, y, width, height);
            if (ObjectGenerator::IsTexture(map->vertex[index].texture, TT_MOUNTAINMEADOW))
            {
                bool mountainNeighbor = false;
                std::vector<int> neighbors = VertexUtility::GetNeighbors(x, y, width, height, 1);
                for (std::vector<int>::iterator it = neighbors.begin(); it != neighbors.end(); ++it)
                {
                    if (ObjectGenerator::IsTexture(map->vertex[*it].texture, TT_MOUNTAIN1))
                    {
                        mountainNeighbor = true; break;
                    }
                }
                
                if (!mountainNeighbor)
                {
                    map->vertex[index].texture = ObjectGenerator::CreateTexture(TT_MEADOW1);
                }
            }
        }
    }
}

void Generator::SetHarbour(Map* map, const Vec2& center, const int waterLevel)
{
    for (int x = center.x - 2; x <= center.x + 2; x++)
    {
        for (int y = center.y - 2; y <= center.y + 2; y++)
        {
            const int index = VertexUtility::GetIndexOf(x, y, map->width, map->height);
            if (!ObjectGenerator::IsTexture(map->vertex[index].texture, TT_WATER))
            {
                if ((x - center.x) * (x - center.x) <= 1 && (y - center.y) * (y - center.y) <= 1)
                {
                    map->vertex[index].texture = ObjectGenerator::CreateTexture(TT_SAVANNAH, true);
                    map->vertex[index].z = waterLevel;
                    map->vertex[index].object = ObjectGenerator::CreateEmpty();
                    map->vertex[index].resource = 0x00;
                }
                else
                {
                    map->vertex[index].texture = ObjectGenerator::CreateTexture(TT_STEPPE);
                    map->vertex[index].z = waterLevel;
                    map->vertex[index].object = ObjectGenerator::CreateEmpty();
                    map->vertex[index].resource = 0x00;
                }
            }
        }
    }
}

void Generator::SetTree(Map* map, const Vec2& position)
{
    const int index = VertexUtility::GetIndexOf(position.x, position.y, map->width, map->height);
    
    if (ObjectGenerator::IsEmpty(map->vertex[index].object))
    {
        if (ObjectGenerator::IsTexture(map->vertex[index].texture, TT_DESERT))
        {
            map->vertex[index].object = ObjectGenerator::CreateRandomPalm();
        }
        else if (ObjectGenerator::IsTexture(map->vertex[index].texture, TT_SAVANNAH) ||
                 ObjectGenerator::IsTexture(map->vertex[index].texture, TT_STEPPE))
        {
            map->vertex[index].object = ObjectGenerator::CreateRandomPalm();
        }
        else if (!ObjectGenerator::IsTexture(map->vertex[index].texture, TT_WATER))
        {
            map->vertex[index].object = ObjectGenerator::CreateRandomTree();
        }
    }
}

void Generator::SetStones(Map* map, const Vec2& center, const double radius)
{
    const int width = map->width;
    const int height = map->height;
    const int cx = center.x, cy = center.y, r = (int)radius;
    
    for (int x = cx - r; x < cx + r; x++)
    {
        for (int y = cy - r; y <cy + r; y++)
        {
            if (VertexUtility::Distance(cx, cy, x, y, width, height) < radius)
            {
                SetStone(map, Vec2(x, y));
            }
        }
    }
}

void Generator::SetStone(Map* map, const Vec2& position)
{
    const int index = VertexUtility::GetIndexOf(position.x, position.y, map->width, map->height);
    
    if (ObjectGenerator::IsEmpty(map->vertex[index].object) &&
        !ObjectGenerator::IsTexture(map->vertex[index].texture, TT_WATER))
    {
        map->vertex[index].object = ObjectGenerator::CreateRandomStone();
    }
}

Vec2 Generator::ComputePointOnCircle(const int index,
                                     const int points,
                                     const Vec2& center,
                                     const double radius)
{
    Vec2 point;
    
    // compute angle according to index
    double angle = index * 2.0 * M_PI / points;
    
    // compute point position via cos/sin
    point.x = center.x + (int) (radius * cos(angle));
    point.y = center.y + (int) (radius * sin(angle));
    
    return point;
}

