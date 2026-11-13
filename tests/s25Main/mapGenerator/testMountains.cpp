// Copyright (C) 2005 - 2026 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "PointOutput.h"
#include "mapGenFixtures.h"
#include "mapGenerator/Algorithms.h"
#include "mapGenerator/Mountains.h"
#include "mapGenerator/TextureHelper.h"
#include <boost/test/unit_test.hpp>
#include <algorithm>

using namespace rttr::mapGenerator;

namespace {
constexpr uint8_t landHeight = 10;
constexpr unsigned mountainLevel = 30;
constexpr MountainSettings settings{3, 100, 25};

struct MountainFixture : MapGenFixture
{
    /// Creates a map covered by land except for a single row of water
    Map createLandMap(const MapExtent size) const
    {
        Map map = createMap(size);
        map.z.Resize(size, landHeight);
        RTTR_FOREACH_PT(MapPoint, size)
        {
            if(pt.y == 0)
                map.z[pt] = map.height.minimum;
        }
        return map;
    }
};

unsigned maxDistance(const NodeMapBase<unsigned>& distances)
{
    return *std::max_element(distances.begin(), distances.end());
}
} // namespace

BOOST_FIXTURE_TEST_SUITE(MountainsTests, MountainFixture)

BOOST_AUTO_TEST_CASE(GetMountainSize_starts_with_maximum_size)
{
    BOOST_TEST(GetMountainSize(settings, 0) == settings.maxSize);
}

BOOST_AUTO_TEST_CASE(GetMountainSize_decreases_with_index)
{
    BOOST_TEST(GetMountainSize(settings, 1) < GetMountainSize(settings, 0));
    BOOST_TEST(GetMountainSize(settings, 2) < GetMountainSize(settings, 1));
}

BOOST_AUTO_TEST_CASE(GetMountainSize_never_drops_below_minimum_size)
{
    BOOST_TEST(GetMountainSize(settings, 1000) == settings.minSize);
}

BOOST_AUTO_TEST_CASE(GetFreeLandDistances_is_zero_for_water)
{
    const Map map = createLandMap(MapExtent(16, 16));
    const auto distances = GetFreeLandDistances(map, mountainLevel);

    BOOST_TEST(distances[MapPoint(5, 0)] == 0u);
    BOOST_TEST(distances[MapPoint(5, 1)] == 1u);
    BOOST_TEST(distances[MapPoint(5, 8)] == 8u);
}

BOOST_AUTO_TEST_CASE(GetFreeLandDistances_is_zero_for_nodes_at_mountain_level)
{
    Map map = createLandMap(MapExtent(16, 16));
    const MapPoint mountain(8, 8);
    map.z[mountain] = mountainLevel;

    const auto distances = GetFreeLandDistances(map, mountainLevel);

    BOOST_TEST(distances[mountain] == 0u);
    BOOST_TEST(distances[MapPoint(9, 8)] == 1u);
}

BOOST_AUTO_TEST_CASE(GetFreeLandDistances_is_zero_for_textured_nodes)
{
    Map map = createLandMap(MapExtent(16, 16));
    const MapPoint river(8, 8);
    map.textureMap.Set(river, map.textureMap.Find(IsWater));

    const auto distances = GetFreeLandDistances(map, mountainLevel);

    BOOST_TEST(distances[river] == 0u);
}

BOOST_AUTO_TEST_CASE(GrowMountain_contains_center_even_if_size_is_one)
{
    const Map map = createLandMap(MapExtent(32, 32));
    const auto distances = GetFreeLandDistances(map, mountainLevel);
    const MapPoint center(16, 16);
    RandomUtility rnd(0);

    const auto mountain = GrowMountain(distances, rnd, center, 1, 3);

    BOOST_TEST(mountain.size() == 1u);
    BOOST_TEST(mountain.count(center) == 1u);
}

BOOST_AUTO_TEST_CASE(GrowMountain_reaches_requested_size_on_sufficient_land)
{
    const Map map = createLandMap(MapExtent(32, 32));
    const auto distances = GetFreeLandDistances(map, mountainLevel);
    RandomUtility rnd(0);

    const auto mountain = GrowMountain(distances, rnd, MapPoint(16, 16), 50, 3);

    BOOST_TEST(mountain.size() == 50u);
}

BOOST_AUTO_TEST_CASE(GrowMountain_keeps_margin_to_obstacles)
{
    const Map map = createLandMap(MapExtent(32, 32));
    const auto distances = GetFreeLandDistances(map, mountainLevel);
    const unsigned margin = 6;
    RandomUtility rnd(0);

    const auto mountain = GrowMountain(distances, rnd, MapPoint(16, 16), 1000, margin);

    BOOST_TEST_REQUIRE(mountain.size() > 1u);
    for(const MapPoint& pt : mountain)
    {
        BOOST_TEST_REQUIRE(distances[pt] > margin, "At point " << pt);
    }
}

BOOST_AUTO_TEST_CASE(FindMountains_does_not_change_map)
{
    const Map map = createLandMap(MapExtent(32, 32));
    const auto heightsBefore = map.z;
    RandomUtility rnd(0);

    const auto mountains = FindMountains(map, rnd, mountainLevel, settings);

    BOOST_TEST_REQUIRE(!mountains.empty());
    RTTR_FOREACH_PT(MapPoint, map.size)
    {
        BOOST_TEST_REQUIRE(map.z[pt] == heightsBefore[pt], "At point " << pt);
    }
}

BOOST_AUTO_TEST_CASE(FindMountains_finds_nothing_without_enough_free_land)
{
    const Map map = createLandMap(MapExtent(32, 32));
    const auto freeLand = maxDistance(GetFreeLandDistances(map, mountainLevel));
    RandomUtility rnd(0);

    const MountainSettings limit{freeLand, settings.maxSize, settings.minSize};

    BOOST_TEST(FindMountains(map, rnd, mountainLevel, limit).empty());
}

BOOST_AUTO_TEST_CASE(FindMountains_finds_nothing_on_water_only)
{
    Map map = createMap(MapExtent(16, 16));
    map.z.Resize(map.size, map.height.minimum);
    RandomUtility rnd(0);

    BOOST_TEST(FindMountains(map, rnd, mountainLevel, settings).empty());
}

BOOST_AUTO_TEST_CASE(FindMountains_covers_map_without_any_obstacles)
{
    Map map = createMap(MapExtent(32, 32));
    map.z.Resize(map.size, landHeight);
    RandomUtility rnd(0);

    BOOST_TEST(!FindMountains(map, rnd, mountainLevel, settings).empty());
}

BOOST_AUTO_TEST_CASE(FindMountains_returns_disjoint_mountains)
{
    const Map map = createLandMap(MapExtent(64, 64));
    RandomUtility rnd(0);

    const auto mountains = FindMountains(map, rnd, mountainLevel, settings);

    BOOST_TEST_REQUIRE(mountains.size() > 1u);
    Mountain visited;
    for(const Mountain& mountain : mountains)
    {
        for(const MapPoint& pt : mountain)
        {
            BOOST_TEST_REQUIRE(visited.insert(pt).second, "At point " << pt);
        }
    }
}

BOOST_AUTO_TEST_CASE(FindMountains_respects_size_of_each_mountain)
{
    const Map map = createLandMap(MapExtent(64, 64));
    RandomUtility rnd(0);

    const auto mountains = FindMountains(map, rnd, mountainLevel, settings);

    BOOST_TEST_REQUIRE(mountains.size() > 1u);
    for(unsigned index = 0; index < mountains.size(); index++)
    {
        BOOST_TEST_REQUIRE(mountains[index].size() <= GetMountainSize(settings, index), "Mountain " << index);
    }
}

BOOST_AUTO_TEST_CASE(FindMountains_keeps_free_land_around_obstacles)
{
    const Map map = createLandMap(MapExtent(64, 64));
    const auto distances = GetFreeLandDistances(map, mountainLevel);
    RandomUtility rnd(0);

    for(const Mountain& mountain : FindMountains(map, rnd, mountainLevel, settings))
    {
        for(const MapPoint& pt : mountain)
        {
            BOOST_TEST_REQUIRE(distances[pt] > settings.minFreeLand, "At point " << pt);
        }
    }
}

BOOST_AUTO_TEST_CASE(RaiseMountain_elevates_center_above_foot_and_keeps_surroundings)
{
    Map map = createLandMap(MapExtent(32, 32));
    const MapPoint center(16, 16);
    const auto points = map.z.GetPointsInRadiusWithCenter(center, 4);
    const Mountain mountain(points.begin(), points.end());
    const MapPoint outside(16, 26);
    RandomUtility rnd(0);

    RaiseMountain(map, rnd, mountain);

    BOOST_TEST(map.z[center] > landHeight + 1);
    BOOST_TEST(map.z[outside] == landHeight);
    for(const MapPoint& pt : mountain)
    {
        BOOST_TEST_REQUIRE(map.z[pt] >= landHeight, "At point " << pt);
    }
}

BOOST_AUTO_TEST_CASE(RaiseMountain_applies_minable_textures_to_all_nodes)
{
    Map map = createLandMap(MapExtent(32, 32));
    const auto points = map.z.GetPointsInRadiusWithCenter(MapPoint(16, 16), 4);
    const Mountain mountain(points.begin(), points.end());
    const auto minable = map.textureMap.FindAll(IsMinableMountain);
    RandomUtility rnd(0);

    RaiseMountain(map, rnd, mountain);

    for(const MapPoint& pt : mountain)
    {
        const auto& textures = map.getTextures()[pt];
        BOOST_TEST_REQUIRE(helpers::contains(minable, textures.rsu), "At point " << pt);
        BOOST_TEST_REQUIRE(helpers::contains(minable, textures.lsd), "At point " << pt);
    }
}

BOOST_AUTO_TEST_CASE(RaiseMountain_does_not_exceed_maximum_height)
{
    Map map = createMap(MapExtent(32, 32));
    map.z.Resize(map.size, map.height.maximum - 1);
    const auto points = map.z.GetPointsInRadiusWithCenter(MapPoint(16, 16), 6);
    RandomUtility rnd(0);

    RaiseMountain(map, rnd, Mountain(points.begin(), points.end()));

    RTTR_FOREACH_PT(MapPoint, map.size)
    {
        BOOST_TEST_REQUIRE(map.z[pt] <= map.height.maximum, "At point " << pt);
    }
}

BOOST_AUTO_TEST_CASE(CreateMountains_leaves_no_free_land_beyond_limit)
{
    Map map = createLandMap(MapExtent(64, 64));
    BOOST_TEST_REQUIRE(maxDistance(GetFreeLandDistances(map, mountainLevel)) > settings.minFreeLand);
    RandomUtility rnd(0);

    const auto mountains = CreateMountains(map, rnd, mountainLevel, settings);

    BOOST_TEST_REQUIRE(!mountains.empty());
    BOOST_TEST(maxDistance(GetFreeLandDistances(map, mountainLevel)) <= settings.minFreeLand);
}

BOOST_AUTO_TEST_CASE(FindMountains_is_deterministic_for_same_seed)
{
    const Map map = createLandMap(MapExtent(48, 48));
    RandomUtility rnd1(42);
    RandomUtility rnd2(42);

    const auto mountains1 = FindMountains(map, rnd1, mountainLevel, settings);
    const auto mountains2 = FindMountains(map, rnd2, mountainLevel, settings);

    BOOST_TEST_REQUIRE(mountains1.size() == mountains2.size());
    for(unsigned index = 0; index < mountains1.size(); index++)
    {
        BOOST_TEST_REQUIRE((mountains1[index] == mountains2[index]), "Mountain " << index);
    }
}

BOOST_AUTO_TEST_SUITE_END()
