// Copyright (c) 2017 - 2020 Settlers Freaks (sf-team at siedler25.org)
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
#include "libsiedler2/enumTypes.h"

#include <stdexcept>
#include <utility>

namespace rttr { namespace mapGenerator {

    Map::Map(const MapExtent& size, uint8_t players, const WorldDescription& worldDesc,
             const DescIdx<LandscapeDesc>& landscape)
        : hqPositions_(MAX_PLAYERS, MapPoint::Invalid()), z(size, 0), textures(TextureMap(worldDesc, landscape)),
          name("Random"), author("Auto"), height(0, 60), players(players), size(size)
    {
        textures.Resize(size);
        objectInfos.Resize(size, libsiedler2::OI_Empty);
        objectTypes.Resize(size, libsiedler2::OT_Empty);
        resources.Resize(size, libsiedler2::R_None);
        animals.Resize(size, libsiedler2::Animal::None);
    }

    void Map::MarkAsHeadQuarter(const MapPoint& position, int index)
    {
        auto oldPosition = hqPositions_[index];

        hqPositions_[index] = position;

        if(position.isValid())
        {
            objectInfos[position] = libsiedler2::OI_HeadquarterMask;
            objectTypes[position] = libsiedler2::ObjectType(index);
        } else if(oldPosition.isValid())
        {
            objectInfos[oldPosition] = libsiedler2::OI_Empty;
            objectTypes[oldPosition] = libsiedler2::OT_Empty;
        }
    }

    libsiedler2::Archiv Map::CreateArchiv() const
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
        header->setGfxSet(textures.GetLandscapeId());

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

        for(unsigned i = 0; i < numNodes; i++)
        {
            z[i] = this->z[i];
            rsu[i] = textures.GetTextureId(this->textures[i].rsu);
            lsd[i] = textures.GetTextureId(this->textures[i].lsd);
            objectType[i] = this->objectTypes[i];
            objectInfo[i] = this->objectInfos[i];
            animal[i] = static_cast<uint8_t>(this->animals[i]);
            resource[i] = this->resources[i];
        }

        for(const Triangle& t : harbors)
        {
            if(t.rsu)
            {
                rsu[t.position.x + t.position.y * size.y] |= libsiedler2::HARBOR_MASK;
            } else
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

}} // namespace rttr::mapGenerator
