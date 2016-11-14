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

#define MIN_DISTANCE_PLAYERS    25
#define MIN_DISTANCE_WATER      15.0
#define MIN_DISTANCE_RES        10.0
#define MIN_DISTANCE_MOUNTAIN   20.0
#define MIN_DISTANCE_TREES      4.0
#define TREE_LIKELYHOOD_MED     20
#define TREE_LIKELYHOOD_MIN     8
#define STONE_LIKELYHOOD        5
#define LEVEL_WATER             1
#define LEVEL_MOUNTAIN          11
#define LEVEL_SNOW              13
#define LEVEL_MAXIMUM           14
#define HILL_LIKELYHOOD_MAX     1
#define HILL_LIKELYHOOD_MED     2
#define HILL_LIKELYHOOD_MIN     25


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
    const int rMin = MIN_DISTANCE_PLAYERS;
    const int rMax = std::max(1, (int) (0.9F * std::min(width / 2, height / 2)));
    
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
        res.push_back(std::pair<int, int>(0, (int)MIN_DISTANCE_RES + rand() % 2)); // stone
        res.push_back(std::pair<int, int>(1, (int)MIN_DISTANCE_RES + rand() % 2)); // stone
        res.push_back(std::pair<int, int>(2, (int)MIN_DISTANCE_RES + rand() % (int)MIN_DISTANCE_WATER)); // tree
        res.push_back(std::pair<int, int>(2, (int)MIN_DISTANCE_RES + rand() % (int)MIN_DISTANCE_WATER)); // tree
        res.push_back(std::pair<int, int>(2, (int)MIN_DISTANCE_RES + rand() % (int)MIN_DISTANCE_WATER)); // tree
        res.push_back(std::pair<int, int>(2, (int)MIN_DISTANCE_RES + rand() % (int)MIN_DISTANCE_WATER)); // tree
        res.push_back(std::pair<int, int>(2, (int)MIN_DISTANCE_RES + rand() % (int)MIN_DISTANCE_WATER)); // tree
        
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
                case 2:
                    SetTrees(map, pos, 1.0F + (float)(rand() % 2));
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
    
    for (int x = 0; x < width; x++)
    {
        for (int y = 0; y < height; y++)
        {
            double distanceToPlayer = (double)(width + height);
            
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
            
            int max = 0, pr = 10;
            if (distanceToPlayer > MIN_DISTANCE_MOUNTAIN)
            {
                max = LEVEL_MAXIMUM;
                pr = HILL_LIKELYHOOD_MAX;
            }
            else if (distanceToPlayer > MIN_DISTANCE_RES)
            {
                max = 4;
                pr = HILL_LIKELYHOOD_MIN;
            }
            else if (distanceToPlayer > MIN_DISTANCE_RES / 2)
            {
                max = 2;
                pr = HILL_LIKELYHOOD_MIN;
            }
            
            if (max > 0 && rand() % 100 < pr)
            {
                int z = rand() % max + 1;
                if (z == LEVEL_MOUNTAIN - 1) z--; // avoid pre-mountains without mountains
                
                std::vector<int> neighbors = VertexUtility::GetNeighbors(x, y, width, height, (double)z);
                for (std::vector<int>::iterator it = neighbors.begin(); it != neighbors.end(); ++it)
                {
                    int x2 = *it % width, y2 = *it / width;
                    const int h = z - (int)VertexUtility::Distance(x, y, x2, y2, width, height);
                    
                    if (!ObjectGenerator::IsTexture(map->vertex[*it].texture, TRIANGLE_TEXTURE_WATER) &&
                        map->vertex[*it].z < h)
                    {
                        map->vertex[*it].z = h;
                    }
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
            
            const int index = y * width + x;
            
            ////////
            /// texturing & animals
            ////////
            
            if (map->vertex[index].z <= LEVEL_WATER)
            {
                if (distanceToPlayer > MIN_DISTANCE_WATER)
                {
                    map->vertex[index].z = LEVEL_WATER;
                    SetWater(map, Vec2(x,y), 1.0F);
                }
            }
            else if (map->vertex[index].z == LEVEL_WATER + 1)
            {
                if (!ObjectGenerator::IsTexture(map->vertex[index].texture, TRIANGLE_TEXTURE_STEPPE_MEADOW1) &&
                    !ObjectGenerator::IsTexture(map->vertex[index].texture, TRIANGLE_TEXTURE_STEPPE_MEADOW2) &&
                    !ObjectGenerator::IsTexture(map->vertex[index].texture, TRIANGLE_TEXTURE_STEPPE) &&
                    !ObjectGenerator::IsTexture(map->vertex[index].texture, TRIANGLE_TEXTURE_WATER))
                {
                    map->vertex[index].texture = ObjectGenerator::CreateTexture(TRIANGLE_TEXTURE_FLOWER);
                    map->vertex[index].animal = (rand() % 15 == 0) ? ObjectGenerator::CreateSheep() : 0x00;
                }
            }
            else if (map->vertex[index].z >= LEVEL_SNOW)
            {
                map->vertex[index].texture = ObjectGenerator::CreateTexture(TRIANGLE_TEXTURE_SNOW);
            }
            else if (map->vertex[index].z >= LEVEL_MOUNTAIN)
            {
                map->vertex[index].texture = ObjectGenerator::CreateTexture(TRIANGLE_TEXTURE_MINING1);
                const int rnd = rand() % 100;
                int resource = 0x00;
                if (rnd <= 9)       resource = 0x51; // 9% gold
                else if (rnd <= 45) resource = 0x49; // 36% iron
                else if (rnd <= 85) resource = 0x41; // 40% coal
                else                resource = 0x59; // 15% granite
                map->vertex[index].resource = resource + rand() % 7;
            }
            else if (map->vertex[index].z >= LEVEL_MOUNTAIN - 1)
            {
                map->vertex[index].texture = ObjectGenerator::CreateTexture(TRIANGLE_TEXTURE_MINING_MEADOW);
            }
            else
            {
                map->vertex[index].animal = (rand() % 25 == 0) ? ObjectGenerator::CreateRandomForestAnimal() : 0x00;
            }
            
            ////////
            /// random trees & stones
            ////////
            
            if (distanceToPlayer > MIN_DISTANCE_TREES * 2)
            {
                if (rand() % 100 <= TREE_LIKELYHOOD_MED)
                {
                    SetTree(map, Vec2(x,y));
                }
                else if (rand() % 100 <= STONE_LIKELYHOOD)
                {
                    SetStone(map, Vec2(x,y));
                }
            }
            else if (distanceToPlayer > MIN_DISTANCE_TREES)
            {
                if (rand() % 100 <= TREE_LIKELYHOOD_MIN) SetTree(map, Vec2(x,y));
            }
        }
    }
}

Map* GreenlandGenerator::GenerateMap(const MapSettings& settings)
{
    Map* map = new Map();
    
    strcpy(map->name, "Random");
    strcpy(map->author, "generator");
    map->width = settings.width;
    map->height = settings.height;
    map->type = settings.type;
    map->players = settings.players;
    map->vertex = new Vertex[settings.width * settings.height];
    
    CreateEmptyTerrain(settings, map);
    PlacePlayers(settings, map);
    PlacePlayerResources(settings, map);
    CreateHills(settings, map);
    FillRemainingTerrain(settings, map);
    
    return map;
}


