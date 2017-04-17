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

#include "mapGenerator/MapUtility.h"
#include "mapGenerator/ObjectGenerator.h"
#include "mapGenerator/VertexUtility.h"
#include "gameData/TerrainData.h"

#define _USE_MATH_DEFINES
#include <math.h>
#include <queue>
#include <list>
#include <vector>
#include <algorithm>
#include <stdlib.h>
#include <stdexcept>

void MapUtility::SetHill(Map& map, const Point<uint16_t>& center, int z)
{
    std::vector<int> neighbors = VertexUtility::GetNeighbors(center.x, center.y, map.width, map.height, z);
    for (std::vector<int>::iterator it = neighbors.begin(); it != neighbors.end(); ++it)
    {
        const int x2 = *it % map.width, y2 = *it / map.width;
        const double d = VertexUtility::Distance(center.x, center.y, x2, y2, map.width, map.height);
        map.z[*it] = std::max((unsigned char)(z - d), (unsigned char)map.z[*it]);
    }
}

unsigned int MapUtility::GetBodySize(Map& map,
                                     int x,
                                     int y,
                                     unsigned int max)
{
    const int width = map.width;
    const int height = map.height;
    
    // compute index of the initial position
    int index = VertexUtility::GetIndexOf(x, y, width, height);

    // figure out terrain type of the initial position
    TerrainType type = TerrainData::MapIdx2Terrain(map.textureRsu[index]);
    
    std::queue<Point<uint16_t> > searchSpace;
    std::list<int> body;
    
    // put intial position to the search space
    searchSpace.push(Point<uint16_t>(x, y));
    
    // stop search if no further neighbors are available or
    // the maximum the body size is reached
    while (!searchSpace.empty() && body.size() < max)
    {
        // get and remove the last element from the queue
        Point<uint16_t> pos = searchSpace.front();
        searchSpace.pop();
        
        // compute the index of the current element
        index = VertexUtility::GetIndexOf(pos.x, pos.y, width, height);
        
        // check if the element has the right terrain and is not yet
        // part of the terrain body
        if (ObjectGenerator::IsTexture(map, index, type) &&
            std::find(body.begin(), body.end(), index) == body.end())
        {
            // add the current element to the body
            body.push_back(index);
            
            // push neighbor elements to the search space
            searchSpace.push(Point<uint16_t>(pos.x+1, pos.y));
            searchSpace.push(Point<uint16_t>(pos.x, pos.y+1));
            searchSpace.push(Point<uint16_t>(pos.x-1, pos.y));
            searchSpace.push(Point<uint16_t>(pos.x, pos.y-1));
        }
    }
    
    return body.size(); 
}

void MapUtility::Smooth(Map& map)
{
    const int width = map.width;
    const int height = map.height;
    const unsigned char waterId = TerrainData::GetTextureIdentifier(TT_WATER);
    
    // fixed broken textures
    for (int x = 0; x < width; x++)
    {
        for (int y = 0; y < height; y++)
        {
            const int index = VertexUtility::GetIndexOf(x, y, width, height);
            const int texLeft = map.textureLsd[VertexUtility::GetIndexOf(x - 1, y, width, height)];
            const int texBottom = map.textureLsd[VertexUtility::GetIndexOf(x, y + 1, width, height)];
            const int tex = map.textureRsu[index];
            
            if (tex != texLeft && tex != texBottom && texLeft == texBottom && texBottom != waterId)
            {
                map.textureRsu[index] = texBottom;
            }
        }
    }
    
    for (int x = 0; x < width; x++)
    {
        for (int y = 0; y < height; y++)
        {
            const int index = VertexUtility::GetIndexOf(x, y, width, height);
            const int texRight = map.textureRsu[VertexUtility::GetIndexOf(x + 1, y, width, height)];
            const int texTop = map.textureRsu[VertexUtility::GetIndexOf(x, y - 1, width, height)];
            const int tex = map.textureLsd[index];

            if (tex != texTop && tex != texRight && texTop == texRight && texTop != waterId)
            {
                map.textureLsd[index] = texTop;
            }
        }
    }
    
    // increase elevation of mountains to visually outline height of mountains
    for (int x = 0; x < width; x++)
    {
        for (int y = 0; y < height; y++)
        {
            const int index = VertexUtility::GetIndexOf(x, y, width, height);
            if (ObjectGenerator::IsTexture(map, index, TT_MOUNTAIN1) ||
                ObjectGenerator::IsTexture(map, index, TT_SNOW))
            {
                map.z[index] = (int)(1.33 * map.z[index]);
            }
        }
    }

    // replace mountain-meadow without mountain by meadow
    for (int x = 0; x < width; x++)
    {
        for (int y = 0; y < height; y++)
        {
            const int index = VertexUtility::GetIndexOf(x, y, width, height);
            if (ObjectGenerator::IsTexture(map, index, TT_MOUNTAINMEADOW))
            {
                bool mountainNeighbor = false;
                std::vector<int> neighbors = VertexUtility::GetNeighbors(x, y, width, height, 1);
                for (std::vector<int>::iterator it = neighbors.begin(); it != neighbors.end(); ++it)
                {
                    if (ObjectGenerator::IsTexture(map, *it, TT_MOUNTAIN1))
                    {
                        mountainNeighbor = true; break;
                    }
                }
                
                if (!mountainNeighbor)
                {
                    ObjectGenerator::CreateTexture(map, index, TT_MEADOW1);
                }
            }
        }
    }
}

void MapUtility::SetHarbour(Map& map, const Point<uint16_t>& center, int waterLevel)
{
    for (int x = center.x - 3; x <= center.x + 3; x++)
    {
        for (int y = center.y - 3; y <= center.y + 3; y++)
        {
            const int index = VertexUtility::GetIndexOf(x, y, map.width, map.height);
            if (!ObjectGenerator::IsTexture(map, index, TT_WATER))
            {
                if ((x - center.x) * (x - center.x) <= 1.7
                    && (y - center.y) * (y - center.y) <= 1.7)
                {
                    ObjectGenerator::CreateTexture(map, index, TT_SAVANNAH, true);
                    ObjectGenerator::CreateEmpty(map, index);
                    map.z[index]        = waterLevel;
                    map.resource[index] = libsiedler2::R_None;
                }
                else
                {
                    ObjectGenerator::CreateTexture(map, index, TT_STEPPE);
                    ObjectGenerator::CreateEmpty(map, index);
                    map.z[index]        = waterLevel;
                    map.resource[index] = libsiedler2::R_None;
                }
            }
        }
    }
}

void MapUtility::SetTree(Map& map, const Point<uint16_t>& position)
{
    const int index = VertexUtility::GetIndexOf(position.x, position.y, map.width, map.height);
    
    if (ObjectGenerator::IsEmpty(map, index))
    {
        if (ObjectGenerator::IsTexture(map, index, TT_DESERT) ||
            ObjectGenerator::IsTexture(map, index, TT_SAVANNAH) ||
            ObjectGenerator::IsTexture(map, index, TT_STEPPE))
        {
            ObjectGenerator::CreateRandomPalm(map, index);
        }
        else if (!ObjectGenerator::IsTexture(map, index, TT_WATER))
        {
            ObjectGenerator::CreateRandomTree(map, index);
        }
    }
}

void MapUtility::SetStones(Map& map, const Point<uint16_t>& center, double radius)
{
    const int width = map.width;
    const int height = map.height;
    const int cx = center.x, cy = center.y, r = (int)radius;
    
    for (int x = cx - r; x < cx + r; x++)
    {
        for (int y = cy - r; y <cy + r; y++)
        {
            if (VertexUtility::Distance(cx, cy, x, y, width, height) < radius)
            {
                SetStone(map, Point<uint16_t>(x, y));
            }
        }
    }
}

void MapUtility::SetStone(Map& map, const Point<uint16_t>& position)
{
    const int index = VertexUtility::GetIndexOf(position.x, position.y, map.width, map.height);
    
    if (ObjectGenerator::IsEmpty(map, index) &&
        !ObjectGenerator::IsTexture(map, index, TT_WATER))
    {
        ObjectGenerator::CreateRandomStone(map, index);
    }
}

Point<uint16_t> MapUtility::ComputePointOnCircle(int index,
                                                 int points,
                                                 const Point<uint16_t>& center,
                                                 double radius)
{
    Point<uint16_t> point;
    
    // compute angle according to index
    double angle = index * 2.0 * M_PI / points;
    
    // compute point position via cos/sin
    point.x = center.x + (int) (radius * cos(angle));
    point.y = center.y + (int) (radius * sin(angle));
    
    return point;
}

