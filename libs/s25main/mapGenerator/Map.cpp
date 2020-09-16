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

#include "mapGenerator/Map.h"
#include "gameData/MaxPlayers.h"
#include "libsiedler2/Archiv.h"
#include "libsiedler2/ArchivItem_Map.h"
#include "libsiedler2/ArchivItem_Map_Header.h"
#include "libsiedler2/ArchivItem_Raw.h"
#include "libsiedler2/enumTypes.h"
#include <utility>

Map::Map() : size(0, 0), type(0), numPlayers(0) {}

Map::Map(const MapExtent& size, std::string name, std::string author)
    : size(size), name(std::move(name)), author(std::move(author)), type(0), numPlayers(0),
      hqPositions(MAX_PLAYERS, MapPoint::Invalid())
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

libsiedler2::Archiv Map::CreateArchiv()
{
    libsiedler2::Archiv info;
    auto map = std::make_unique<libsiedler2::ArchivItem_Map>();
    auto header = std::make_unique<libsiedler2::ArchivItem_Map_Header>();

    // create header information for the archiv
    header->setName(name);
    header->setAuthor(author);
    header->setWidth(size.x);
    header->setHeight(size.y);
    header->setNumPlayers(numPlayers);
    header->setGfxSet(type);

    // First 7 players go into the header
    for(unsigned i = 0; i < std::min<unsigned>(hqPositions.size(), 7); i++)
    {
        header->setPlayerHQ(i, hqPositions[i].x, hqPositions[i].y);
    }

    map->push(std::move(header));
    map->push(std::make_unique<libsiedler2::ArchivItem_Raw>(z));
    map->push(std::make_unique<libsiedler2::ArchivItem_Raw>(textureRsu));
    map->push(std::make_unique<libsiedler2::ArchivItem_Raw>(textureLsd));
    map->push(std::make_unique<libsiedler2::ArchivItem_Raw>(road));
    map->push(std::make_unique<libsiedler2::ArchivItem_Raw>(objectType));
    map->push(std::make_unique<libsiedler2::ArchivItem_Raw>(objectInfo));
    map->push(std::make_unique<libsiedler2::ArchivItem_Raw>(animal));
    map->push(std::make_unique<libsiedler2::ArchivItem_Raw>(unknown1));
    map->push(std::make_unique<libsiedler2::ArchivItem_Raw>(build));
    map->push(std::make_unique<libsiedler2::ArchivItem_Raw>(unknown2));
    map->push(std::make_unique<libsiedler2::ArchivItem_Raw>(unknown3));
    map->push(std::make_unique<libsiedler2::ArchivItem_Raw>(resource));
    map->push(nullptr); // No shading
    map->push(std::make_unique<libsiedler2::ArchivItem_Raw>(unknown5));

    info.push(std::move(map));

    return info;
}
