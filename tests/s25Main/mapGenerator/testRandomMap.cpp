// Copyright (c) 2017 - 2017 Settlers Freaks (sf-team at siedler25.org)
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

#include "lua/GameDataLoader.h"
#include "mapGenerator/RandomMap.h"
#include <boost/test/unit_test.hpp>

using namespace rttr::mapGenerator;

BOOST_AUTO_TEST_SUITE(RandomMapTests)

void ValidateMap(const Map& map, const MapExtent& size, unsigned players);

void ValidateMap(const Map& map, const MapExtent& size, unsigned players)
{
    BOOST_REQUIRE(map.players == players);
    BOOST_REQUIRE(map.size == size);

    for(unsigned index = 0; index < players; index++)
    {
        BOOST_REQUIRE(map.hqPositions[index].isValid());
    }
}

BOOST_AUTO_TEST_CASE(GenerateRandomMap_returns_valid_tiny_7_player_land_map)
{
    RandomUtility rnd(0);
    WorldDescription worldDesc;
    loadGameData(worldDesc);
    MapSettings settings;

    settings.size = MapExtent(64, 64);
    settings.numPlayers = 7u;
    settings.style = MapStyle::Land;

    Map map = GenerateRandomMap(rnd, worldDesc, settings);
    ValidateMap(map, settings.size, settings.numPlayers);
}

BOOST_AUTO_TEST_CASE(GenerateRandomMap_returns_valid_tiny_7_player_water_map)
{
    RandomUtility rnd(0);
    WorldDescription worldDesc;
    loadGameData(worldDesc);
    MapSettings settings;

    settings.size = MapExtent(64, 64);
    settings.numPlayers = 7u;
    settings.style = MapStyle::Water;

    Map map = GenerateRandomMap(rnd, worldDesc, settings);
    ValidateMap(map, settings.size, settings.numPlayers);
}

BOOST_AUTO_TEST_CASE(GenerateRandomMap_returns_valid_tiny_7_player_mixed_map)
{
    RandomUtility rnd(0);
    WorldDescription worldDesc;
    loadGameData(worldDesc);
    MapSettings settings;

    settings.size = MapExtent(64, 64);
    settings.numPlayers = 7u;
    settings.style = MapStyle::Mixed;

    Map map = GenerateRandomMap(rnd, worldDesc, settings);
    ValidateMap(map, settings.size, settings.numPlayers);
}

BOOST_AUTO_TEST_SUITE_END()
