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

#include "rttrDefines.h" // IWYU pragma: keep
#include "mapGenerator/RandomMapGenerator.h"
#include "mapGenerator/ObjectGenerator.h"
#include "mapGenerator/RandomConfig.h"
#include "mapGenerator/VertexUtility.h"
#include "world/MapGeometry.h"
#include "gameData/TerrainDesc.h"
#include <algorithm>
#include <cmath>
#include <cstdlib>

// harbor placement
#define MIN_HARBOR_DISTANCE 35.0
#define MIN_HARBOR_WATER 200

RandomMapGenerator::RandomMapGenerator(RandomConfig& config) : config(config), helper(config) {}

unsigned RandomMapGenerator::GetMaxTerrainHeight(const DescIdx<TerrainDesc> terrain)
{
    for(int i = config.textures.size() - 1; i >= 0; --i)
    {
        if(config.textures[i] == terrain)
            return i;
    }
    return config.textures.size();
}

unsigned RandomMapGenerator::GetMinTerrainHeight(const DescIdx<TerrainDesc> terrain)
{
    return static_cast<unsigned>(std::find(config.textures.begin(), config.textures.end(), terrain) - config.textures.begin());
}

void RandomMapGenerator::PlacePlayers(const MapSettings& settings, Map& map)
{
    const int length = std::min(map.size.x, map.size.y) / 2;

    // compute center of the map
    Position center(map.size / 2);

    // radius for player distribution
    const double rnd = config.DRand(settings.minPlayerRadius * length, settings.maxPlayerRadius * length);

    // player headquarters for the players
    for(unsigned i = 0; i < settings.numPlayers; i++)
    {
        // compute headquarter position
        Position position = helper.ComputePointOnCircle(i, settings.numPlayers, center, rnd);

        // store headquarter position
        map.hqPositions[i] = MapPoint(position);

        // create headquarter
        helper.objGen.CreateHeadquarter(map, VertexUtility::GetIndexOf(position, map.size), i);
    }
}

void RandomMapGenerator::PlacePlayerResources(const MapSettings& settings, Map& map)
{
    for(unsigned i = 0; i < settings.numPlayers; i++)
    {
        int offset1 = config.Rand(0, 180);
        int offset2 = config.Rand(180, 360);
        const Position p(map.hqPositions[i]);

        helper.SetStones(map, helper.ComputePointOnCircle(offset1, 360, p, 12), 2.0F);
        helper.SetStones(map, helper.ComputePointOnCircle(offset2, 360, p, 12), 2.7F);
    }
}

void RandomMapGenerator::CreateHills(const MapSettings& settings, Map& map)
{
    const int players = settings.numPlayers;
    std::vector<AreaDesc> areas = config.areas;

    for(int x = 0; x < map.size.x; x++)
    {
        for(int y = 0; y < map.size.y; y++)
        {
            double distanceToPlayer = (double)(map.size.x + map.size.y);
            Position tile(x, y);

            for(int i = 0; i < players; i++)
            {
                distanceToPlayer = std::min(distanceToPlayer, VertexUtility::Distance(tile, Position(map.hqPositions[i]), map.size));
            }

            for(std::vector<AreaDesc>::iterator it = areas.begin(); it != areas.end(); ++it)
            {
                if(it->IsInArea(tile, distanceToPlayer, map.size))
                {
                    const int pr = (int)it->likelyhoodHill;
                    const int maxZ = it->maxElevation;

                    if(maxZ > 0 && config.Rand(101) <= pr)
                    {
                        unsigned z = (unsigned)config.Rand(it->minElevation, maxZ + 1);
                        helper.SetHill(map, tile, z);
                    }
                }
            }
        }
    }
}

void RandomMapGenerator::FillRemainingTerrain(const MapSettings& settings, Map& map)
{
    const int players = settings.numPlayers;
    std::vector<AreaDesc> areas = config.areas;
    std::vector<DescIdx<TerrainDesc> > textures = config.textures;

    RTTR_FOREACH_PT(Position, map.size)
    {
        const int index = VertexUtility::GetIndexOf(pt, map.size);
        const int level = map.z[index];

        // create texture for current height value
        helper.objGen.CreateTexture(map, index, textures[level]);

        double distanceToPlayer = (double)(map.size.x + map.size.y);

        for(int i = 0; i < players; i++)
            distanceToPlayer = std::min(distanceToPlayer, VertexUtility::Distance(pt, Position(map.hqPositions[i]), map.size));

        for(std::vector<AreaDesc>::iterator it = areas.begin(); it != areas.end(); ++it)
        {
            if(it->IsInArea(pt, distanceToPlayer, map.size))
            {
                if(static_cast<unsigned>(config.Rand(0, 100)) < it->likelyhoodTree)
                    helper.SetTree(map, pt);
                else if(static_cast<unsigned>(config.Rand(0, 100)) < it->likelyhoodStone)
                    helper.SetStone(map, pt);
            }
        }
    }
    // post-processing of texture (add animals, adapt height, ...)
    RTTR_FOREACH_PT(Position, map.size)
    {
        const int index = VertexUtility::GetIndexOf(pt, map.size);
        const int level = map.z[index];
        const TerrainDesc& t = config.worldDesc.get(textures[level]);
        if(t.kind == TerrainKind::WATER)
        {
            map.z[index] = GetMaxTerrainHeight(textures[level]);
            map.animal[index] = helper.objGen.CreateDuck(3);
        } else if(t.humidity > 0 && t.IsUsableByAnimals())
        {
            std::vector<int> positions = VertexUtility::GetNeighbors(pt, map.size, 1);
            bool treeFound = false;
            BOOST_FOREACH(int curIdx, positions)
            {
                if(helper.objGen.IsTree(map, curIdx))
                {
                    treeFound = true;
                    break;
                }
            }
            map.animal[index] = treeFound ? helper.objGen.CreateRandomForestAnimal(4) : helper.objGen.CreateSheep(4);
        }
    }

    ///////
    /// Harbour placement
    ///////
    std::vector<Position> harbors;
    int maxWaterIndex = -1;
    for(unsigned i = 0; i < textures.size(); i++)
    {
        if(config.worldDesc.get(textures[i]).kind == TerrainKind::WATER)
            maxWaterIndex = i;
        else
            break;
    }

    if(maxWaterIndex > 0)
    {
        RTTR_FOREACH_PT(Position, map.size)
        {
            const int index = VertexUtility::GetIndexOf(pt, map.size);

            // under certain circumstances replace coast texture by harbor position
            if(map.z[index] != maxWaterIndex + 1)
                continue;
            Position water(0, 0);
            // ensure there's water close to the coast texture
            bool waterNeighbor = false;
            std::vector<int> neighbors = VertexUtility::GetNeighbors(pt, map.size, 1);
            for(std::vector<int>::iterator it = neighbors.begin(); it != neighbors.end(); ++it)
            {
                if(helper.objGen.IsTexture(map, *it, textures[maxWaterIndex]))
                {
                    waterNeighbor = true;
                    water = VertexUtility::GetPosition(*it, map.size);
                    break;
                }
            }
            if(!waterNeighbor)
                continue;

            // ensure there's no other harbor nearby
            double closestHarbor = MIN_HARBOR_DISTANCE + 1.0;
            for(std::vector<Position>::iterator it = harbors.begin(); it != harbors.end(); ++it)
            {
                closestHarbor = std::min(closestHarbor, VertexUtility::Distance(pt, *it, map.size));
            }

            if(closestHarbor < MIN_HARBOR_DISTANCE)
                continue;

            // setup harbor position
            if(helper.GetBodySize(map, water, MIN_HARBOR_WATER) >= MIN_HARBOR_WATER)
            {
                helper.SetHarbour(map, pt, maxWaterIndex);
                harbors.push_back(pt);
            }
        }
    }
}

void RandomMapGenerator::SetResources(const MapSettings& settings, Map& map)
{
    RTTR_FOREACH_PT(Position, map.size)
    {
        const int index = VertexUtility::GetIndexOf(pt, map.size);
        const TerrainDesc& tRsu = config.GetTerrainByS2Id(map.textureRsu[index]);
        const TerrainDesc& tLsd = config.GetTerrainByS2Id(map.textureLsd[index]);

        uint8_t res = libsiedler2::R_None;

        if(tRsu.kind == TerrainKind::WATER && tLsd.kind == TerrainKind::WATER) {
                res = libsiedler2::R_Fish;
        } else if(tRsu.IsVital() && tLsd.IsVital())
        {
            int nb = VertexUtility::GetIndexOf(GetNeighbour(pt, Direction::NORTHWEST), map.size);
            const TerrainDesc& t1 = config.GetTerrainByS2Id(map.textureRsu[nb]);
            const TerrainDesc& t2 = config.GetTerrainByS2Id(map.textureLsd[nb]);
            nb = VertexUtility::GetIndexOf(GetNeighbour(pt, Direction::NORTHEAST), map.size);
            const TerrainDesc& t3 = config.GetTerrainByS2Id(map.textureLsd[nb]);
            nb = VertexUtility::GetIndexOf(GetNeighbour(pt, Direction::EAST), map.size);
            const TerrainDesc& t4 = config.GetTerrainByS2Id(map.textureRsu[nb]);
            // Less strict check: Include all terrain that can also be used by animals
            if(tRsu.humidity > 0 && tLsd.humidity > 0 && t1.IsUsableByAnimals() && t2.IsUsableByAnimals() && t3.IsUsableByAnimals()
               && t4.IsUsableByAnimals())
                res = libsiedler2::R_Water;
        } else if(tRsu.Is(ETerrain::Mineable) && tLsd.Is(ETerrain::Mineable))
        {
            int nb = VertexUtility::GetIndexOf(GetNeighbour(pt, Direction::NORTHWEST), map.size);
            const TerrainDesc& t1 = config.GetTerrainByS2Id(map.textureRsu[nb]);
            const TerrainDesc& t2 = config.GetTerrainByS2Id(map.textureLsd[nb]);
            nb = VertexUtility::GetIndexOf(GetNeighbour(pt, Direction::NORTHEAST), map.size);
            const TerrainDesc& t3 = config.GetTerrainByS2Id(map.textureLsd[nb]);
            nb = VertexUtility::GetIndexOf(GetNeighbour(pt, Direction::EAST), map.size);
            const TerrainDesc& t4 = config.GetTerrainByS2Id(map.textureRsu[nb]);
            if(t1.Is(ETerrain::Mineable) && t2.Is(ETerrain::Mineable) && t3.Is(ETerrain::Mineable)
               && t4.Is(ETerrain::Mineable))
                res = helper.objGen.CreateRandomResource(settings.ratioGold, settings.ratioIron, settings.ratioCoal, settings.ratioGranite);
        }

        map.resource[index] = res;
    }
}

Map* RandomMapGenerator::Create(MapSettings settings)
{
    settings.Validate();
    Map* map = new Map(settings.size, settings.name, settings.author);

    // configuration of the map settings
    map->type = config.worldDesc.get(settings.type).s2Id;
    map->numPlayers = settings.numPlayers;

    // the actual map generation
    PlacePlayers(settings, *map);
    PlacePlayerResources(settings, *map);
    CreateHills(settings, *map);
    FillRemainingTerrain(settings, *map);
    helper.Smooth(*map);
    SetResources(settings, *map);

    return map;
}
