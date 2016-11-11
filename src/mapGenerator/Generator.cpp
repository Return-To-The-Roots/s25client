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

#ifndef PI
#define PI 3.14159265
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

void Generator::SetTrees(Map* map, const Vec2& center, const float radius)
{
    // cast radius to integer for performance in loop
    int iRadius = (int)radius + 1;
    
    for (int x = center.x - iRadius; x < center.x + iRadius; x++)
    {
        for (int y = center.y - iRadius; y < center.y + iRadius; y++)
        {
            // check for edge of the map reached
            if (x < 0) x = map->width - x;
            if (y < 0) y = map->height - y;
            
            // compute difference to center point
            int diff_x = center.x - x;
            int diff_y = center.y - y;
            
            // compute distance to center point
            float dist = (float)std::sqrt(diff_x * diff_x + diff_y * diff_y);
            
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
        }
    }
}

void Generator::SetStone(Map* map, const Vec2& center, const float radius)
{
    // cast radius to integer for performance in loop
    int iRadius = (int)radius + 1;
    
    for (int x = center.x - iRadius; x < center.x + iRadius; x++)
    {
        for (int y = center.y - iRadius; y < center.y + iRadius; y++)
        {
            // check for edge of the map reached
            if (x < 0) x = map->width - x;
            if (y < 0) y = map->height - y;
  
            // compute difference to center point
            int diff_x = center.x - x;
            int diff_y = center.y - y;
            
            // compute distance to center point
            float dist = (float)std::sqrt(diff_x * diff_x + diff_y * diff_y);
            
            // check if the current point is inside of the radius
            if (dist < radius && map->vertex[y * map->width + x].objectType == 0x00)
            {
                float scale = 1.0F - dist / radius;
                int f = std::min(5, std::max((int)(scale * 4.49) + 1, 0));
                
                // set object type & info to a random stone object
                map->vertex[y * map->width + x].objectType = 0x01 + f;
                map->vertex[y * map->width + x].objectInfo = 0xCC + rand() % 2;
            }
        }
    }
}

Vec2 Generator::PointOnCircle(const int index, const int points, const Vec2& center, const float radius)
{
    Vec2 point;
    
    // compute angle according to index
    double angle = index * 2.0 * PI / points;
    
    // compute point position via cos/sin
    point.x = center.x + (int) (radius * cos(angle));
    point.y = center.y + (int) (radius * sin(angle));
    
    return point;
}

