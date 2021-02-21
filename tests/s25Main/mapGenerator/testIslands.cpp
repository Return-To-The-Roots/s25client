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
