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
             DescIdx<LandscapeDesc> landscape, uint8_t maxHeight)
        : textureMap(TextureMap(worldDesc, landscape, textures)),
          name("Random"), author("Auto"), height(0, maxHeight), players(players), size(size)
    {
        z.Resize(size);
        textures.Resize(size);
        objectInfos.Resize(size, libsiedler2::OI_Empty);
        objectTypes.Resize(size, libsiedler2::OT_Empty);
        resources.Resize(size, libsiedler2::R_None);
        animals.Resize(size, libsiedler2::Animal::None);
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
        header->setGfxSet(textureMap.GetLandscapeId());

        // First players go into the header
        const unsigned numPlayersInHeader =
          std::min(static_cast<unsigned>(header->maxPlayers), static_cast<unsigned>(hqPositions.size()));
        for(unsigned i = 0; i < numPlayersInHeader; i++)
        {
            header->setPlayerHQ(i, hqPositions[i].x, hqPositions[i].y);
        }

        std::vector<uint8_t> z(numNodes);
        std::vector<uint8_t> rsu(numNodes);
        std::vector<uint8_t> lsd(numNodes);
        std::vector<uint8_t> road(numNodes);
        std::vector<uint8_t> objectType(numNodes);
        std::vector<uint8_t> objectInfo(numNodes);
        std::vector<uint8_t> animal(numNodes);
        std::vector<uint8_t> unknown1(numNodes);
        std::vector<uint8_t> build(numNodes);
        std::vector<uint8_t> unknown2(numNodes);
        std::vector<uint8_t> unknown3(numNodes);
        std::vector<uint8_t> resource(numNodes);
        std::vector<uint8_t> unknown5(numNodes);

        for(unsigned i = 0; i < numNodes; i++)
        {
            z[i] = this->z[i];
            rsu[i] = textureMap.GetTextureId(this->textures[i].rsu);
            lsd[i] = textureMap.GetTextureId(this->textures[i].lsd);
            objectType[i] = this->objectTypes[i];
            objectInfo[i] = this->objectInfos[i];
            animal[i] = static_cast<uint8_t>(this->animals[i]);
            resource[i] = this->resources[i];
        }

        for(unsigned i = 0; i < hqPositions.size(); i++)
        {
            if(hqPositions[i].isValid())
            {
                objectInfo[this->objectInfos.GetIdx(hqPositions[i])] = libsiedler2::OI_HeadquarterMask;
                objectType[this->objectTypes.GetIdx(hqPositions[i])] = libsiedler2::ObjectType(i);
            }
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
        for(const auto& cur :
            {z, rsu, lsd, road, objectType, objectInfo, animal, unknown1, build, unknown2, unknown3, resource})
        {
            map->push(std::make_unique<libsiedler2::ArchivItem_Raw>(cur));
        }
        map->push(nullptr); // No shading
        map->push(std::make_unique<libsiedler2::ArchivItem_Raw>(unknown5));

        info.push(std::move(map));

        return info;
    }

}} // namespace rttr::mapGenerator
