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

#define _USE_MATH_DEFINES

#include "defines.h" // IWYU pragma: keep
#include "mapGenerator/MapUtility.h"
#include "mapGenerator/Map.h"
#include "mapGenerator/ObjectGenerator.h"
#include "mapGenerator/VertexUtility.h"
#include "gameData/TerrainData.h"

#include <algorithm>
#include <cmath>
#include <list>
#include <queue>
#include <stdexcept>
#include <stdlib.h>
#include <vector>

void MapUtility::SetHill(Map& map, const Position& center, int z)
{
    std::vector<int> neighbors = VertexUtility::GetNeighbors(center, map.size, z);
    for(std::vector<int>::iterator it = neighbors.begin(); it != neighbors.end(); ++it)
    {
        const Position neighbor = VertexUtility::GetPosition(*it, map.size);
        const double d = VertexUtility::Distance(center, neighbor, map.size);
        map.z[*it] = std::max((unsigned char)(z - d), (unsigned char)map.z[*it]);
    }
}

unsigned MapUtility::GetBodySize(Map& map, const Position& p, unsigned max)
{
    // compute index of the initial position
    int index = VertexUtility::GetIndexOf(p, map.size);

    // figure out terrain type of the initial position
    TerrainType type = TerrainData::MapIdx2Terrain(map.textureRsu[index]);

    std::queue<Position> searchSpace;
    std::list<int> body;

    // put initial position to the search space
    searchSpace.push(p);

    // stop search if no further neighbors are available or
    // the maximum the body size is reached
    while(!searchSpace.empty() && body.size() < max)
    {
        // get and remove the last element from the queue
        Position pos = searchSpace.front();
        searchSpace.pop();

        // compute the index of the current element
        index = VertexUtility::GetIndexOf(pos, map.size);

        // check if the element has the right terrain and is not yet
        // part of the terrain body
        if(ObjectGenerator::IsTexture(map, index, type) && std::find(body.begin(), body.end(), index) == body.end())
        {
            // add the current element to the body
            body.push_back(index);

            // push neighbor elements to the search space
            searchSpace.push(Position(pos.x + 1, pos.y));
            searchSpace.push(Position(pos.x, pos.y + 1));
            searchSpace.push(Position(pos.x - 1, pos.y));
            searchSpace.push(Position(pos.x, pos.y - 1));
        }
    }

    return body.size();
}

void MapUtility::Smooth(Map& map)
{
    const unsigned char waterId = TerrainData::GetTextureIdentifier(TT_WATER);

    // fixed broken textures
    for(int x = 0; x < map.size.x; x++)
    {
        for(int y = 0; y < map.size.y; y++)
        {
            int index = VertexUtility::GetIndexOf(Position(x, y), map.size);
            int indexLeft = VertexUtility::GetIndexOf(Position(x - 1, y), map.size);
            int indexBottom = VertexUtility::GetIndexOf(Position(x, y + 1), map.size);

            int texLeft = map.textureLsd[indexLeft];
            int texBottom = map.textureLsd[indexBottom];
            int tex = map.textureRsu[index];

            if(tex != texLeft && tex != texBottom && texLeft == texBottom && texBottom != waterId)
            {
                map.textureRsu[index] = texBottom;
            }
        }
    }

    for(int x = 0; x < map.size.x; x++)
    {
        for(int y = 0; y < map.size.y; y++)
        {
            int index = VertexUtility::GetIndexOf(Position(x, y), map.size);
            int indexRight = VertexUtility::GetIndexOf(Position(x + 1, y), map.size);
            int indexTop = VertexUtility::GetIndexOf(Position(x, y - 1), map.size);

            int texRight = map.textureRsu[indexRight];
            int texTop = map.textureRsu[indexTop];
            int tex = map.textureLsd[index];

            if(tex != texTop && tex != texRight && texTop == texRight && texTop != waterId)
            {
                map.textureLsd[index] = texTop;
            }
        }
    }

    // increase elevation of mountains to visually outline height of mountains
    RTTR_FOREACH_PT(Position, map.size)
    {
        int index = VertexUtility::GetIndexOf(pt, map.size);
        if(ObjectGenerator::IsTexture(map, index, TT_MOUNTAIN1) || ObjectGenerator::IsTexture(map, index, TT_SNOW))
        {
            map.z[index] = (int)(1.33 * map.z[index]);
        }
    }

    // replace mountain-meadow without mountain by meadow
    RTTR_FOREACH_PT(Position, map.size)
    {
        int index = VertexUtility::GetIndexOf(pt, map.size);
        if(ObjectGenerator::IsTexture(map, index, TT_MOUNTAINMEADOW))
        {
            bool mountainNeighbor = false;
            std::vector<int> neighbors = VertexUtility::GetNeighbors(pt, map.size, 1);
            for(std::vector<int>::iterator it = neighbors.begin(); it != neighbors.end(); ++it)
            {
                if(ObjectGenerator::IsTexture(map, *it, TT_MOUNTAIN1))
                {
                    mountainNeighbor = true;
                    break;
                }
            }

            if(!mountainNeighbor)
            {
                ObjectGenerator::CreateTexture(map, index, TT_MEADOW1);
            }
        }
    }
}

void MapUtility::SetHarbour(Map& map, const Position& center, int waterLevel)
{
    for(int x = center.x - 3; x <= center.x + 3; x++)
    {
        for(int y = center.y - 3; y <= center.y + 3; y++)
        {
            int index = VertexUtility::GetIndexOf(Position(x, y), map.size);
            if(!ObjectGenerator::IsTexture(map, index, TT_WATER))
            {
                if((x - center.x) * (x - center.x) <= 1.7 && (y - center.y) * (y - center.y) <= 1.7)
                {
                    ObjectGenerator::CreateTexture(map, index, TT_SAVANNAH, true);
                    ObjectGenerator::CreateEmpty(map, index);
                    map.z[index] = waterLevel;
                    map.resource[index] = libsiedler2::R_None;
                } else
                {
                    ObjectGenerator::CreateTexture(map, index, TT_STEPPE);
                    ObjectGenerator::CreateEmpty(map, index);
                    map.z[index] = waterLevel;
                    map.resource[index] = libsiedler2::R_None;
                }
            }
        }
    }
}

void MapUtility::SetTree(Map& map, ObjectGenerator& objGen, const Position& position)
{
    int index = VertexUtility::GetIndexOf(position, map.size);

    if(ObjectGenerator::IsEmpty(map, index))
    {
        if(ObjectGenerator::IsTexture(map, index, TT_DESERT) || ObjectGenerator::IsTexture(map, index, TT_SAVANNAH)
           || ObjectGenerator::IsTexture(map, index, TT_STEPPE))
        {
            objGen.CreateRandomPalm(map, index);
        } else if(!ObjectGenerator::IsTexture(map, index, TT_WATER))
        {
            objGen.CreateRandomTree(map, index);
        }
    }
}

void MapUtility::SetStones(Map& map, ObjectGenerator& objGen, const Position& center, double radius)
{
    int cx = center.x;
    int cy = center.y;
    int r = (int)radius;

    for(int x = cx - r; x < cx + r; x++)
    {
        for(int y = cy - r; y < cy + r; y++)
        {
            Position p(x, y);
            if(VertexUtility::Distance(center, p, map.size) < radius)
            {
                SetStone(map, objGen, p);
            }
        }
    }
}

void MapUtility::SetStone(Map& map, ObjectGenerator& objGen, const Position& position)
{
    int index = VertexUtility::GetIndexOf(position, map.size);

    if(ObjectGenerator::IsEmpty(map, index) && !ObjectGenerator::IsTexture(map, index, TT_WATER))
    {
        objGen.CreateRandomStone(map, index);
    }
}

Position MapUtility::ComputePointOnCircle(int index, int points, const Position& center, double radius)
{
    // compute angle according to index
    double angle = index * 2.0 * M_PI / points;

    // compute point position via cos/sin
    Position point = center + Position(Point<double>::all(radius) * Point<double>(cos(angle), sin(angle)));
    return point;
}
