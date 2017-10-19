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

#include "defines.h" // IWYU pragma: keep
#include "mapGenerator/RandomMapGenerator.h"
#include "mapGenerator/ObjectGenerator.h"
#include "mapGenerator/RandomConfig.h"
#include "mapGenerator/VertexUtility.h"

#include <cmath>
#include <cstdlib>

// harbor placement
#define MIN_HARBOR_DISTANCE 35.0
#define MIN_HARBOR_WATER 200

RandomMapGenerator::RandomMapGenerator(RandomConfig& config) : config(config) {}

unsigned RandomMapGenerator::GetMaxTerrainHeight(const TerrainType terrain, const std::vector<TerrainType>& textures)
{
    unsigned maxHeight = 0;
    for(unsigned i = 0; i < textures.size(); i++)
    {
        if(textures[i] == terrain)
        {
            maxHeight = i;
        }
    }

    return maxHeight;
}

unsigned RandomMapGenerator::GetMinTerrainHeight(const TerrainType terrain, const std::vector<TerrainType>& textures)
{
    for(unsigned i = 0; i < textures.size(); i++)
    {
        if(textures[i] == terrain)
        {
            return i;
        }
    }

    return textures.size();
}

void RandomMapGenerator::PlacePlayers(const MapSettings& settings, Map& map)
{
    const int length = std::min(map.size.x / 2, map.size.y / 2);

    // compute center of the map
    Position center(map.size / 2);

    // radius for player distribution
    const int rMin = (int)(settings.minPlayerRadius * length);
    ;
    const int rMax = (int)(settings.maxPlayerRadius * length);
    const int rnd = config.Rand(rMin, rMax);

    // player headquarters for the players
    for(unsigned i = 0; i < settings.players; i++)
    {
        // compute headquarter position
        Position position = helper.ComputePointOnCircle(i, settings.players, center, (double)(rMin + rnd));

        // store headquarter position
        map.positions[i] = MapPoint(position);

        // create headquarter
        ObjectGenerator::CreateHeadquarter(map, VertexUtility::GetIndexOf(position, map.size), i);
    }
}

void RandomMapGenerator::PlacePlayerResources(const MapSettings& settings, Map& map)
{
    ObjectGenerator objGen(config);
    for(unsigned i = 0; i < settings.players; i++)
    {
        int offset1 = config.Rand(0, 180);
        int offset2 = config.Rand(180, 360);
        const Position p(map.positions[i]);

        helper.SetStones(map, objGen, helper.ComputePointOnCircle(offset1, 360, p, 12), 2.0F);
        helper.SetStones(map, objGen, helper.ComputePointOnCircle(offset2, 360, p, 12), 2.7F);
    }
}

void RandomMapGenerator::CreateHills(const MapSettings& settings, Map& map)
{
    const int players = settings.players;
    std::vector<AreaDesc> areas = config.areas;
    std::vector<TerrainType> textures = config.textures;

    for(int x = 0; x < map.size.x; x++)
    {
        for(int y = 0; y < map.size.y; y++)
        {
            double distanceToPlayer = (double)(map.size.x + map.size.y);
            Position tile(x, y);

            for(int i = 0; i < players; i++)
            {
                distanceToPlayer = std::min(distanceToPlayer, VertexUtility::Distance(tile, Position(map.positions[i]), map.size));
            }

            for(std::vector<AreaDesc>::iterator it = areas.begin(); it != areas.end(); ++it)
            {
                if(it->IsInArea(tile, distanceToPlayer, map.size))
                {
                    const int pr = (int)(*it).likelyhoodHill;
                    const int rnd = config.Rand(0, pr > 0 ? 101 : (int)(100.0 / (*it).likelyhoodHill));
                    const int minZ = (*it).minElevation;
                    const int maxZ = (*it).maxElevation;

                    if(maxZ > 0 && rnd <= pr)
                    {
                        const unsigned z = (unsigned)config.Rand(minZ, maxZ + 1);
                        helper.SetHill(map, tile, z == GetMinTerrainHeight(TT_MOUNTAINMEADOW, textures) ? z - 1 : z);
                    }
                }
            }
        }
    }
}

void RandomMapGenerator::FillRemainingTerrain(const MapSettings& settings, Map& map)
{
    const int players = settings.players;
    std::vector<AreaDesc> areas = config.areas;
    std::vector<TerrainType> textures = config.textures;

    ObjectGenerator objGen(config);

    for(int x = 0; x < map.size.x; x++)
    {
        for(int y = 0; y < map.size.y; y++)
        {
            const int index = VertexUtility::GetIndexOf(Position(x, y), map.size);
            const int level = map.z[index];

            // create texture for current height value
            ObjectGenerator::CreateTexture(map, index, textures[level]);

            // post-processing of texture (add animals, adapt height, ...)
            switch(textures[level])
            {
                case TT_WATER:
                    map.z[index] = GetMaxTerrainHeight(TT_WATER, textures);
                    map.animal[index] = objGen.CreateDuck(3);
                    map.resource[index] = libsiedler2::R_Fish;
                    break;
                case TT_MEADOW1: map.animal[index] = objGen.CreateRandomForestAnimal(4); break;
                case TT_MEADOW_FLOWERS: map.animal[index] = objGen.CreateSheep(4); break;
                case TT_MOUNTAIN1:
                    map.resource[index] =
                      objGen.CreateRandomResource(settings.ratioGold, settings.ratioIron, settings.ratioCoal, settings.ratioGranite);
                    break;
                default: break;
            }

            double distanceToPlayer = (double)(map.size.x + map.size.y);
            Position tile(x, y);

            for(int i = 0; i < players; i++)
            {
                distanceToPlayer = std::min(distanceToPlayer, VertexUtility::Distance(tile, Position(map.positions[i]), map.size));
            }

            for(std::vector<AreaDesc>::iterator it = areas.begin(); it != areas.end(); ++it)
            {
                if(it->IsInArea(tile, distanceToPlayer, map.size))
                {
                    if(static_cast<unsigned>(config.Rand(0, 100)) < (*it).likelyhoodTree)
                    {
                        helper.SetTree(map, objGen, tile);
                    } else if(static_cast<unsigned>(config.Rand(0, 100)) < (*it).likelyhoodStone)
                    {
                        helper.SetStone(map, objGen, tile);
                    }
                }
            }
        }
    }

    ///////
    /// Harbour placement
    ///////
    std::vector<Position> harbors;

    RTTR_FOREACH_PT(Position, map.size)
    {
        const int index = VertexUtility::GetIndexOf(pt, map.size);

        // under certain circumstances replace dessert texture by harbor position
        Position water(0, 0);
        if(ObjectGenerator::IsTexture(map, index, TT_DESERT))
        {
            // ensure there's water close to the dessert texture
            bool waterNeighbor = false;
            std::vector<int> neighbors = VertexUtility::GetNeighbors(pt, map.size, 1);
            for(std::vector<int>::iterator it = neighbors.begin(); it != neighbors.end(); ++it)
            {
                if(ObjectGenerator::IsTexture(map, *it, TT_WATER))
                {
                    waterNeighbor = true;
                    water = VertexUtility::GetPosition(*it, map.size);
                    break;
                }
            }

            // ensure there's no other harbor nearby
            double closestHarbor = MIN_HARBOR_DISTANCE + 1.0;
            for(std::vector<Position>::iterator it = harbors.begin(); it != harbors.end(); ++it)
            {
                closestHarbor = std::min(closestHarbor, VertexUtility::Distance(pt, *it, map.size));
            }

            int waterTiles = (closestHarbor >= MIN_HARBOR_DISTANCE && waterNeighbor) ? helper.GetBodySize(map, water, MIN_HARBOR_WATER) : 0;

            // setup harbor position
            if(waterTiles >= MIN_HARBOR_WATER)
            {
                helper.SetHarbour(map, pt, GetMaxTerrainHeight(TT_WATER, textures));
                harbors.push_back(pt);
            }
        }
    }
}

Map* RandomMapGenerator::Create(const MapSettings& settings)
{
    Map* map = new Map(settings.size, "Random", "auto");

    // configuration of the map settings
    map->type = settings.type;
    map->players = settings.players;

    // the actual map generation
    PlacePlayers(settings, *map);
    PlacePlayerResources(settings, *map);
    CreateHills(settings, *map);
    FillRemainingTerrain(settings, *map);

    // post-processing
    helper.Smooth(*map);

    return map;
}
