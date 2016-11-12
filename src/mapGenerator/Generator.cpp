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

#include <algorithm>
#include <random>
#include <cmath>
#include "mapGenerator/Generator.h"
#include "mapGenerator/MapWriter.h"
#include "mapGenerator/Defines.h"

#ifndef PI
#define PI 3.14159265
#endif

// define function to iterator over a rectangle around a center point and compute the
// distance of each point to the center
// @param r integer radius (maximum distance to the center in one direction)
// @param cx x coordinate of the center point
// @param cy y coordinate of the center point
// @param w width of the entire map (to escape map overflow)
// @param h height of the entire map (to escape map overflow)
#ifndef ITER_RECT_BEGIN
#define ITER_RECT_BEGIN(r, cx, cy, w, h) \
    for (int x = cx - r; x < cx + r; x++) { \
        for (int y = cy - r; y <cy + r; y++) { \
            if (x < 0) x = w - x; \
            if (y < 0) y = h - y; \
            int diff_x = cx - x; \
            int diff_y = cy - y; \
            float dist = (float)std::sqrt(diff_x * diff_x + diff_y * diff_y);
#define ITER_RECT_END \
        } \
    }
#endif

void Generator::Create(const std::string& filePath, const MapSettings& settings)
{
    // generate a new random map
    Map* map = GenerateMap(settings);

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

void Generator::SetWater(Map* map, const Vec2& center, const float radius)
{
    ITER_RECT_BEGIN((int)radius, center.x, center.y, map->width, map->height)
    
    // check if the current point is inside of the radius
    if (dist < radius && map->vertex[y * map->width + x].objectType == 0x00)
    {
        map->vertex[y * map->width + x].rsuTexture = TRIANGLE_TEXTURE_WATER;
        map->vertex[y * map->width + x].usdTexture = TRIANGLE_TEXTURE_WATER;
    }
    
    ITER_RECT_END
}

void Generator::SetTrees(Map* map, const Vec2& center, const float radius)
{
    ITER_RECT_BEGIN((int)radius, center.x, center.y, map->width, map->height)
    
    // check if the current point is inside of the radius
    if (dist < radius && map->vertex[y * map->width + x].objectType == 0x00)
    {
        float f = (1.0F - dist / radius);
        int p = std::max(1, (int)(100 * f * f));
        if (rand() % p > 10)
        {
            int treeType;
            int rnd = rand()%3;
            switch (rnd)
            {
                case 0: treeType = 0x30; break;
                case 1: treeType = 0x70; break;
                case 2: treeType = 0xB0; break;
            }
            
            // set object type & info to a random tree object
            map->vertex[y * map->width + x].objectType = treeType + rand() % 8;
            map->vertex[y * map->width + x].objectInfo = 0xC4;
        }
    }
    
    ITER_RECT_END
}

void Generator::SetStone(Map* map, const Vec2& center, const float radius)
{
    ITER_RECT_BEGIN((int)radius, center.x, center.y, map->width, map->height)
    
    // check if the current point is inside of the radius
    if (dist < radius && map->vertex[y * map->width + x].objectType == 0x00)
    {
        float scale = 1.0F - dist / radius;
        int f = std::min(5, std::max((int)(scale * 4.49) + 1, 0));
        
        // set object type & info to a random stone object
        map->vertex[y * map->width + x].objectType = 0x01 + f;
        map->vertex[y * map->width + x].objectInfo = 0xCC + rand() % 2;
    }

    ITER_RECT_END
}

Vec2 Generator::ComputePointOnCircle(const int index,
                                     const int points,
                                     const Vec2& center,
                                     const float radius)
{
    Vec2 point;
    
    // compute angle according to index
    double angle = index * 2.0 * PI / points;
    
    // compute point position via cos/sin
    point.x = center.x + (int) (radius * cos(angle));
    point.y = center.y + (int) (radius * sin(angle));
    
    return point;
}

