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

#include <stdexcept>
#include "mapGenerator/Defines.h"
#include "mapGenerator/MapGenerator.h"
#include "mapGenerator/MapWriter.h"
#include "files.h"
#include "fileFuncs.h"

MapGenerator::MapGenerator(int players, int width, int height, int type)
{
    _players = players;
    _width = width;
    _height = height;
    _type = type;
}

MapGenerator::~MapGenerator()
{
    
}

void MapGenerator::Create(const std::string& filePath)
{
    MapWriter* writer = new MapWriter();
    mgMap* map = CreateEmptyMap(128, 128, 0, TRIANGLE_TEXTURE_MEADOW1); // TODO create random map
    
    if (!writer->Write(filePath, map))
    {
        delete map;
        delete writer;
        
        throw std::invalid_argument("Failed to write the random map to the filePath");
    }
    
    delete map;
    delete writer;
}

mgMap* MapGenerator::CreateEmptyMap(int texture)
{
    mgMap* myMap = new mgMap();
    
    strcpy(myMap->name, "Random");
    strcpy(myMap->author, "generator");
    myMap->width = _width;
    myMap->width_old = _width;
    myMap->height = _height;
    myMap->height_old = _height;
    myMap->type = _type;
    myMap->player = _players;
    myMap->vertex = new mgPoint[myMap->width * myMap->height];
    
    for (int j = 0; j < myMap->height; j++)
    {
        for (int i = 0; i < myMap->width; i++)
        {
            myMap->vertex[j*myMap->width+i].z = 0;
            myMap->vertex[j*myMap->width+i].rsuTexture = texture;
            myMap->vertex[j*myMap->width+i].usdTexture = texture;
            myMap->vertex[j*myMap->width+i].build = 0x04;
            myMap->vertex[j*myMap->width+i].shading = 0x80;
            myMap->vertex[j*myMap->width+i].resource = 0x00;
            myMap->vertex[j*myMap->width+i].road = 0x00;
            myMap->vertex[j*myMap->width+i].objectType = 0x00;
            myMap->vertex[j*myMap->width+i].objectInfo = 0x00;
            myMap->vertex[j*myMap->width+i].animal = 0x00;
            myMap->vertex[j*myMap->width+i].unknown1 = 0x00;
            myMap->vertex[j*myMap->width+i].unknown2 = 0x07;
            myMap->vertex[j*myMap->width+i].unknown3 = 0x00;
            myMap->vertex[j*myMap->width+i].unknown5 = 0x00;
        }
    }
    
    // player headquarters for the players
    for (int i = 0; i < 7; i++)
    {
        if (i < myMap->player)
        {
            // compute headquater position
            uint16_t x = uint16_t( (width / (2 * myMap->player)) * (i+1));
            uint16_t y = uint16_t( (height / (2 * myMap->player)) * (i+1));

            // x-position of headerquater for player i
            myMap->HQx[i] = x;
            
            // y-position of headerquater for player i
            myMap->HQy[i] = y;
            
            // object info must be 0x80 for headquaters
            myMap->vertex[y * myMap->width + x].objectInfo = 0x80;
            
            // object type must equal the player number
            myMap->vertex[y * myMap->width + x].objectType = i;
        }
        else
        {
            myMap->HQx[i] = 0xFFFF;
            myMap->HQy[i] = 0xFFFF;
        }
    }
    
    return myMap;
}


