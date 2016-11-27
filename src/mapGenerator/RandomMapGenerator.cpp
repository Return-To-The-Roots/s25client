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

#include "mapGenerator/RandomMapGenerator.h"
#include "mapGenerator/ObjectGenerator.h"
#include "mapGenerator/VertexUtility.h"

#include <vector>
#include <cstdlib>
#include <cmath>

// harbor placement
#define MIN_HARBOR_DISTANCE     35.0
#define MIN_HARBOR_WATER        200

RandomMapGenerator::RandomMapGenerator(bool random)
{
    _textures[0]    = TT_WATER;
    _textures[1]    = TT_WATER;
    _textures[2]    = TT_WATER;
    _textures[3]    = TT_WATER;
    _textures[4]    = TT_DESERT;
    _textures[5]    = TT_STEPPE;
    _textures[6]    = TT_SAVANNAH;
    _textures[7]    = TT_MEADOW1;
    _textures[8]    = TT_MEADOW_FLOWERS;
    _textures[9]    = TT_MEADOW2;
    _textures[10]   = TT_MEADOW2;
    _textures[11]   = TT_MOUNTAINMEADOW;
    _textures[12]   = TT_MOUNTAIN1;
    _textures[13]   = TT_MOUNTAIN1;
    _textures[14]   = TT_MOUNTAIN1;
    _textures[15]   = TT_SNOW;
    _textures[16]   = TT_SNOW;
    _textures[17]   = TT_SNOW;
    _textures[18]   = TT_SNOW;
    _textures[19]   = TT_SNOW;
    _textures[20]   = TT_SNOW;
    _textures[21]   = TT_SNOW;
    _textures[22]   = TT_SNOW;
    _textures[23]   = TT_SNOW;
    _textures[24]   = TT_SNOW;
    
    if (!random)
    {
        return;
    }
    
    const double p1 = DRand(0.0, 0.4);
    const double p2 = DRand(p1, p1 + 1.4);
    const double p3 = DRand(p2, p2 + 1.0);
    const double pHill = DRand(1.5, 5.0);
    const int minHill = rand() % 5;
    
    _areas.push_back(AreaDesc(0.5, 0.5, 0.0, p1,    1.0,  4, 7, 0, 23, 15));
    _areas.push_back(AreaDesc(0.5, 0.5, p1,  p2,    pHill, 18, 5, minHill, 10, 15));
    _areas.push_back(AreaDesc(0.5, 0.5, p1,  p2,    0.5,  0, 0, 0, 17, 18));
    _areas.push_back(AreaDesc(0.5, 0.5, p2,  p3,    0.1, 15, 5, 0,  7, 15));
    _areas.push_back(AreaDesc(0.5, 0.5, 0.0, 2.0, 100.0,  0, 0, 7,  7,  0, 4));
    _areas.push_back(AreaDesc(0.5, 0.5, 0.0, 2.0, 100.0,  8, 0, 5, 10,  4, 15));
}

int RandomMapGenerator::GetMaxTerrainHeight(const TerrainType terrain)
{
    int maxHeight = -1;
    for (int i = 0; i < MAXIMUM_HEIGHT; i++)
    {
        if (_textures[i] == terrain)
        {
            maxHeight = i;
        }
    }
    
    return maxHeight;
}

int RandomMapGenerator::GetMinTerrainHeight(const TerrainType terrain)
{
    for (int i = 0; i < MAXIMUM_HEIGHT; i++)
    {
        if (_textures[i] == terrain)
        {
            return i;
        }
    }
    
    return -1;
}

void RandomMapGenerator::PlacePlayers(const MapSettings& settings, Map* map)
{
    const int width = map->width;
    const int height = map->height;
    const int length = std::min(width / 2, height / 2);
    
    // compute center of the map
    Vec2 center(width / 2, height / 2);

    // radius for player distribution
    const int rMin = (int)(settings.minPlayerRadius * length);;
    const int rMax = (int)(settings.maxPlayerRadius * length);
    const int rnd = Rand(rMin, rMax);
    
    // player headquarters for the players
    for (int i = 0; i < settings.players; i++)
    {
        // compute headquater position
        Vec2 position = _helper.ComputePointOnCircle(i,
                                                     settings.players,
                                                     center, (double)(rMin + rnd));

        // store headquarter position
        map->positions[i] = position;
        
        // create headquarter
        ObjectGenerator::CreateHeadquarter(map, position.y * width + position.x, i);
    }
}

void RandomMapGenerator::PlacePlayerResources(const MapSettings& settings, Map* map)
{
    for (int i = 0; i < settings.players; i++)
    {
        const int offset1 = Rand(0, 180);
        const int offset2 = Rand(180, 360);

        _helper.SetStones(map, _helper.ComputePointOnCircle(offset1,
                                                            360,
                                                            map->positions[i], 12), 2.0F);
        _helper.SetStones(map, _helper.ComputePointOnCircle(offset2,
                                                            360,
                                                            map->positions[i], 12), 2.7F);
    }
}

void RandomMapGenerator::CreateHills(const MapSettings& settings, Map* map)
{
    const int width = map->width;
    const int height = map->height;
    const int players = settings.players;
    
    for (int x = 0; x < width; x++)
    {
        for (int y = 0; y < height; y++)
        {
            double distanceToPlayer = (double)(width + height);
            
            for (int i = 0; i < players; i++)
            {
                distanceToPlayer = std::min(distanceToPlayer,
                                       VertexUtility::Distance(x, y,
                                                               map->positions[i].x,
                                                               map->positions[i].y,
                                                               width, height));
            }
            
            for (std::vector<AreaDesc>::iterator it = _areas.begin(); it != _areas.end(); ++it)
            {
                
                if (it->IsInArea(x, y, distanceToPlayer, width, height))
                {
                    const int pr = (int)(*it).likelyhoodHill;
                    const int rnd = Rand(0, pr > 0 ? 101 : (int)(100.0 / (*it).likelyhoodHill));
                    const int minZ = (*it).minElevation;
                    const int maxZ = (*it).maxElevation;
                    
                    if (maxZ > 0 && rnd <= pr)
                    {
                        const int z = Rand(minZ, maxZ + 1);
                        _helper.SetHill(map,
                                        Vec2(x, y),
                                        z == GetMinTerrainHeight(TT_MOUNTAIN1) - 1 ? z-1 : z);
                    }
                }
            }
        }
    }
}

void RandomMapGenerator::FillRemainingTerrain(const MapSettings& settings, Map* map)
{
    const int width = map->width;
    const int height = map->height;
    const int players = settings.players;

    for (int x = 0; x < width; x++)
    {
        for (int y = 0; y < height; y++)
        {
            const int index = y * width + x;
            const int level = map->z[index];
            
            // create texture for current height value
            ObjectGenerator::CreateTexture(map, index, _textures[level]);
            
            // post-processing of texture (add animals, adapt height, ...)
            switch (_textures[level])
            {
                case TT_WATER:
                    map->z[index]        = GetMaxTerrainHeight(TT_WATER);
                    map->animal[index]   = ObjectGenerator::CreateDuck(3);
                    map->resource[index] = 0x87; // fish
                    break;
                case TT_MEADOW1:
                    map->animal[index]   = ObjectGenerator::CreateRandomForestAnimal(4);
                    break;
                case TT_MEADOW_FLOWERS:
                    map->animal[index]   = ObjectGenerator::CreateSheep(4);
                    break;
                case TT_MOUNTAIN1:
                    map->resource[index] = ObjectGenerator::CreateRandomResource();
                    break;
                default:
                    break;
            }
            
            double distanceToPlayer = (double)(width + height);
            for (int i = 0; i < players; i++)
            {
                distanceToPlayer = std::min(distanceToPlayer,
                                            VertexUtility::Distance(x, y,
                                                                    map->positions[i].x,
                                                                    map->positions[i].y,
                                                                    width,
                                                                    height));
            }

            for (std::vector<AreaDesc>::iterator it = _areas.begin(); it != _areas.end(); ++it)
            {
                if (it->IsInArea(x, y, distanceToPlayer, width, height))
                {
                    if (Rand(0, 100) < (*it).likelyhoodTree)
                    {
                        _helper.SetTree(map, Vec2(x,y));
                    }
                    else if (Rand(0, 100) < (*it).likelyhoodStone)
                    {
                        _helper.SetStone(map, Vec2(x,y));
                    }
                }
            }
        }
    }
    
    ///////
    /// Harbour placement
    ///////
    std::vector<Vec2> harbors;
    
    for (int x = 0; x < width; x++)
    {
        for (int y = 0; y < height; y++)
        {
            const int index = VertexUtility::GetIndexOf(x, y, width, height);
            
            // under certain circumstances replace dessert texture by harbor position
            Vec2 water(0,0);
            if (ObjectGenerator::IsTexture(map, index, TT_DESERT))
            {
                // ensure there's water close to the dessert texture
                bool waterNeighbor = false;
                std::vector<int> neighbors = VertexUtility::GetNeighbors(x, y, width, height, 1);
                for (std::vector<int>::iterator it = neighbors.begin(); it != neighbors.end(); ++it)
                {
                    if (ObjectGenerator::IsTexture(map, *it, TT_WATER))
                    {
                        waterNeighbor = true;
                        water = Vec2(*it % width, *it / width);
                        break;
                    }
                }
                
                // ensure there's no other harbor nearby
                double closestHarbor = MIN_HARBOR_DISTANCE + 1.0;
                for (std::vector<Vec2>::iterator it = harbors.begin(); it != harbors.end(); ++it)
                {
                    closestHarbor = std::min(closestHarbor,
                                             VertexUtility::Distance(x, y, it->x, it->y, width, height));
                }
                
                const int waterTiles = (closestHarbor >= MIN_HARBOR_DISTANCE && waterNeighbor)
                                        ? _helper.ComputeWaterSize(map,
                                                                   water,
                                                                   MIN_HARBOR_WATER) : 0;

                // setup harbor position
                if (waterTiles >= MIN_HARBOR_WATER)
                {
                    _helper.SetHarbour(map, Vec2(x, y), GetMaxTerrainHeight(TT_WATER));
                    harbors.push_back(Vec2(x,y));
                }
            }
        }
    }
}

Map* RandomMapGenerator::Create(const MapSettings& settings)
{
    Map* map = new Map(settings.width, settings.height, "Random", "auto");
    
    // configuration of the map settings
    map->type = settings.type;
    map->players = settings.players;

    // the actual map generation
    PlacePlayers(settings, map);
    PlacePlayerResources(settings, map);
    CreateHills(settings, map);
    FillRemainingTerrain(settings, map);
    
    // post-processing
    _helper.SmoothTextures(map);
    
    return map;
}


