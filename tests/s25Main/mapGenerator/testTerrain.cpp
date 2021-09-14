// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "mapGenFixtures.h"
#include "mapGenerator/Terrain.h"
#include "mapGenerator/TextureHelper.h"
#include <boost/test/unit_test.hpp>

using namespace rttr::mapGenerator;

BOOST_FIXTURE_TEST_SUITE(TerrainTests, MapGenFixture)

BOOST_AUTO_TEST_CASE(Restructure_keeps_minimum_and_maximum_values_unchanged)
{
    Map map = createMap(MapExtent(6, 8));

    map.z.Resize(map.size, 5); // default height
    auto predicate = [](const MapPoint& pt) noexcept { return pt.x == 3 && pt.y == 4; };

    Restructure(map, predicate);

    RTTR_FOREACH_PT(MapPoint, map.size)
    {
        BOOST_TEST_REQUIRE(map.z[pt] <= map.height.maximum);
        BOOST_TEST_REQUIRE(map.z[pt] >= map.height.minimum);
    }
}

BOOST_AUTO_TEST_CASE(Restructure_increases_height_of_focus_area)
{
    Map map = createMap(MapExtent(6, 8));

    MapPoint focus(3, 4);
    auto predicate = [&focus](const MapPoint& pt) noexcept { return pt == focus; };
    const uint8_t heightBefore = 5;
    map.z.Resize(map.size, heightBefore);

    Restructure(map, predicate);

    const uint8_t heightAfter = map.z[focus];

    BOOST_TEST(heightAfter > heightBefore);
}

BOOST_AUTO_TEST_CASE(Restructure_increases_height_less_when_further_away_from_focus)
{
    Map map = createMap(MapExtent(6, 8));

    MapPoint focus(3, 4);
    auto predicate = [&focus](const MapPoint& pt) noexcept { return pt == focus; };
    MapPoint nonFocus(0, 3);

    const uint8_t heightBefore = 5;
    map.z.Resize(map.size, heightBefore);

    Restructure(map, predicate);

    const int diffFocus = map.z[focus] - heightBefore;
    const int diffNonFocus = map.z[nonFocus] - heightBefore;

    BOOST_TEST(diffFocus > diffNonFocus);
}

BOOST_AUTO_TEST_SUITE_END()
