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
    header->setName(name);
    header->setAuthor(author);
    header->setWidth(width);
    header->setHeight(height);
    header->setPlayer(players);
    header->setGfxSet(type);
    for (int i = 0; i < 7; i++)
    {
        header->setPlayerHQ(i, positions[i].x, positions[i].y);
    }
    map->set(0, header);

    // altitude information
    map->set(1, new ArchivItem_Raw(z));

    // texture information (right-side-up)
    map->set(2, new ArchivItem_Raw(textureRsu));

    // texture information (up-side-down)
    map->set(3, new ArchivItem_Raw(textureLsd));

    // road information
    map->set(4, new ArchivItem_Raw(road));
    
    // object type information
    map->set(5, new ArchivItem_Raw(objectType));
    
    // object information
    map->set(6, new ArchivItem_Raw(objectInfo));

    // animal information
    map->set(7, new ArchivItem_Raw(animal));
    
    // unknown information
    map->set(8, new ArchivItem_Raw(unknown1));

    // build information
    map->set(9, new ArchivItem_Raw(build));

    // unknown information
    map->set(10, new ArchivItem_Raw(unknown2));

    // unknown information
    map->set(11, new ArchivItem_Raw(unknown3));

    // resource information
    map->set(12, new ArchivItem_Raw(resource));

    // shading information
    map->set(13, new ArchivItem_Raw(shading));
    
    // unknown information
    map->set(14, new ArchivItem_Raw(unknown5));

    info->push(map);

    return info;
}
