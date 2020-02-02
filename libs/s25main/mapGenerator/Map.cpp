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
#include "mapGenerator/TextureMapping.h"

#include "gameData/MaxPlayers.h"
#include "libsiedler2/enumTypes.h"

#include <set>
#include <stdexcept>
#include <utility>

namespace rttr {
namespace mapGenerator {

// NEW WORLD

Map::Map(TextureMapping& mapping, DescIdx<LandscapeDesc> landscape, uint8_t players, const MapExtent& size) :
    mapping_(mapping),
    hqPositions_(MAX_PLAYERS, MapPoint::Invalid()),
    landscape_(landscape),
    name("Random"),
    author("Auto"),
    height(0, 44),
    players(players),
    size(size)
{
    terrains_ = mapping.GetTextures(landscape);
    z.Resize(size, height.min);
    textures.Resize(size);
    objectInfos.Resize(size, libsiedler2::OI_Empty);
    objectTypes.Resize(size, libsiedler2::OT_Empty);
    resources.Resize(size, libsiedler2::R_None);
    animals.Resize(size, libsiedler2::A_None);
}

void Map::MarkAsHarbor(const Triangle& triangle)
{
    harbors_.push_back(triangle);
}

void Map::MarkAsHeadQuarter(const MapPoint& position, int index)
{
    auto oldPosition = hqPositions_[index];
    
    hqPositions_[index] = position;

    if (position.isValid())
    {
        objectInfos[position] = libsiedler2::OI_HeadquarterMask;
        objectTypes[position] = libsiedler2::ObjectType(index);
    }
    else if (oldPosition.isValid())
    {
        objectInfos[oldPosition] = libsiedler2::OI_Empty;
        objectTypes[oldPosition] = libsiedler2::OT_Empty;
    }
}

libsiedler2::Archiv Map::CreateArchiv()
{
    libsiedler2::Archiv info;
    
    const unsigned numNodes = size.x * size.y;
    auto map = std::make_unique<libsiedler2::ArchivItem_Map>();
    auto header = std::make_unique<libsiedler2::ArchivItem_Map_Header>();

    // create header information for the archiv
    header->setName(name);
    header->setAuthor(author);
    header->setWidth(size.x);
    header->setHeight(size.y);
    header->setNumPlayers(players);
    header->setGfxSet(mapping_.GetLandscapeIndex(landscape_));

    // First 7 players go into the header
    for(unsigned i = 0; i < 7; i++)
    {
        header->setPlayerHQ(i, hqPositions_[i].x, hqPositions_[i].y);
    }

    std::vector<uint8_t> z(numNodes);
    std::vector<uint8_t> rsu(numNodes);
    std::vector<uint8_t> lsd(numNodes);
    std::vector<uint8_t> road(numNodes, 0x0);
    std::vector<uint8_t> objectType(numNodes);
    std::vector<uint8_t> objectInfo(numNodes);
    std::vector<uint8_t> animal(numNodes);
    std::vector<uint8_t> unknown1(numNodes, 0x0);
    std::vector<uint8_t> build(numNodes, 0x0);
    std::vector<uint8_t> unknown2(numNodes, 0x0);
    std::vector<uint8_t> unknown3(numNodes, 0x0);
    std::vector<uint8_t> resource(numNodes);
    std::vector<uint8_t> unknown5(numNodes, 0x0);
    
    for (unsigned i = 0; i < numNodes; i++)
    {
        z[i] = this->z[i];
        rsu[i] = mapping_.GetTerrainIndex(this->textures[i].rsu);
        lsd[i] = mapping_.GetTerrainIndex(this->textures[i].lsd);
        objectType[i] = this->objectTypes[i];
        objectInfo[i] = this->objectInfos[i];
        animal[i] = this->animals[i];
        resource[i] = this->resources[i];
    }
    
    for (Triangle t : harbors_)
    {
        if (t.rsu)
        {
            rsu[t.position.x + t.position.y * size.y] |= libsiedler2::HARBOR_MASK;
        }
        else
        {
            lsd[t.position.x + t.position.y * size.y] |= libsiedler2::HARBOR_MASK;
        }
    }
    
    map->push(std::move(header));
    map->push(std::make_unique<libsiedler2::ArchivItem_Raw>(z));
    map->push(std::make_unique<libsiedler2::ArchivItem_Raw>(rsu));
    map->push(std::make_unique<libsiedler2::ArchivItem_Raw>(lsd));
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


// OLD WORLD

Map_::Map_(WorldDescription& worldDesc, const MapExtent& size, int minHeight, int maxHeight)
    : worldDesc_(worldDesc),
      height(minHeight, maxHeight),
      size(size),
      numPlayers(0),
      hqPositions(MAX_PLAYERS, MapPoint::Invalid())
{
    const unsigned numNodes = size.x * size.y;

    z.resize(numNodes, 0x00);
    textureRsu.resize(numNodes);
    textureLsd.resize(numNodes);
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

libsiedler2::Archiv Map_::CreateArchiv()
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
    header->setGfxSet(worldDesc_.get(landscape).s2Id);

    // First 7 players go into the header
    for(unsigned i = 0; i < std::min<unsigned>(hqPositions.size(), 7); i++)
    {
        header->setPlayerHQ(i, hqPositions[i].x, hqPositions[i].y);
    }

    const int numNodes = size.x * size.y;
    
    std::vector<uint8_t> rsu(numNodes);
    std::vector<uint8_t> lsd(numNodes);
    
    for (int i = 0; i < numNodes; i++)
    {
        rsu[i] = worldDesc_.get(textureRsu[i]).s2Id;
        lsd[i] = worldDesc_.get(textureLsd[i]).s2Id;
    }
    
    for (int i: harborsRsu)
    {
        rsu[i] |= libsiedler2::HARBOR_MASK;
    }

    for (int i: harborsLsd)
    {
        lsd[i] |= libsiedler2::HARBOR_MASK;
    }

    map->push(std::move(header));
    map->push(std::make_unique<libsiedler2::ArchivItem_Raw>(z));
    map->push(std::make_unique<libsiedler2::ArchivItem_Raw>(rsu));
    map->push(std::make_unique<libsiedler2::ArchivItem_Raw>(lsd));
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

}}
