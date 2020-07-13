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

#include "PointOutput.h"
#include "mapGenerator/MapSettings.h"
#include "mapGenerator/RandomConfig.h"
#include "mapGenerator/RandomMapGenerator.h"
#include "mapGenerator/VertexUtility.h"
#include "gameData/MaxPlayers.h"
#include "libsiedler2/enumTypes.h"
#include <boost/test/unit_test.hpp>
#include <memory>
#include <vector>

BOOST_AUTO_TEST_SUITE(RandomMapGeneratorTest)

/**
 * Tests the RandomMapGenerator.Create method. The generated map must have the
 * same width and height as defined inside of the settings.
 */
BOOST_AUTO_TEST_CASE(Create_CorrectSize)
{
    RandomConfig config;
    BOOST_REQUIRE(config.Init(MapStyle::Random, DescIdx<LandscapeDesc>(0), 0x1337));
    MapSettings settings;
    settings.size = MapExtent(38, 30);
    settings.numPlayers = 1;
    settings.minPlayerRadius = 0.2;
    settings.maxPlayerRadius = 0.3;

    RandomMapGenerator generator(config);
    Map map(generator.Create(settings));
    BOOST_REQUIRE_EQUAL(map.size, settings.size);
}

/**
 * Tests the RandomMapGenerator.Create method. The generated map must contain the
 * the same number of headquarters as the number of players in the settings.
 */
BOOST_AUTO_TEST_CASE(Create_Headquarters)
{
    RandomConfig config;
    BOOST_REQUIRE(config.Init(MapStyle::Random, DescIdx<LandscapeDesc>(0), 0x1337));
    MapSettings settings;
    settings.size = MapExtent(30u, 32u);
    settings.numPlayers = 2;
    settings.minPlayerRadius = 0.2;
    settings.maxPlayerRadius = 0.3;

    RandomMapGenerator generator(config);

    Map map(generator.Create(settings));
    BOOST_REQUIRE_EQUAL(map.numPlayers, settings.numPlayers);

    unsigned minSize = std::min(map.size.x, map.size.y) / 2; //-V807
    for(unsigned i = 0; i < settings.numPlayers; i++)
    {
        Point<uint16_t> p = map.hqPositions[i];
        BOOST_REQUIRE_LT(p.x, map.size.x);
        BOOST_REQUIRE_LT(p.y, map.size.y);

        BOOST_REQUIRE_EQUAL(map.objectType[p.y * settings.size.x + p.x], i);
        BOOST_REQUIRE_EQUAL(map.objectInfo[p.y * settings.size.x + p.x], libsiedler2::OI_HeadquarterMask);

        double distance = VertexUtility::Distance(Position(p), Position(settings.size / 2), map.size);
        BOOST_REQUIRE_GE(distance, settings.minPlayerRadius * minSize);
        BOOST_REQUIRE_LE(distance, settings.maxPlayerRadius * minSize);
    }

    for(unsigned i = settings.numPlayers; i < MAX_PLAYERS; i++)
        BOOST_REQUIRE(!map.hqPositions[i].isValid());
}

BOOST_AUTO_TEST_CASE(InvalidConfig)
{
    RandomConfig config;
    BOOST_REQUIRE(config.Init(MapStyle::Random, DescIdx<LandscapeDesc>(0), 0x1337));
    RandomMapGenerator generator(config);

    MapSettings settings;
    settings.size = MapExtent(30u, 20u);
    settings.numPlayers = 0;
    settings.minPlayerRadius = 0.2;
    settings.maxPlayerRadius = 0.3;

    Map map(generator.Create(settings));
    BOOST_REQUIRE_GE(map.numPlayers, 1);

    settings.numPlayers = 99;
    map = generator.Create(settings);
    BOOST_REQUIRE_LT(map.numPlayers, 99);

    settings.numPlayers = 2;
    settings.minPlayerRadius = -1;
    settings.maxPlayerRadius = 100;
    map = generator.Create(settings);
    BOOST_REQUIRE(map.hqPositions[1].isValid());

    settings.minPlayerRadius = 1;
    settings.maxPlayerRadius = 1;
    map = generator.Create(settings);
    BOOST_REQUIRE(map.hqPositions[1].isValid());

    settings.minPlayerRadius = 1;
    settings.maxPlayerRadius = 0.5;
    map = generator.Create(settings);
    BOOST_REQUIRE(map.hqPositions[1].isValid());

    settings.ratioCoal = 0;
    settings.ratioGold = 0;
    settings.ratioGranite = 0;
    settings.ratioIron = 0;
    map = generator.Create(settings);

    settings.size = MapExtent(33, 35);
    map = generator.Create(settings);
    BOOST_REQUIRE_EQUAL(map.size, MapExtent(32, 34));
}

BOOST_AUTO_TEST_SUITE_END()
