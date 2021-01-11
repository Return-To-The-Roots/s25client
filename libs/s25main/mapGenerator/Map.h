// Copyright (c) 2005 - 2017 Settlers Freaks (sf-team at siedler25.org)
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
        DescIdx<LandscapeDesc> landscape_;

    public:
        NodeMapBase<uint8_t> z;
        NodeMapBase<TexturePair> textures;
        NodeMapBase<uint8_t> objectInfos;
        NodeMapBase<uint8_t> objectTypes;
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
         * Marks the position as HQ position if set to a valid position, otherwise unmarks previously marked position.
         *
         * @param position position to mark or unmark as HQ position
         * @param index index of the player
         */
        void MarkAsHeadQuarter(const MapPoint& position, int index);

        /**
         * Creates a new archiv for this map.
         *
         * @return a new archiv containing the information of this map
         */
        libsiedler2::Archiv CreateArchiv() const;
    };

}} // namespace rttr::mapGenerator
