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
#include "commonDefines.h"
#include "lua/GameDataLoader.h"
#include "mapGenerator/RandomMap.h"
#include "gameData/MaxPlayers.h"
#include "libsiedler2/ArchivItem_Map.h"
#include "libsiedler2/ArchivItem_Map_Header.h"
#include "libsiedler2/libsiedler2.h"
#include "rttr/test/TmpFolder.hpp"
#include "rttr/test/random.hpp"
#include <boost/test/unit_test.hpp>

using namespace rttr::mapGenerator;

BOOST_AUTO_TEST_SUITE(RandomMapTests)

static void ValidateMap(const Map& map, const MapExtent& size, unsigned players)
{
    BOOST_TEST_REQUIRE(map.players == players);
    BOOST_TEST(map.size == size);

    const auto validateHqPos = [&](const MapPoint hqPos) {
        // All textures around the point must allow a castle
        const auto neighbors = map.z.GetNeighbours(hqPos);
        for(const MapPoint pt : neighbors)
        {
            BOOST_TEST_REQUIRE(
              map.textureMap.All(pt, [](const TerrainDesc& terrain) { return terrain.GetBQ() == TerrainBQ::Castle; }),
              "At point " << pt);
        }
        // All direct neighbors must have the same height as the hqPos
        for(const MapPoint pt : neighbors)
            BOOST_TEST_REQUIRE(map.z[pt] == map.z[hqPos], "At point " << pt);

        const auto neighbors2 = map.z.GetPointsInRadiusWithCenter(hqPos, 2);

        const auto compareHeight = [&](const MapPoint& p1, const MapPoint& p2) { return map.z[p1] < map.z[p2]; };
        const MapPoint lowestPoint = *std::min_element(neighbors2.begin(), neighbors2.end(), compareHeight);
        const MapPoint highestPoint = *std::max_element(neighbors2.begin(), neighbors2.end(), compareHeight);
        // Height difference must be at most 2
        BOOST_TEST(absDiff(map.z[hqPos], map.z[lowestPoint]) <= 2);
        BOOST_TEST(absDiff(map.z[hqPos], map.z[highestPoint]) <= 2);
        // No objects around
        for(const MapPoint pt : neighbors2)
        {
            if(pt == hqPos) // This is the HQ
                continue;
            BOOST_TEST_REQUIRE(map.objectTypes[pt] == libsiedler2::OT_Empty, "At point " << pt);
        }
    };
    for(unsigned index = 0; index < players; index++)
    {
        BOOST_TEST_REQUIRE(map.hqPositions[index].isValid());
        BOOST_TEST_CONTEXT("Player " << index) { validateHqPos(map.hqPositions[index]); }
    }
}

static MapExtent getRandomMapSize(const unsigned minSize = 32, const unsigned maxSize = 80)
{
    auto size = rttr::test::randomPoint<MapExtent>(minSize, maxSize);
    // Need even size
    size.x &= ~1u;
    size.y &= ~1u;
    return size;
}

BOOST_AUTO_TEST_CASE(GenerateRandomMap_returns_valid_land_map)
{
    RandomUtility rnd(0);
    WorldDescription worldDesc;
    loadGameData(worldDesc);
    MapSettings settings;

    settings.size = getRandomMapSize(32, 32);
    settings.numPlayers = rttr::test::randomValue(2u, MAX_PLAYERS);
    settings.style = MapStyle::Land;

    Map map = GenerateRandomMap(rnd, worldDesc, settings);
    ValidateMap(map, settings.size, settings.numPlayers);
}

BOOST_AUTO_TEST_CASE(GenerateRandomMap_returns_valid_water_map)
{
    RandomUtility rnd(0);
    WorldDescription worldDesc;
    loadGameData(worldDesc);
    MapSettings settings;

    settings.size = getRandomMapSize(80, 90); // Need enough space for player islands
    settings.numPlayers = rttr::test::randomValue(2u, MAX_PLAYERS);
    settings.style = MapStyle::Water;

    Map map = GenerateRandomMap(rnd, worldDesc, settings);
    ValidateMap(map, settings.size, settings.numPlayers);
}

BOOST_AUTO_TEST_CASE(GenerateRandomMap_returns_valid_mixed_map)
{
    RandomUtility rnd(0);
    WorldDescription worldDesc;
    loadGameData(worldDesc);
    MapSettings settings;

    settings.size = getRandomMapSize();
    settings.numPlayers = rttr::test::randomValue(2u, MAX_PLAYERS);
    settings.style = MapStyle::Mixed;

    Map map = GenerateRandomMap(rnd, worldDesc, settings);
    ValidateMap(map, settings.size, settings.numPlayers);
}

BOOST_AUTO_TEST_CASE(GenerateRandomMap_for_many_islands_returns_valid_map)
{
    RandomUtility rnd(0);
    WorldDescription worldDesc;
    loadGameData(worldDesc);
    MapSettings settings;

    settings.islands = rttr::mapGenerator::IslandAmount::Many;
    settings.size = getRandomMapSize();
    settings.numPlayers = 2u;
    settings.style = MapStyle::Mixed;

    Map map = GenerateRandomMap(rnd, worldDesc, settings);
    ValidateMap(map, settings.size, settings.numPlayers);
}

BOOST_AUTO_TEST_CASE(GenerateRandomMap_returns_valid_map_with_max_players)
{
    RandomUtility rnd(0);
    WorldDescription worldDesc;
    loadGameData(worldDesc);
    MapSettings settings;

    settings.size = getRandomMapSize(38);
    settings.numPlayers = MAX_PLAYERS;
    settings.style = MapStyle::Land;

    Map map = GenerateRandomMap(rnd, worldDesc, settings);
    ValidateMap(map, settings.size, settings.numPlayers);
}

BOOST_AUTO_TEST_CASE(GenerateRandomMap_returns_valid_mapfile_with_max_players)
{
    rttr::test::TmpFolder tmpFolder;
    const auto mapPath = tmpFolder.get() / "map.wld";

    MapSettings settings;
    settings.size = MapExtent(64, 64);
    settings.numPlayers = MAX_PLAYERS;
    settings.style = MapStyle::Land;

    CreateRandomMap(mapPath, settings);
    libsiedler2::Archiv archive;
    BOOST_TEST_REQUIRE(libsiedler2::Load(mapPath, archive) == 0);
    const auto* map = dynamic_cast<const libsiedler2::ArchivItem_Map*>(archive[0]);
    BOOST_TEST_REQUIRE(map);
    const auto& mapHeader = map->getHeader();
    BOOST_TEST(mapHeader.getWidth() == settings.size.x);
    BOOST_TEST(mapHeader.getHeight() == settings.size.y);
    BOOST_TEST(mapHeader.getNumPlayers() == settings.numPlayers);
    // Must have unique HQ positions
    std::vector<MapPoint> hqs;
    for(unsigned i = 0; i < std::min(unsigned(mapHeader.maxPlayers), settings.numPlayers); i++)
    {
        Point<uint16_t> hqPos;
        mapHeader.getPlayerHQ(i, hqPos.x, hqPos.y);
        BOOST_TEST(!helpers::contains(hqs, MapPoint(hqPos)));
        hqs.emplace_back(hqPos);
    }
}

BOOST_AUTO_TEST_SUITE_END()
