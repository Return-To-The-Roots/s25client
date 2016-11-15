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

#include <vector>
#include <random>
#include <utility>
#include "mapGenerator/Defines.h"
#include "mapGenerator/GreenlandGenerator.h"
#include "mapGenerator/VertexUtility.h"
#include "mapGenerator/ObjectGenerator.h"

// texture definition through height-map
#define LEVEL_WATER             3
#define LEVEL_DESSERT           4
#define LEVEL_STEPPE            5
#define LEVEL_GRASSYSTEPPE      6
#define LEVEL_GRASS             7
#define LEVEL_GRASS_FLOWERS     8
#define LEVEL_GRASS2            10
#define LEVEL_PREMOUNTAIN       11
#define LEVEL_MOUNTAIN          14

// maximum height for specific land areas
#define MAX_LEVEL_ISLANDS_SMALL 7
#define MAX_LEVEL_ISLANDS       15
#define MAX_LEVEL_LAND          15
#define MAX_LEVEL_LAND_INNER    23

// minimum distance from each player
#define MIN_DISTANCE_WATER      15.0
#define MIN_DISTANCE_MOUNTAIN   15.0
#define MIN_DISTANCE_TREES      6.0
#define MIN_DISTANCE_STONE      10.0

// harbor placement
#define LIKELYHOOD_HARBOR       10
#define MIN_HARBOR_DISTANCE     20.0

// likelyhood for hill-generation for specific land areas
#define LIKELYHOOD_HILL_LAND            2.0
#define LIKELYHOOD_HILL_LAND_INNER      1.0
#define LIKELYHOOD_HILL_ISLANDS         0.1
#define LIKELYHOOD_HILL_ISLANDS_SMALL   0.1

void GreenlandGenerator::CreateEmptyTerrain(const MapSettings& settings, Map* map)
{
    const int width = map->width;
    const int height = map->height;
    
    for (int j = 0; j < height; j++)
    {
        for (int i = 0; i < width; i++)
        {
            map->vertex[j * width + i].z = 0;
            map->vertex[j * width + i].texture = ObjectGenerator::CreateTexture(TRIANGLE_TEXTURE_MEADOW1);
            map->vertex[j * width + i].build = 0x04;
            map->vertex[j * width + i].shading = 0x80;
            map->vertex[j * width + i].resource = 0x00;
            map->vertex[j * width + i].road = 0x00;
            map->vertex[j * width + i].object.first = 0x00;
            map->vertex[j * width + i].object.second = 0x00;
            map->vertex[j * width + i].animal = 0x00;
            map->vertex[j * width + i].unknown1 = 0x00;
            map->vertex[j * width + i].unknown2 = 0x07;
            map->vertex[j * width + i].unknown3 = 0x00;
            map->vertex[j * width + i].unknown5 = 0x00;
        }
    }
}

void GreenlandGenerator::PlacePlayers(const MapSettings& settings, Map* map)
{
    const int width = map->width;
    const int height = map->height;

    // compute center of the map
    Vec2 center(width / 2, height / 2);

    // radius for player distribution
    const int rMin = (int)(_radiusPlayerMin * std::min(width / 2, height / 2));;
    const int rMax = (int)(_radiusPlayerMax * std::min(width / 2, height / 2));
    
    // initialize randomize timer
    srand(time(NULL));
    
    // player headquarters for the players
    for (int i = 0; i < settings.players; i++)
    {
        // compute headquater position
        Vec2 position = ComputePointOnCircle(i, settings.players, center,
                                            rMin + rand() % (rMax - rMin));

        // create headquarter
        map->positions[i] = position;
        map->vertex[position.y * width + position.x].object = ObjectGenerator::CreateHeadquarter(i);
    }
}

void GreenlandGenerator::PlacePlayerResources(const MapSettings& settings, Map* map)
{
    for (int i = 0; i < settings.players; i++)
    {
        // intialize list of different resources identified by indices
        std::vector<std::pair<int, int> > res; // resource index + distance to player
        res.push_back(std::pair<int, int>(0, (int)MIN_DISTANCE_STONE + rand() % 2)); // stone
        res.push_back(std::pair<int, int>(1, (int)MIN_DISTANCE_STONE + rand() % 2)); // stone
        
        // put resource placement into random order to generate more interesting maps
        std::random_shuffle(res.begin(), res.end());
            
        // stores the current offset of the current resource position on an imaginary cycle
        // to avoid overlapping resources during placement
        int circle_offset = 0;
        Vec2 pos;
        
        for (std::vector<std::pair<int, int> >::iterator it = res.begin(); it != res.end(); ++it)
        {
            pos = ComputePointOnCircle(rand() % (360 / res.size()) + circle_offset,
                                        360, map->positions[i], it->second);
                
            switch (it->first)
            {
                case 0:
                    SetStones(map, pos, 2.0F);
                    break;
                case 1:
                    SetStones(map, pos, 2.7F);
                    break;
            }
                
            // iteration about a circle in degree
            circle_offset = (circle_offset + 360 / res.size()) % 360;
        }
    }
}

void GreenlandGenerator::CreateHills(const MapSettings& settings, Map* map)
{
    const int width = map->width;
    const int height = map->height;
    const int players = settings.players;
    const int length = std::min(width / 2, height / 2);
    
    for (int x = 0; x < width; x++)
    {
        for (int y = 0; y < height; y++)
        {
            double distanceToCenter = VertexUtility::Distance(width / 2, height / 2, x, y, width, height);
            double distanceToPlayer = (double)(width + height);
            
            for (int i = 0; i < players; i++)
            {
                distanceToPlayer = std::min(distanceToPlayer,
                                            VertexUtility::Distance(x, y,
                                                                    map->positions[i].x,
                                                                    map->positions[i].y,
                                                                    width, height));
            }
            
            int max = 0, min = 0;
            double likelyhood = 0.0;
            
            if (distanceToPlayer <= 3.0) // plane land around headquarter
            {
                min = LEVEL_GRASS;
                max = LEVEL_GRASS;
                likelyhood = 100.0;
            }
            else
            if (distanceToPlayer <= MIN_DISTANCE_MOUNTAIN) // low hills around player
            {
                min = LEVEL_STEPPE;
                max = LEVEL_GRASS2;
                likelyhood = 100.0;
            }
            else
            if (distanceToCenter <= _radiusInnerLand * length)
            {
                max = MAX_LEVEL_LAND_INNER;
                likelyhood = LIKELYHOOD_HILL_LAND_INNER;
            }
            else
            if (distanceToCenter <= _radiusIslands * length)
            {
                max = MAX_LEVEL_LAND;
                likelyhood = LIKELYHOOD_HILL_LAND;
            }
            else
            if (distanceToCenter <= _radiusSmallIslands * length)
            {
                max = MAX_LEVEL_ISLANDS;
                likelyhood = LIKELYHOOD_HILL_ISLANDS;
            }
            else
            if (distanceToCenter <= _radiusWaterOnly * length)
            {
                max = MAX_LEVEL_ISLANDS_SMALL;
                likelyhood = LIKELYHOOD_HILL_ISLANDS_SMALL;
            }

            const int pr = (int)likelyhood;
            const int rnd = pr > 0 ? (rand() % 100 + 1) : (rand() % (int)(100.0 / likelyhood));
            
            if (max > 0 && rnd <= pr)
            {
                int z = min + rand() % (max - min + 1);
                if (z == LEVEL_MOUNTAIN - 1) z--; // avoid pre-mountains without mountains
                
                std::vector<int> neighbors = VertexUtility::GetNeighbors(x, y, width, height, z);
                for (std::vector<int>::iterator it = neighbors.begin(); it != neighbors.end(); ++it)
                {
                    const int x2 = *it % width, y2 = *it / width;
                    const int dist = (int)(z - VertexUtility::Distance(x, y, x2, y2, width, height));
                    map->vertex[*it].z = std::max(dist, map->vertex[*it].z);
                }
            }
        }
    }
}

void GreenlandGenerator::FillRemainingTerrain(const MapSettings& settings, Map* map)
{
    const int width = map->width;
    const int height = map->height;
    const int players = settings.players;
    
    for (int x = 0; x < width; x++)
    {
        for (int y = 0; y < height; y++)
        {
            double distanceToPlayer = (double)(width + height);

            // compute distance to the closest player
            for (int i = 0; i < players; i++)
            {
                distanceToPlayer = std::min(distanceToPlayer,
                                            VertexUtility::Distance(x,
                                                                    y,
                                                                    map->positions[i].x,
                                                                    map->positions[i].y,
                                                                    width,
                                                                    height));
            }
            
            ////////
            /// texturing, animals and mountain resources
            ////////
            
            const int index = y * width + x;
            const int level = map->vertex[index].z;
            
            if (level <= LEVEL_WATER && distanceToPlayer > MIN_DISTANCE_WATER)
            {
                map->vertex[index].z = LEVEL_WATER;
                map->vertex[index].texture = ObjectGenerator::CreateTexture(TRIANGLE_TEXTURE_WATER);
                map->vertex[index].animal = (rand() % 30 == 0) ? ObjectGenerator::CreateDuck() : 0x00;
                map->vertex[index].resource = 0x87; // fish
            }
            else if (level <= LEVEL_DESSERT && distanceToPlayer > MIN_DISTANCE_WATER)
            {
                map->vertex[index].texture = ObjectGenerator::CreateTexture(TRIANGLE_TEXTURE_STEPPE);
            }
            else if (level <= LEVEL_STEPPE)
            {
                map->vertex[index].texture = ObjectGenerator::CreateTexture(TRIANGLE_TEXTURE_STEPPE_MEADOW2);
            }
            else if (level <= LEVEL_GRASSYSTEPPE)
            {
                map->vertex[index].texture = ObjectGenerator::CreateTexture(TRIANGLE_TEXTURE_STEPPE_MEADOW1);
            }
            else if (level <= LEVEL_GRASS)
            {
                map->vertex[index].texture = ObjectGenerator::CreateTexture(TRIANGLE_TEXTURE_MEADOW1);
                map->vertex[index].animal = (rand() % 20 == 0) ? ObjectGenerator::CreateRandomForestAnimal() : 0x00;
            }
            else if (level <= LEVEL_GRASS_FLOWERS)
            {
                map->vertex[index].texture = ObjectGenerator::CreateTexture(TRIANGLE_TEXTURE_FLOWER);
                map->vertex[index].animal = (rand() % 19 == 0) ? ObjectGenerator::CreateSheep() : 0x00;
            }
            else if (level <= LEVEL_GRASS2)
            {
                map->vertex[index].texture = ObjectGenerator::CreateTexture(TRIANGLE_TEXTURE_MEADOW2);
                map->vertex[index].animal = (rand() % 20 == 0) ? ObjectGenerator::CreateRandomForestAnimal() : 0x00;
            }
            else if (level <= LEVEL_PREMOUNTAIN)
            {
                map->vertex[index].texture = ObjectGenerator::CreateTexture(TRIANGLE_TEXTURE_MINING_MEADOW);
            }
            else if (level <= LEVEL_MOUNTAIN)
            {
                map->vertex[index].texture = ObjectGenerator::CreateTexture(TRIANGLE_TEXTURE_MINING1);
                map->vertex[index].resource = ObjectGenerator::CreateRandomResource();
            }
            else
            {
                map->vertex[index].texture = ObjectGenerator::CreateTexture(TRIANGLE_TEXTURE_SNOW);
            }

            ////////
            /// random resources
            ////////
            
            const int rnd = rand() % 100;
            
            if (distanceToPlayer > MIN_DISTANCE_TREES && rnd < _likelyhoodTree)
            {
                SetTree(map, Vec2(x,y));
            }
            else
            if (distanceToPlayer > MIN_DISTANCE_STONE && rnd < _likelyhoodTree + _likelyhoodStone)
            {
                SetStone(map, Vec2(x,y));
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
            if (ObjectGenerator::IsTexture(map->vertex[index].texture, TRIANGLE_TEXTURE_STEPPE))
            {
                // ensure there's water close to the dessert texture
                bool waterNeighbor = false;
                std::vector<int> neighbors = VertexUtility::GetNeighbors(x, y, width, height, 1);
                for (std::vector<int>::iterator it = neighbors.begin(); it != neighbors.end(); ++it)
                {
                    if (ObjectGenerator::IsTexture(map->vertex[*it].texture, TRIANGLE_TEXTURE_WATER))
                    {
                        waterNeighbor = true;
                        break;
                    }
                }
                
                // ensure there's no other harbor nearby
                double closestHarbor = MIN_HARBOR_DISTANCE + 1.0;
                for (std::vector<Vec2>::iterator it = harbors.begin(); it != harbors.end(); ++it)
                {
                    closestHarbor = std::min(closestHarbor, VertexUtility::Distance(x,
                                                                                    y,
                                                                                    it->x,
                                                                                    it->y,
                                                                                    width,
                                                                                    height));
                }
                
                // setup harbor position
                if (closestHarbor >= MIN_HARBOR_DISTANCE && waterNeighbor && rand() % 100 < LIKELYHOOD_HARBOR)
                {
                    SetHarbour(map, Vec2(x, y), LEVEL_WATER);
                    harbors.push_back(Vec2(x,y));
                }
            }
        }
    }
}

Map* GreenlandGenerator::GenerateMap(const MapSettings& settings)
{
    Map* map = new Map();
    
    // configuration of the map header
    strcpy(map->name, "Random");
    strcpy(map->author, "auto");
    
    // configuration of the map settings
    map->width = settings.width;
    map->height = settings.height;
    map->type = settings.type;
    map->players = settings.players;
    map->vertex = new Vertex[settings.width * settings.height];

    // the actual map generation
    CreateEmptyTerrain(settings, map);
    PlacePlayers(settings, map);
    PlacePlayerResources(settings, map);
    CreateHills(settings, map);
    FillRemainingTerrain(settings, map);
    
    return map;
}


