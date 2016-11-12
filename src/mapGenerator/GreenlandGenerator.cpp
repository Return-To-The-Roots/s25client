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

Map* GreenlandGenerator::GenerateMap(const MapSettings& settings)
{
    Map* myMap = new Map();
    
    int width = settings.width;
    int height = settings.height;
    int players = settings.players;
    int distance = settings.distance;
    
    strcpy(myMap->name, "Random");
    strcpy(myMap->author, "generator");
    myMap->width = width;
    myMap->height = height;
    myMap->type = settings.type;
    myMap->players = players;
    myMap->vertex = new Vertex[myMap->width * myMap->height];
    
    for (int j = 0; j < height; j++)
    {
        for (int i = 0; i < width; i++)
        {
            myMap->vertex[j * width + i].z = 0;
            myMap->vertex[j * width + i].rsuTexture = TRIANGLE_TEXTURE_MEADOW1;
            myMap->vertex[j * width + i].usdTexture = TRIANGLE_TEXTURE_MEADOW1;
            myMap->vertex[j * width + i].build = 0x04;
            myMap->vertex[j * width + i].shading = 0x80;
            myMap->vertex[j * width + i].resource = 0x00;
            myMap->vertex[j * width + i].road = 0x00;
            myMap->vertex[j * width + i].objectType = 0x00;
            myMap->vertex[j * width + i].objectInfo = 0x00;
            myMap->vertex[j * width + i].animal = 0x00;
            myMap->vertex[j * width + i].unknown1 = 0x00;
            myMap->vertex[j * width + i].unknown2 = 0x07;
            myMap->vertex[j * width + i].unknown3 = 0x00;
            myMap->vertex[j * width + i].unknown5 = 0x00;
        }
    }

    // compute center of the map
    Vec2 center;
    center.x = width / 2;
    center.y = height / 2;
    
    // radius for player distribution
    int max = (int) (0.9F * std::min(width / 2, height / 2));
    int offset = std::max(max - distance, 1);

    // initialize randomize timer
    srand(time(NULL));

    // player headquarters for the players
    for (int i = 0; i < 7; i++)
    {
        if (i < players)
        {
            // compute headquater position
            Vec2 position = PointOnCircle(i, players, center, distance + rand() % offset);
            
            // apply position of the headquater
            myMap->HQx[i] = position.x;
            myMap->HQy[i] = position.y;
            
            // compute index of the player's vertex
            int index = position.y * width + position.x;
            
            // object info must be 0x80 for headquaters
            myMap->vertex[index].objectInfo = 0x80;
            
            // object type must equal the player number
            myMap->vertex[index].objectType = i;
        }
        else
        {
            myMap->HQx[i] = 0xFFFF;
            myMap->HQy[i] = 0xFFFF;
        }
    }

    // stone in the center
    int centerRadius = rand() % std::max(1, distance / 8);
    SetStone(myMap, center, centerRadius);
    
    // forest in the center
    SetTrees(myMap, center, centerRadius + rand() % std::max(1, distance / 2));
    
    // resources for each player
    for (int i = 0; i < 7; i++)
    {
        if (i < players)
        {
            // store current player position
            Vec2 playerPos;
            playerPos.x = myMap->HQx[i];
            playerPos.y = myMap->HQy[i];

            // intialize list of different resources identified by indices
            std::vector<std::pair<int, int> > res; // resource index + distance to player
            res.push_back(std::pair<int, int>(0, 12)); // stone
            res.push_back(std::pair<int, int>(1, 12)); // stone
            res.push_back(std::pair<int, int>(2, 10)); // trees
            res.push_back(std::pair<int, int>(3, 16)); // trees
            res.push_back(std::pair<int, int>(4, 16)); // water
            res.push_back(std::pair<int, int>(5, 22)); // trees
            res.push_back(std::pair<int, int>(5, 25)); // trees
            
            // put resource placement into random order to generate more interesting maps
            std::random_shuffle(res.begin(), res.end());
            
            // stores the current offset of the current resource position on an imaginary cycle
            // to avoid overlapping resources during placement
            int circle_offset = 0;
            Vec2 pos;
            
            for (std::vector<std::pair<int, int> >::iterator it = res.begin(); it != res.end(); ++it)
            {
                pos = PointOnCircle(rand() % (360 / res.size()) + circle_offset, 360, playerPos, it->second);

                switch (it->first)
                {
                    case 0:
                        SetStone(myMap, pos, 2.0F);
                        break;
                    case 1:
                        SetStone(myMap, pos, 2.7F);
                        break;
                    case 2:
                        SetTrees(myMap, pos, 5.5F);
                        break;
                    case 3:
                        SetTrees(myMap, pos, 8.7F);
                        break;
                    case 4:
                        SetWater(myMap, pos, 4.0F + (float)(rand() % 5));
                        break;
                    case 5:
                        SetTrees(myMap, pos, 3.3F);
                        break;
                }
                
                // iteration about a circle in degree
                circle_offset = (circle_offset + 360 / res.size()) % 360;
            }
        }
    }
    
    return myMap;
}


