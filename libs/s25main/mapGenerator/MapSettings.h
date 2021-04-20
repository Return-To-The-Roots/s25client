// Copyright (c) 2005 - 2017 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

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

    enum class IslandAmount : uint8_t
    {
        Few = 0,
        Normal = 10,
        Many = 30
    };
    constexpr auto maxEnumValue(IslandAmount) { return IslandAmount::Many; }

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
        IslandAmount islands = IslandAmount::Few;
        MountainDistance mountainDistance = MountainDistance::Normal;
        DescIdx<LandscapeDesc> type = DescIdx<LandscapeDesc>(0);
        MapStyle style = MapStyle::Mixed;
    };

}} // namespace rttr::mapGenerator
