// Copyright (C) 2017 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "PointOutput.h"
#include "mapGenFixtures.h"
#include "mapGenerator/Islands.h"
#include <boost/test/unit_test.hpp>

using namespace rttr::mapGenerator;

BOOST_FIXTURE_TEST_SUITE(IslandsTests, MapGenFixture)

BOOST_AUTO_TEST_CASE(CreateIsland_keeps_minimum_distance_to_land)
{
    RandomUtility rnd(0);
    Map map = createMap(MapExtent(26, 34));
    MapPoint land(0, 0);
    map.z.Resize(map.size, map.height.minimum); // all sea
    map.z[land] = map.height.minimum + 1;

    const double mountainCoverage = 0.1;
    const unsigned minLandDist = 5;
    const unsigned size = 800;
    const auto island = CreateIsland(map, rnd, size, minLandDist, mountainCoverage);

    for(MapPoint p : island)
    {
        BOOST_TEST(map.z.CalcDistance(p, land) >= minLandDist);
    }
}

BOOST_AUTO_TEST_SUITE_END()
