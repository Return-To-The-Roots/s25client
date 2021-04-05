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
#include "helpers/containerUtils.h"
#include "gameData/MaxPlayers.h"
#include "libsiedler2/enumTypes.h"
#include <stdexcept>
#include <utility>

namespace rttr { namespace mapGenerator {

    Map::Map(const MapExtent& size, uint8_t players, const WorldDescription& worldDesc,
             DescIdx<LandscapeDesc> landscape, uint8_t maxHeight)
        : textureMap(TextureMap(worldDesc, landscape, textures)), name("Random"), author("Auto"), height(0, maxHeight),
          players(players), size(size)
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
        // create header information for the archiv
        auto header = std::make_unique<libsiedler2::ArchivItem_Map_Header>();
        header->setName(name);
        header->setAuthor(author);
        header->setWidth(size.x);
        header->setHeight(size.y);
        header->setNumPlayers(players);
        header->setGfxSet(textureMap.GetLandscapeId());

        if(players != hqPositions.size())
            throw std::logic_error("Not all players have an HQ set");
        RTTR_Assert(!helpers::contains(hqPositions, MapPoint::Invalid()));
        // First players go into the header
        const unsigned numPlayersInHeader =
          std::min(static_cast<unsigned>(header->maxPlayers), static_cast<unsigned>(hqPositions.size()));
        for(unsigned i = 0; i < numPlayersInHeader; i++)
        {
            header->setPlayerHQ(i, hqPositions[i].x, hqPositions[i].y);
        }

        auto map = std::make_unique<libsiedler2::ArchivItem_Map>();
        map->init(std::move(header));

        using libsiedler2::MapLayer;
        auto& z = map->getLayer(MapLayer::Altitude);
        auto& rsu = map->getLayer(MapLayer::Terrain1);
        auto& lsd = map->getLayer(MapLayer::Terrain2);
        auto& objectType = map->getLayer(MapLayer::ObjectType);
        auto& objectInfo = map->getLayer(MapLayer::ObjectIndex);
        auto& animal = map->getLayer(MapLayer::Animals);
        auto& resource = map->getLayer(MapLayer::Resources);

        z.assign(this->z.begin(), this->z.end());
        objectType.assign(this->objectTypes.begin(), this->objectTypes.end());
        objectInfo.assign(this->objectInfos.begin(), this->objectInfos.end());
        resource.assign(this->resources.begin(), this->resources.end());

        const unsigned numNodes = size.x * size.y;
        for(unsigned i = 0; i < numNodes; i++)
        {
            rsu[i] = textureMap.GetTextureId(this->textures[i].rsu);
            lsd[i] = textureMap.GetTextureId(this->textures[i].lsd);
            animal[i] = static_cast<uint8_t>(this->animals[i]);
        }

        for(unsigned i = 0; i < hqPositions.size(); i++)
        {
            if(hqPositions[i].isValid())
            {
                objectInfo[this->objectInfos.GetIdx(hqPositions[i])] = libsiedler2::OI_HeadquarterMask;
                objectType[this->objectTypes.GetIdx(hqPositions[i])] = i;
            }
        }

        for(const Triangle& t : harbors)
        {
            if(t.rsu)
                rsu[t.position.x + t.position.y * size.y] |= libsiedler2::HARBOR_MASK;
            else
                lsd[t.position.x + t.position.y * size.y] |= libsiedler2::HARBOR_MASK;
        }

        libsiedler2::Archiv info;
        info.push(std::move(map));

        return info;
    }

}} // namespace rttr::mapGenerator
