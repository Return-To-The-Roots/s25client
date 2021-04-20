// Copyright (c) 2005 - 2017 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "helpers/containerUtils.h"
#include "mapGenerator/NodeMapUtilities.h"
#include "mapGenerator/Textures.h"
#include "mapGenerator/Triangles.h"
#include "gameData/WorldDescription.h"
#include "libsiedler2/archives.h"

#include <cmath>
#include <string>

namespace rttr { namespace mapGenerator {

    class Map
    {
    private:
        std::vector<DescIdx<TerrainDesc>> terrains_;

    public:
        NodeMapBase<uint8_t> z;
        NodeMapBase<uint8_t> objectTypes;
        NodeMapBase<uint8_t> objectInfos;
        NodeMapBase<uint8_t> resources;
        NodeMapBase<libsiedler2::Animal> animals;
        std::vector<Triangle> harbors;
        std::vector<MapPoint> hqPositions;
        TextureMap textureMap;

        const std::string name;
        const std::string author;
        const ValueRange<uint8_t> height;
        const uint8_t players;
        const MapExtent size;

        Map(const MapExtent& size, uint8_t players, const WorldDescription& worldDesc, DescIdx<LandscapeDesc> landscape,
            uint8_t maxHeight = 0x60);

        /**
         * Creates a new archiv for this map.
         *
         * @return a new archiv containing the information of this map
         */
        libsiedler2::Archiv CreateArchiv() const;

        NodeMapBase<TexturePair>& getTextures() { return textureMap.textures_; }
        const NodeMapBase<TexturePair>& getTextures() const { return textureMap.textures_; }
    };

}} // namespace rttr::mapGenerator
