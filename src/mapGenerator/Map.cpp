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

#include "mapGenerator/Map.h"

#include <iostream>
#include <vector>

ArchivInfo* Map::CreateArchiv()
{
    ArchivInfo* info = new ArchivInfo();
    ArchivItem_Map* map = new ArchivItem_Map();
    ArchivItem_Map_Header* header = new ArchivItem_Map_Header();
    std::vector<unsigned char> data;
    
    // create header information for the archiv
    header->setName("Random");
    header->setAuthor("auto");
    header->setWidth(width);
    header->setHeight(height);
    header->setPlayer(players);
    header->setGfxSet(type);
    for (int i = 0; i < 7; i++)
    {
        header->setPlayerHQx(i, positions[i].x);
        header->setPlayerHQy(i, positions[i].y);
    }
    map->set(0, header);

    // altitude information
    for (int j = 0; j < height; j++)
    {
        for (int i = 0; i < width; i++)
        {
            data.push_back(vertex[j * width + i].z);
        }
    }
    map->set(1, new ArchivItem_Raw(data));

    // texture information (right-side-up)
    data.clear();
    for (int j = 0; j < height; j++)
    {
        for (int i = 0; i < width; i++)
        {
            data.push_back(vertex[j * width + i].texture.first);
        }
    }
    map->set(2, new ArchivItem_Raw(data));

    // texture information (up-side-down)
    data.clear();
    for (int j = 0; j < height; j++)
    {
        for (int i = 0; i < width; i++)
        {
            data.push_back(vertex[j * width + i].texture.second);
        }
    }
    map->set(3, new ArchivItem_Raw(data));

    // road information
    data.clear();
    for (int j = 0; j < height; j++)
    {
        for (int i = 0; i < width; i++)
        {
            data.push_back(vertex[j * width + i].road);
        }
    }
    map->set(4, new ArchivItem_Raw(data));
    
    // object type information
    data.clear();
    for (int j = 0; j < height; j++)
    {
        for (int i = 0; i < width; i++)
        {
            data.push_back(vertex[j * width + i].object.first);
        }
    }
    map->set(5, new ArchivItem_Raw(data));
    
    // object information
    data.clear();
    for (int j = 0; j < height; j++)
    {
        for (int i = 0; i < width; i++)
        {
            data.push_back(vertex[j * width + i].object.second);
        }
    }
    map->set(6, new ArchivItem_Raw(data));

    // animal information
    data.clear();
    for (int j = 0; j < height; j++)
    {
        for (int i = 0; i < width; i++)
        {
            data.push_back(vertex[j * width + i].animal);
        }
    }
    map->set(7, new ArchivItem_Raw(data));
    
    // unknown information
    data.clear();
    for (int j = 0; j < height; j++)
    {
        for (int i = 0; i < width; i++)
        {
            data.push_back(vertex[j * width + i].unknown1);
        }
    }
    map->set(8, new ArchivItem_Raw(data));

    // build information
    data.clear();
    for (int j = 0; j < height; j++)
    {
        for (int i = 0; i < width; i++)
        {
            data.push_back(vertex[j * width + i].build);
        }
    }
    map->set(9, new ArchivItem_Raw(data));

    // unknown information
    data.clear();
    for (int j = 0; j < height; j++)
    {
        for (int i = 0; i < width; i++)
        {
            data.push_back(vertex[j * width + i].unknown2);
        }
    }
    map->set(10, new ArchivItem_Raw(data));

    // unknown information
    data.clear();
    for (int j = 0; j < height; j++)
    {
        for (int i = 0; i < width; i++)
        {
            data.push_back(vertex[j * width + i].unknown3);
        }
    }
    map->set(11, new ArchivItem_Raw(data));

    // resource information
    data.clear();
    for (int j = 0; j < height; j++)
    {
        for (int i = 0; i < width; i++)
        {
            data.push_back(vertex[j * width + i].resource);
        }
    }
    map->set(12, new ArchivItem_Raw(data));

    // shading information
    data.clear();
    for (int j = 0; j < height; j++)
    {
        for (int i = 0; i < width; i++)
        {
            data.push_back(vertex[j * width + i].shading);
        }
    }
    map->set(13, new ArchivItem_Raw(data));
    
    // unknown information
    data.clear();
    for (int j = 0; j < height; j++)
    {
        for (int i = 0; i < width; i++)
        {
            data.push_back(vertex[j * width + i].unknown5);
        }
    }
    map->set(14, new ArchivItem_Raw(data));

    info->push(map);

    return info;
}
