// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "PointOutput.h"
#include "commonDefines.h"
#include "lua/GameDataLoader.h"
#include "mapGenerator/Algorithms.h"
#include "mapGenerator/RandomMap.h"
#include "mapGenerator/TextureHelper.h"
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

BOOST_AUTO_TEST_CASE(GetMaxMountainSize_grows_with_map_size)
{
    BOOST_TEST(GetMaxMountainSize(MapExtent(64, 64)) < GetMaxMountainSize(MapExtent(128, 128)));
    BOOST_TEST(GetMaxMountainSize(MapExtent(128, 128)) < GetMaxMountainSize(MapExtent(256, 256)));
    BOOST_TEST(GetMaxMountainSize(MapExtent(256, 256)) < GetMaxMountainSize(MapExtent(512, 512)));
    BOOST_TEST(GetMaxMountainSize(MapExtent(512, 512)) < GetMaxMountainSize(MapExtent(1024, 1024)));
}

BOOST_AUTO_TEST_CASE(GetMinFreeLand_keeps_mountain_distance_for_default_amount)
{
    const MapSettings settings;

    BOOST_TEST(GetMinFreeLand(15, settings.mountains) == 15u);
}

BOOST_AUTO_TEST_CASE(GetMinFreeLand_decreases_with_amount_of_mountains)
{
    BOOST_TEST(GetMinFreeLand(30, 25) > GetMinFreeLand(30, 50));
    BOOST_TEST(GetMinFreeLand(30, 50) > GetMinFreeLand(30, 75));
}

BOOST_AUTO_TEST_CASE(GetMinFreeLand_keeps_space_for_headquarters)
{
    BOOST_TEST(GetMinFreeLand(30, 100) == 4u);
    BOOST_TEST(GetMinFreeLand(5, 90) == 4u);
    BOOST_TEST(GetMinFreeLand(30, 1000) == 4u);
}

BOOST_AUTO_TEST_CASE(GenerateRandomMap_creates_more_mountains_for_higher_amount)
{
    WorldDescription worldDesc;
    loadGameData(worldDesc);
    MapSettings settings;

    settings.size = MapExtent(256, 256);
    settings.style = MapStyle::Mixed;

    const auto countMountainNodes = [&worldDesc, &settings](const unsigned short mountains) {
        settings.mountains = mountains;
        RandomUtility rnd(0);
        const Map map = GenerateRandomMap(rnd, worldDesc, settings);
        return SelectPoints([&map](const MapPoint& pt) { return map.textureMap.Any(pt, IsMinableMountain); }, map.size)
          .size();
    };

    const auto noMountains = countMountainNodes(0);
    const auto defaultMountains = countMountainNodes(50);
    const auto maxMountains = countMountainNodes(100);

    BOOST_TEST(noMountains < defaultMountains);
    BOOST_TEST(defaultMountains < maxMountains);
}

BOOST_AUTO_TEST_CASE(GenerateRandomMap_returns_valid_maps_for_maximum_amount_of_mountains)
{
    WorldDescription worldDesc;
    loadGameData(worldDesc);
    MapSettings settings;

    settings.size = MapExtent(128, 128);
    settings.mountains = 100;

    for(const unsigned numPlayers : {2u, MAX_PLAYERS})
    {
        for(const MapStyle style : {MapStyle::Land, MapStyle::Mixed, MapStyle::Water})
        {
            settings.numPlayers = numPlayers;
            settings.style = style;
            RandomUtility rnd(1);

            Map map = GenerateRandomMap(rnd, worldDesc, settings);
            ValidateMap(map, settings.size, settings.numPlayers);
        }
    }
}

BOOST_AUTO_TEST_CASE(GenerateRandomMap_leaves_no_large_free_land)
{
    WorldDescription worldDesc;
    loadGameData(worldDesc);
    MapSettings settings;

    // Smaller maps do not contain enough free land to require additional mountains
    settings.size = MapExtent(256, 256);
    settings.mountainDistance = MountainDistance::Close;

    // Maximum of the randomized free land to keep plus the transition textures added at the foot of mountains
    const unsigned maxFreeLand = static_cast<unsigned>(settings.mountainDistance) + 5 + 3;

    for(const MapStyle style : {MapStyle::Land, MapStyle::Mixed})
    {
        settings.style = style;
        RandomUtility rnd(0);

        const Map map = GenerateRandomMap(rnd, worldDesc, settings);
        const auto distances = DistancesTo(map.size, [&map](const MapPoint& pt) {
            return map.textureMap.Any(pt, [](const TerrainDesc& t) { return IsWater(t) || IsMountainOrSnowOrLava(t); });
        });

        BOOST_TEST(*std::max_element(distances.begin(), distances.end()) <= maxFreeLand);
    }
}

BOOST_AUTO_TEST_CASE(GenerateRandomMap_returns_valid_maps_for_all_styles_and_mountain_distances)
{
    WorldDescription worldDesc;
    loadGameData(worldDesc);
    MapSettings settings;

    settings.size = MapExtent(96, 96);
    settings.numPlayers = 4;

    for(const MapStyle style : {MapStyle::Land, MapStyle::Mixed, MapStyle::Water})
    {
        for(const MountainDistance distance : {MountainDistance::Close, MountainDistance::VeryFar})
        {
            settings.style = style;
            settings.mountainDistance = distance;
            RandomUtility rnd(1);

            Map map = GenerateRandomMap(rnd, worldDesc, settings);
            ValidateMap(map, settings.size, settings.numPlayers);
        }
    }
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
