// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "PointOutput.h"
#include "mapGenFixtures.h"
#include "mapGenerator/HeadQuarters.h"
#include "mapGenerator/TextureHelper.h"
#include "rttr/test/random.hpp"
#include <boost/test/unit_test.hpp>

using namespace rttr::mapGenerator;

static auto getAllPoints(const MapExtent size)
{
    std::vector<MapPoint> allPositions;
    allPositions.reserve(prodOfComponents(size));

    RTTR_FOREACH_PT(MapPoint, size)
    {
        allPositions.push_back(pt);
    }
    return allPositions;
}

BOOST_FIXTURE_TEST_SUITE(HeadQuartersTests, MapGenFixture)

BOOST_AUTO_TEST_CASE(FindLargestConnectedArea_returns_expected_nodes)
{
    const MapExtent size(28, 32);
    Map map = createMap(size);
    TextureMap& textures = map.textureMap;

    const auto water = textures.Find(IsShipableWater);
    const auto land = textures.Find(IsBuildableLand);
    map.getTextures().Resize(size, TexturePair(water));
    MapPoint centerOfLargeArea(2, 3);
    const auto largeArea = map.getTextures().GetPointsInRadiusWithCenter(centerOfLargeArea, 3);

    for(auto node : largeArea)
    {
        textures.Set(node, land);
    }

    MapPoint centerOfSmallArea(13, 15);
    const auto smallArea = map.getTextures().GetPointsInRadiusWithCenter(centerOfSmallArea, 2);

    for(auto node : smallArea)
    {
        textures.Set(node, land);
    }

    const auto result = FindLargestConnectedArea(map);

    BOOST_TEST_REQUIRE(result.size() == largeArea.size());

    for(auto node : largeArea)
    {
        BOOST_TEST_REQUIRE(helpers::contains(result, node));
    }
}

BOOST_AUTO_TEST_CASE(FindHqPositions_returns_empty_for_map_without_suitable_position)
{
    const MapExtent size(28, 32);
    Map map = createMap(size);
    const TextureMap& textures = map.textureMap;

    map.getTextures().Resize(size, TexturePair(textures.Find(IsShipableWater)));
    const auto mntDist = rttr::test::randomEnum<MountainDistance>();
    const auto positions = FindHqPositions(map, getAllPoints(size), mntDist);

    BOOST_TEST(positions.empty());
}

BOOST_AUTO_TEST_CASE(FindHqPositions_returns_suitable_position_for_single_player)
{
    const MapExtent size(6, 8);
    Map map = createMap(size);
    const TextureMap& textures = map.textureMap;

    const auto water = textures.Find(IsWater);
    const auto buildable = textures.Find(IsBuildableLand);

    MapPoint obstacle(1, 3);

    map.getTextures().Resize(size, TexturePair(buildable));
    map.getTextures()[obstacle] = TexturePair(water);

    const auto mntDist = rttr::test::randomEnum<MountainDistance>();
    const auto positions = FindHqPositions(map, getAllPoints(size), mntDist);

    BOOST_TEST(!positions.empty());
}

BOOST_AUTO_TEST_CASE(PlaceHeadquarter_with_suitable_position_for_player)
{
    const MapExtent size(6, 8);
    Map map = createMap(size);
    const TextureMap& textures = map.textureMap;

    const auto mountain = textures.Find(IsMinableMountain);
    const auto buildable = textures.Find(IsBuildableLand);

    MapPoint obstacle(1, 2);
    MapPoint hq(3, 4);

    map.getTextures().Resize(size, TexturePair(buildable));
    map.getTextures()[obstacle] = TexturePair(mountain);

    std::vector<MapPoint> area{hq};
    BOOST_REQUIRE_NO_THROW(PlaceHeadquarter(map, area, MountainDistance::Normal));
    BOOST_TEST_REQUIRE(map.hqPositions.size() == 1u);
    BOOST_TEST(map.hqPositions[0] == hq);
}

BOOST_AUTO_TEST_CASE(PlaceHeadquarter_with_suitable_position_without_mountain)
{
    const MapExtent size(6, 8);
    Map map = createMap(size);
    const TextureMap& textures = map.textureMap;

    const auto mntDist = rttr::test::randomEnum<MountainDistance>();
    const auto buildable = textures.Find(IsBuildableLand);
    MapPoint hq(4, 4);
    map.getTextures().Resize(size, TexturePair(buildable));
    std::vector<MapPoint> area{hq};
    BOOST_REQUIRE_NO_THROW(PlaceHeadquarter(map, area, mntDist));
    BOOST_TEST_REQUIRE(map.hqPositions.size() == 1u);
    BOOST_TEST(map.hqPositions[0] == hq);
}

BOOST_AUTO_TEST_CASE(PlaceHeadquarter_places_hq_on_map_at_suitable_position)
{
    const MapExtent size(6, 8);
    Map map = createMap(size);
    const TextureMap& textures = map.textureMap;

    const auto mntDist = rttr::test::randomEnum<MountainDistance>();
    const MapPoint obstacle(1, 3);
    const MapPoint hq(4, 5);

    map.getTextures().Resize(size, TexturePair(textures.Find(IsBuildableLand)));
    map.getTextures()[obstacle] = TexturePair(textures.Find(IsWater));

    std::vector<MapPoint> area{hq};

    PlaceHeadquarter(map, area, mntDist);
    BOOST_TEST_REQUIRE(map.hqPositions.size() == 1u);
    BOOST_TEST(map.hqPositions[0] == hq);
}

BOOST_AUTO_TEST_CASE(PlaceHeadquarter_with_empty_area_throws_exception)
{
    const MapExtent size(6, 8);
    Map map = createMap(size);
    const TextureMap& textures = map.textureMap;

    const auto mntDist = rttr::test::randomEnum<MountainDistance>();
    const auto water = textures.Find(IsShipableWater);
    map.getTextures().Resize(size, TexturePair(water));
    std::vector<MapPoint> area;
    BOOST_CHECK_THROW(PlaceHeadquarter(map, area, mntDist), std::runtime_error);
}

BOOST_AUTO_TEST_CASE(PlaceHeadquarter_without_suitable_position_throws_exception)
{
    const MapExtent size(6, 8);
    Map map = createMap(size);
    const TextureMap& textures = map.textureMap;

    const auto mntDist = rttr::test::randomEnum<MountainDistance>();
    const auto water = textures.Find(IsShipableWater);
    map.getTextures().Resize(size, TexturePair(water));
    std::vector<MapPoint> area{MapPoint(0, 0)};
    BOOST_CHECK_THROW(PlaceHeadquarter(map, area, mntDist), std::runtime_error);
}

BOOST_AUTO_TEST_CASE(PlaceHeadquarters_with_suitable_positions_for_all_players)
{
    const MapExtent size(28, 32);
    RandomUtility rnd(0);
    const int players = rttr::test::randomValue(1, 8);
    Map map = createMap(size, players);
    const TextureMap& textures = map.textureMap;

    const auto mntDist = rttr::test::randomEnum<MountainDistance>();
    map.getTextures().Resize(size, TexturePair(textures.Find(IsBuildableLand)));
    map.getTextures()[MapPoint(0, 0)] = TexturePair(textures.Find(IsWater));
    BOOST_REQUIRE_NO_THROW(PlaceHeadquarters(map, rnd, players, mntDist));
}

BOOST_AUTO_TEST_CASE(PlaceHeadquarters_places_hqs_for_any_player_number_on_suitable_map)
{
    const MapExtent size(28, 32);
    RandomUtility rnd(0);
    const unsigned players = rttr::test::randomValue(1u, 8u);
    Map map = createMap(size, players);
    const TextureMap& textures = map.textureMap;

    const auto mntDist = rttr::test::randomEnum<MountainDistance>();
    map.getTextures().Resize(size, textures.Find(IsBuildableLand));
    map.getTextures()[MapPoint(0, 0)] = TexturePair(textures.Find(IsWater));
    PlaceHeadquarters(map, rnd, players, mntDist);
    BOOST_TEST_REQUIRE(map.hqPositions.size() == players);
    for(unsigned index = 0; index < players; index++)
    {
        BOOST_TEST_REQUIRE(map.hqPositions[index].isValid());
    }
}

BOOST_AUTO_TEST_CASE(PlaceHeadquarters_without_suitable_position_throws_exception)
{
    const MapExtent size(28, 32);
    RandomUtility rnd(0);
    const int players = rttr::test::randomValue(1, 8);
    Map map = createMap(size, players);
    const TextureMap& textures = map.textureMap;

    const auto mntDist = rttr::test::randomEnum<MountainDistance>();
    map.getTextures().Resize(size, textures.Find(IsWater));
    BOOST_CHECK_THROW(PlaceHeadquarters(map, rnd, players, mntDist), std::runtime_error);
}

BOOST_AUTO_TEST_SUITE_END()
