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

#include "mapGenerator/MapWriter.h"

#include <iostream>

bool MapWriter::Write(const std::string& filePath, Map* map)
{
    if (map == NULL)
    {
        return false;
    }
    
    FILE* file = fopen(filePath.c_str(), "w+");

    if (file == NULL)
    {
        return false;
    }
    
    char zero = 0; //to fill bytes
    unsigned char temp = 0; //to fill bytes
    Map *myMap = map;
    char map_version[11] = "WORLD_V1.0";
    char map_data_header[16];

    // prepare map data header
    map_data_header[0] = 0x10;
    map_data_header[1] = 0x27;
    map_data_header[2] = 0x00;
    map_data_header[3] = 0x00;
    map_data_header[4] = 0x00;
    map_data_header[5] = 0x00;
    *((uint16_t*)(map_data_header+6)) = myMap->width;
    *((uint16_t*)(map_data_header+8)) = myMap->height;
    map_data_header[10] = 0x01;
    map_data_header[11] = 0x00;
    *((uint32_t*)(map_data_header+12)) = myMap->width*myMap->height;

    // begin writing data to file
    // first of all the map header
    // WORLD_V1.0
    fwrite(map_version, 1, 10, file);
    
    // name
    fwrite(myMap->name, 1, 20, file);
    
    // old width
    fwrite(&myMap->width, 2, 1, file);
    
    // old height
    fwrite(&myMap->height, 2, 1, file);
    
    // type
    fwrite(&myMap->type, 1, 1, file);
    
    // players
    fwrite(&myMap->players, 1, 1, file);
    
    // author
    fwrite(myMap->author, 1, 20, file);
        
    // headquarters x
    for (int i = 0; i < 7; i++)
        fwrite(&myMap->positions[i].x, 2, 1, file);
    
    // headquarters y
    for (int i = 0; i < 7; i++)
        fwrite(&myMap->positions[i].y, 2, 1, file);
    
    // unknown data (8 Bytes)
    for (int i = 0; i < 8; i++)
    {
        fwrite(&zero, 1, 1, file);
    }
    
    // big map header with area information
    for (int i = 0; i < 250; i++)
    {
        fwrite(&myMap->header[i].type, 1, 1, file);
        fwrite(&myMap->header[i].x, 1, 2, file);
        fwrite(&myMap->header[i].y, 1, 2, file);
        fwrite(&myMap->header[i].area, 1, 4, file);
    }

    // 0x11 0x27
    temp = 0x11;
    fwrite(&temp, 1, 1, file);
    temp = 0x27;
    fwrite(&temp, 1, 1, file);
    
    // unknown data (always null, 4 Bytes)
    for (int i = 0; i < 4; i++)
    {
        fwrite(&zero, 1, 1, file);
    }
    
    // width
    fwrite(&myMap->width, 2, 1, file);
    
    // height
    fwrite(&myMap->height, 2, 1, file);
    
    // now begin writing the real map data
    
    // altitude information
    fwrite(&map_data_header, 16, 1, file);
    
    for (int j = 0; j < myMap->height; j++)
    {
        for (int i = 0; i < myMap->width; i++)
        {
            temp = myMap->vertex[j*myMap->width+i].z;
            fwrite(&temp, 1, 1, file);
        }
    }
    
    // texture information for RightSideUp-Triangles
    fwrite(&map_data_header, 16, 1, file);
    
    for (int j = 0; j < myMap->height; j++)
    {
        for (int i = 0; i < myMap->width; i++)
        {
            fwrite(&myMap->vertex[j*myMap->width+i].texture.first, 1, 1, file);
        }
    }
    
    // go to texture information for UpSideDown-Triangles
    fwrite(&map_data_header, 16, 1, file);
    
    for (int j = 0; j < myMap->height; j++)
    {
        for (int i = 0; i < myMap->width; i++)
        {
            fwrite(&myMap->vertex[j*myMap->width+i].texture.second, 1, 1, file);
        }
    }
    
    // go to road data
    fwrite(&map_data_header, 16, 1, file);
    
    for (int j = 0; j < myMap->height; j++)
    {
        for (int i = 0; i < myMap->width; i++)
        {
            fwrite(&myMap->vertex[j*myMap->width+i].road, 1, 1, file);
        }
    }
    
    // go to object type data
    fwrite(&map_data_header, 16, 1, file);
    
    for (int j = 0; j < myMap->height; j++)
    {
        for (int i = 0; i < myMap->width; i++)
        {
            fwrite(&myMap->vertex[j*myMap->width+i].object.first, 1, 1, file);
        }
    }
    
    // go to object info data
    fwrite(&map_data_header, 16, 1, file);
    
    for (int j = 0; j < myMap->height; j++)
    {
        for (int i = 0; i < myMap->width; i++)
        {
            fwrite(&myMap->vertex[j*myMap->width+i].object.second, 1, 1, file);
        }
    }
    
    // go to animal data
    fwrite(&map_data_header, 16, 1, file);
    
    for (int j = 0; j < myMap->height; j++)
    {
        for (int i = 0; i < myMap->width; i++)
        {
            fwrite(&myMap->vertex[j*myMap->width+i].animal, 1, 1, file);
        }
    }
    
    // go to unknown1 data
    fwrite(&map_data_header, 16, 1, file);
    
    for (int j = 0; j < myMap->height; j++)
    {
        for (int i = 0; i < myMap->width; i++)
        {
            fwrite(&myMap->vertex[j*myMap->width+i].unknown1, 1, 1, file);
        }
    }
    
    // go to build data
    fwrite(&map_data_header, 16, 1, file);
    
    for (int j = 0; j < myMap->height; j++)
    {
        for (int i = 0; i < myMap->width; i++)
        {
            fwrite(&myMap->vertex[j*myMap->width+i].build, 1, 1, file);
        }
    }
    
    // go to unknown2 data
    fwrite(&map_data_header, 16, 1, file);
    
    for (int j = 0; j < myMap->height; j++)
    {
        for (int i = 0; i < myMap->width; i++)
        {
            fwrite(&myMap->vertex[j*myMap->width+i].unknown2, 1, 1, file);
        }
    }
    
    // go to unknown3 data
    fwrite(&map_data_header, 16, 1, file);
    
    for (int j = 0; j < myMap->height; j++)
    {
        for (int i = 0; i < myMap->width; i++)
        {
            fwrite(&myMap->vertex[j*myMap->width+i].unknown3, 1, 1, file);
        }
    }
    
    // go to resource data
    fwrite(&map_data_header, 16, 1, file);
    
    for (int j = 0; j < myMap->height; j++)
    {
        for (int i = 0; i < myMap->width; i++)
        {
            fwrite(&myMap->vertex[j*myMap->width+i].resource, 1, 1, file);
        }
    }
    
    // go to shading data
    fwrite(&map_data_header, 16, 1, file);
    
    for (int j = 0; j < myMap->height; j++)
    {
        for (int i = 0; i < myMap->width; i++)
        {
            fwrite(&myMap->vertex[j*myMap->width+i].shading, 1, 1, file);
        }
    }
    
    // go to unknown5 data
    fwrite(&map_data_header, 16, 1, file);
    
    for (int j = 0; j < myMap->height; j++)
    {
        for (int i = 0; i < myMap->width; i++)
        {
            fwrite(&myMap->vertex[j*myMap->width+i].unknown5, 1, 1, file);
        }
    }
    
    // at least write the map footer (ends in 0xFF)
    temp = 0xFF;
    fwrite(&temp, 1, 1, file);
    
    // close file after writing
    fclose(file);
    
    return true;
}

