// Copyright (c) 2017 - 2017 Settlers Freaks (sf-team at siedler25.org)
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
#include "mapGenerator/Map.h"
#include "gameData/MaxPlayers.h"
#include "libsiedler2/enumTypes.h"

Map::Map() : size(0, 0) {}

Map::Map(const MapExtent& size, const std::string& name, const std::string& author)
    : size(size), name(name), author(author), positions(MAX_PLAYERS, Point<uint16_t>(0xFF, 0xFF))
{
    const unsigned numNodes = size.x * size.y;

    z.resize(numNodes, 0x00);
    textureRsu.resize(numNodes, 0x08);
    textureLsd.resize(numNodes, 0x08);
    build.resize(numNodes, 0x04);
    shading.resize(numNodes, 0x40);
    resource.resize(numNodes, libsiedler2::R_Water);
    road.resize(numNodes, 0x00);
    objectType.resize(numNodes, 0x00);
    objectInfo.resize(numNodes, 0x00);
    animal.resize(numNodes, 0x00);
    unknown1.resize(numNodes, 0x00);
    unknown2.resize(numNodes, 0x07);
    unknown3.resize(numNodes, 0x00);
    unknown5.resize(numNodes, 0x00);
}

libsiedler2::Archiv* Map::CreateArchiv()
{
    libsiedler2::Archiv* info = new libsiedler2::Archiv();
    libsiedler2::ArchivItem_Map* map = new libsiedler2::ArchivItem_Map();
    libsiedler2::ArchivItem_Map_Header* header = new libsiedler2::ArchivItem_Map_Header();
    std::vector<unsigned char> data;

    // create header information for the archiv
    header->setName(name);
    header->setAuthor(author);
    header->setWidth(size.x);
    header->setHeight(size.y);
    header->setNumPlayers(players);
    header->setGfxSet(type);

    for(unsigned i = 0; i < positions.size(); i++)
    {
        header->setPlayerHQ(i, positions[i].x, positions[i].y);
    }

    map->push(header);
    map->push(new libsiedler2::ArchivItem_Raw(z));
    map->push(new libsiedler2::ArchivItem_Raw(textureRsu));
    map->push(new libsiedler2::ArchivItem_Raw(textureLsd));
    map->push(new libsiedler2::ArchivItem_Raw(road));
    map->push(new libsiedler2::ArchivItem_Raw(objectType));
    map->push(new libsiedler2::ArchivItem_Raw(objectInfo));
    map->push(new libsiedler2::ArchivItem_Raw(animal));
    map->push(new libsiedler2::ArchivItem_Raw(unknown1));
    map->push(new libsiedler2::ArchivItem_Raw(build));
    map->push(new libsiedler2::ArchivItem_Raw(unknown2));
    map->push(new libsiedler2::ArchivItem_Raw(unknown3));
    map->push(new libsiedler2::ArchivItem_Raw(resource));
    map->push(NULL); // No shading
    map->push(new libsiedler2::ArchivItem_Raw(unknown5));

    info->push(map);

    return info;
}
