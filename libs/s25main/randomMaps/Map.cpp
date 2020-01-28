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
#include "randomMaps/Map.h"
#include "gameData/MaxPlayers.h"
#include "libsiedler2/enumTypes.h"

#include <iostream>

Map::Map() : size(0, 0), type(0), numPlayers(0) {}

Map::Map(const MapExtent& size, const std::string& name, const std::string& author)
    : size(size), name(name), author(author), type(0), numPlayers(0), hqPositions(MAX_PLAYERS, MapPoint::Invalid())
{
    const unsigned numNodes = size.x * size.y;

    z.resize(numNodes, 0x00);
    textureRsu.resize(numNodes, TextureType::Grass1);
    textureLsd.resize(numNodes, TextureType::Grass1);
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

libsiedler2::Archiv* Map::CreateArchiv(TextureMapping& mapping)
{
    auto info = new libsiedler2::Archiv();
    auto map = new libsiedler2::ArchivItem_Map();
    auto header = new libsiedler2::ArchivItem_Map_Header();

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

    const unsigned numNodes = size.x * size.y;
    std::vector<unsigned char> rsu(numNodes);
    std::vector<unsigned char> lsd(numNodes);

    for (auto i = 0u; i < numNodes; i++)
    {
        rsu[i] = mapping.Get(textureRsu[i]);
        lsd[i] = mapping.Get(textureLsd[i]);
    }
    
    for (auto i: harborsRsu)
    {
        rsu[i] |= libsiedler2::HARBOR_MASK;
    }

    for (auto i: harborsLsd)
    {
        lsd[i] |= libsiedler2::HARBOR_MASK;
    }

    map->push(std::unique_ptr<libsiedler2::ArchivItem>(header));
    map->push(std::unique_ptr<libsiedler2::ArchivItem>(new libsiedler2::ArchivItem_Raw(z)));
    map->push(std::unique_ptr<libsiedler2::ArchivItem>(new libsiedler2::ArchivItem_Raw(rsu)));
    map->push(std::unique_ptr<libsiedler2::ArchivItem>(new libsiedler2::ArchivItem_Raw(lsd)));
    map->push(std::unique_ptr<libsiedler2::ArchivItem>(new libsiedler2::ArchivItem_Raw(road)));
    map->push(std::unique_ptr<libsiedler2::ArchivItem>(new libsiedler2::ArchivItem_Raw(objectType)));
    map->push(std::unique_ptr<libsiedler2::ArchivItem>(new libsiedler2::ArchivItem_Raw(objectInfo)));
    map->push(std::unique_ptr<libsiedler2::ArchivItem>(new libsiedler2::ArchivItem_Raw(animal)));
    map->push(std::unique_ptr<libsiedler2::ArchivItem>(new libsiedler2::ArchivItem_Raw(unknown1)));
    map->push(std::unique_ptr<libsiedler2::ArchivItem>(new libsiedler2::ArchivItem_Raw(build)));
    map->push(std::unique_ptr<libsiedler2::ArchivItem>(new libsiedler2::ArchivItem_Raw(unknown2)));
    map->push(std::unique_ptr<libsiedler2::ArchivItem>(new libsiedler2::ArchivItem_Raw(unknown3)));
    map->push(std::unique_ptr<libsiedler2::ArchivItem>(new libsiedler2::ArchivItem_Raw(resource)));
    map->push(nullptr); // No shading
    map->push(std::unique_ptr<libsiedler2::ArchivItem>(new libsiedler2::ArchivItem_Raw(unknown5)));

    info->push(std::unique_ptr<libsiedler2::ArchivItem>(map));

    return info;
}
