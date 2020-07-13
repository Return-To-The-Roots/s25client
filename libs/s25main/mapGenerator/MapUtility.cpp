// Copyright (c) 2017 - 2020 Settlers Freaks (sf-team at siedler25.org)
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

#include "mapGenerator/MapUtility.h"
#include "RTTR_Assert.h"
#include "RandomConfig.h"
#include "RttrForeachPt.h"
#include "mapGenerator/Map.h"
#include "mapGenerator/ObjectGenerator.h"
#include "mapGenerator/VertexUtility.h"
#include "gameData/TerrainDesc.h"
#include <libsiedler2/enumTypes.h>
#include <algorithm>
#include <cmath>
#include <queue>
#include <set>
#include <vector>

void MapUtility::SetHill(Map& map, const Position& center, int z)
{
    std::vector<int> neighbors = VertexUtility::GetNeighbors(center, map.size, z);
    for(int& it : neighbors)
    {
        const Position neighbor = VertexUtility::GetPosition(it, map.size);
        const double d = VertexUtility::Distance(center, neighbor, map.size);
        map.z[it] = std::max((unsigned char)(z - d), (unsigned char)map.z[it]);
    }
}

unsigned MapUtility::GetBodySize(Map& map, const Position& p, unsigned max)
{
    // compute index of the initial position
    int index = VertexUtility::GetIndexOf(p, map.size);

    // figure out terrain type of the initial position
    uint8_t type = map.textureRsu[index];

    std::queue<Position> searchSpace;
    std::set<int> body;

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
        if((map.textureRsu[index] == type || map.textureLsd[index] == type) && body.find(index) == body.end())
        {
            // add the current element to the body
            body.insert(index);

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

            if(tex != texLeft && tex != texBottom && texLeft == texBottom && cfg.GetTerrainByS2Id(texBottom).kind != TerrainKind::WATER)
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

            if(tex != texTop && tex != texRight && texTop == texRight && cfg.GetTerrainByS2Id(texTop).kind != TerrainKind::WATER)
            {
                map.textureLsd[index] = texTop;
            }
        }
    }

    // increase elevation of mountains to visually outline height of mountains
    RTTR_FOREACH_PT(Position, map.size)
    {
        int index = VertexUtility::GetIndexOf(pt, map.size);
        int tex = map.textureLsd[index];
        const TerrainDesc& t = cfg.GetTerrainByS2Id(tex);
        if(t.Is(ETerrain::Mineable) || t.kind == TerrainKind::SNOW)
        {
            map.z[index] = (int)(1.33 * map.z[index]);
        }
    }

    DescIdx<TerrainDesc> highestNonMountain(0);
    for(unsigned i = 1; i < cfg.landscapeTerrains.size(); i++)
    {
        if(cfg.worldDesc.get(cfg.landscapeTerrains[i]).kind == TerrainKind::MOUNTAIN)
        {
            highestNonMountain = cfg.landscapeTerrains[i - 1u];
            break;
        }
    }

    // replace mountain-meadow without mountain by meadow
    RTTR_FOREACH_PT(Position, map.size)
    {
        int index = VertexUtility::GetIndexOf(pt, map.size);
        int tex = map.textureLsd[index];
        const TerrainDesc& t = cfg.GetTerrainByS2Id(tex);
        if(t.kind == TerrainKind::MOUNTAIN && t.Is(ETerrain::Buildable))
        {
            bool mountainNeighbor = false;
            std::vector<int> neighbors = VertexUtility::GetNeighbors(pt, map.size, 1);
            for(int& neighbor : neighbors)
            {
                if(objGen.IsTexture(map, neighbor, [](const auto& desc) { return desc.Is(ETerrain::Mineable); }))
                {
                    mountainNeighbor = true;
                    break;
                }
            }

            if(!mountainNeighbor)
                objGen.CreateTexture(map, index, highestNonMountain);
        }
    }
}

void MapUtility::SetHarbour(Map& map, const Position& center, int waterLevel)
{
    DescIdx<TerrainDesc> buildable = cfg.FindTerrain([](const auto& desc) { return desc.Is(ETerrain::Buildable); });
    DescIdx<TerrainDesc> buildable2 = cfg.FindTerrain(
      [this, buildable](const auto& desc) { return desc.Is(ETerrain::Buildable) && desc.name != cfg.worldDesc.get(buildable).name; });
    if(!buildable2)
        buildable2 = buildable;

    for(int x = center.x - 3; x <= center.x + 3; x++)
    {
        for(int y = center.y - 3; y <= center.y + 3; y++)
        {
            int index = VertexUtility::GetIndexOf(Position(x, y), map.size);
            if(!objGen.IsTexture(map, index, [](const auto& desc) { return desc.kind == TerrainKind::WATER; }))
            {
                if(VertexUtility::Distance(Position(x, y), center, map.size) <= 1.7)
                {
                    objGen.CreateTexture(map, index, buildable, center == Position(x, y));
                    ObjectGenerator::CreateEmpty(map, index);
                    map.z[index] = waterLevel;
                    map.resource[index] = libsiedler2::R_None;
                } else
                {
                    objGen.CreateTexture(map, index, buildable2);
                    ObjectGenerator::CreateEmpty(map, index);
                    map.z[index] = waterLevel;
                    map.resource[index] = libsiedler2::R_None;
                }
            }
        }
    }
}

void MapUtility::SetTree(Map& map, const Position& position)
{
    int index = VertexUtility::GetIndexOf(position, map.size);

    if(ObjectGenerator::IsEmpty(map, index)
       && !objGen.IsTexture(map, index, [](const auto& desc) { return desc.kind == TerrainKind::WATER; })
       && !objGen.IsTexture(map, index, [](const auto& desc) { return desc.Is(ETerrain::Unreachable); }))
    {
        if(objGen.IsTexture(map, index, [](const auto& desc) { return desc.humidity < 90; }))
            objGen.CreateRandomPalm(map, index);
        else
            objGen.CreateRandomTree(map, index);
    }
}

void MapUtility::SetStones(Map& map, const Position& center, double radius)
{
    int cx = center.x;
    int cy = center.y;
    auto r = (int)radius;

    for(int x = cx - r; x < cx + r; x++)
    {
        for(int y = cy - r; y < cy + r; y++)
        {
            Position p(x, y);
            if(VertexUtility::Distance(center, p, map.size) < radius)
            {
                SetStone(map, p);
            }
        }
    }
}

void MapUtility::SetStone(Map& map, const Position& position)
{
    int index = VertexUtility::GetIndexOf(position, map.size);

    if(ObjectGenerator::IsEmpty(map, index)
       && !objGen.IsTexture(map, index, [](const auto& desc) { return desc.kind == TerrainKind::WATER; })
       && !objGen.IsTexture(map, index, [](const auto& desc) { return desc.Is(ETerrain::Unreachable); }))
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
