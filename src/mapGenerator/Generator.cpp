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
#include "mapGenerator/MapWriter.h"
#include "mapGenerator/ObjectGenerator.h"
#include "mapGenerator/VertexUtility.h"

#define _USE_MATH_DEFINES
#include <math.h>
#include <stdlib.h>
#include <stdexcept>

// Function for iterating over a rectangle around a center point and compute the
// distance of each point to the center.
// @param r integer radius (maximum distance to the center in one direction)
// @param cx x coordinate of the center point
// @param cy y coordinate of the center point
// @param w width of the entire map (to escape map overflow)
// @param h height of the entire map (to escape map overflow)
#ifndef ITER_RECT_BEGIN
#define ITER_RECT_BEGIN(r, cx, cy, w, h) \
    for (int x = cx - r; x < cx + r; x++) { \
        for (int y = cy - r; y <cy + r; y++) { \
            double dist = VertexUtility::Distance(cx, cy, x, y, w, h);
#define ITER_RECT_END \
        } \
    }
#endif

void Generator::Create(const std::string& filePath, const MapSettings& settings)
{
    // generate a new random map
    Map* map = GenerateMap(settings);
    SmoothTextures(map);

    // create a map writer
    MapWriter* writer = new MapWriter();

    // try to write the generated map to a file
    if (!writer->Write(filePath, map))
    {
        // cleanup memory if failed
        delete map;
        delete writer;
        
        throw std::invalid_argument("Failed to write the random map to the filePath");
    }
    
    // cleanup map and writer
    delete map;
    delete writer;
}

void Generator::SmoothTextures(Map* map)
{
    const int waterId = ObjectGenerator::GetTextureId(TerrainType::TT_WATER);
    const int width = map->width;
    const int height = map->height;
    
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
}

void Generator::SetHarbour(Map* map, const Vec2& center, const int waterLevel)
{
    for (int x = center.x - 2; x <= center.x + 2; x++)
    {
        for (int y = center.y - 2; y <= center.y + 2; y++)
        {
            const int index = VertexUtility::GetIndexOf(x, y, map->width, map->height);
            if (!ObjectGenerator::IsTexture(map->vertex[index].texture, TerrainType::TT_WATER))
            {
                if ((x - center.x) * (x - center.x) <= 1 && (y - center.y) * (y - center.y) <= 1)
                {
                    map->vertex[index].texture = ObjectGenerator::CreateTexture(TerrainType::TT_SAVANNAH, true);
                    map->vertex[index].z = waterLevel;
                    map->vertex[index].object = ObjectGenerator::CreateEmpty();
                    map->vertex[index].resource = 0x00;
                }
                else
                {
                    map->vertex[index].texture = ObjectGenerator::CreateTexture(TerrainType::TT_STEPPE);
                    map->vertex[index].z = waterLevel;
                    map->vertex[index].object = ObjectGenerator::CreateEmpty();
                    map->vertex[index].resource = 0x00;
                }
            }
        }
    }
}

void Generator::SetTrees(Map* map, const Vec2& center, const double radius)
{
    ITER_RECT_BEGIN((int)radius, center.x, center.y, map->width, map->height)
    
    if (dist < radius)
    {
        // compute value representing distance to center
        double f = (1.0F - dist / radius);
        int p = (int)(100 * f * f);
        
        // lower likelyhood for tree placement with increasing distance to center
        if (p > 0 && rand() % p > 10)
        {
            SetTree(map, Vec2(x, y));
        }
    }
    
    ITER_RECT_END
}

void Generator::SetTree(Map* map, const Vec2& position)
{
    const int index = VertexUtility::GetIndexOf(position.x, position.y, map->width, map->height);
    
    if (ObjectGenerator::IsEmpty(map->vertex[index].object))
    {
        if (ObjectGenerator::IsTexture(map->vertex[index].texture, TerrainType::TT_DESERT))
        {
            map->vertex[index].object = ObjectGenerator::CreateRandomPalm();
        }
        else if (ObjectGenerator::IsTexture(map->vertex[index].texture, TerrainType::TT_SAVANNAH) ||
                 ObjectGenerator::IsTexture(map->vertex[index].texture, TerrainType::TT_STEPPE))
        {
            map->vertex[index].object = ObjectGenerator::CreateRandomPalm();
        }
        else if (!ObjectGenerator::IsTexture(map->vertex[index].texture, TerrainType::TT_WATER))
        {
            map->vertex[index].object = ObjectGenerator::CreateRandomTree();
        }
    }
}

void Generator::SetStones(Map* map, const Vec2& center, const double radius)
{
    ITER_RECT_BEGIN((int)radius, center.x, center.y, map->width, map->height)
    
    if (dist < radius)
    {
        SetStone(map, Vec2(x, y));
    }

    ITER_RECT_END
}

void Generator::SetStone(Map* map, const Vec2& position)
{
    const int index = VertexUtility::GetIndexOf(position.x, position.y, map->width, map->height);
    
    if (ObjectGenerator::IsEmpty(map->vertex[index].object) &&
        !ObjectGenerator::IsTexture(map->vertex[index].texture, TerrainType::TT_WATER))
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

