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

#include "gameTypes/MapCoordinates.h"
#include "gameData/DescIdx.h"
#include "gameData/LandscapeDesc.h"
#include <string>

namespace rttr { namespace mapGenerator {

    enum class MapStyle
    {
        Water,
        Land,
        Mixed
    };

    enum class MountainDistance : uint8_t
    {
        Close = 5,
        Normal = 15,
        Far = 25,
        VeryFar = 30
    };
    constexpr auto maxEnumValue(MountainDistance) { return MountainDistance::VeryFar; }

    struct MapSettings
    {
        void MakeValid();

        std::string name, author;
        unsigned numPlayers = 2;
        MapExtent size = MapExtent::all(128);
        unsigned short ratioGold = 9;
        unsigned short ratioIron = 36;
        unsigned short ratioCoal = 40;
        unsigned short ratioGranite = 15;
        unsigned short rivers = 15;
        unsigned short trees = 40;
        unsigned short stonePiles = 5;
        MountainDistance mountainDistance = MountainDistance::Normal;
        DescIdx<LandscapeDesc> type = DescIdx<LandscapeDesc>(0);
        MapStyle style = MapStyle::Mixed;
    };

}} // namespace rttr::mapGenerator
