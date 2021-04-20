// Copyright (c) 2020 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "lua/GameDataLoader.h"
#include "mapGenerator/Map.h"
#include "gameData/WorldDescription.h"

struct MapGenFixture
{
    DescIdx<LandscapeDesc> landscape = DescIdx<LandscapeDesc>(1);
    WorldDescription worldDesc;
    using Map = rttr::mapGenerator::Map;

    MapGenFixture() { loadGameData(worldDesc); }

    Map createMap(const MapExtent size, unsigned numPlayers = 1) const
    {
        return Map(size, numPlayers, worldDesc, landscape);
    }
};
