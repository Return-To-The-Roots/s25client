// Copyright (c) 2016 Settlers Freaks (sf-team at siedler25.org)
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

#include "defines.h" // IWYU pragma: keep
#include "test/WorldFixture.h"
#include "world/TerritoryRegion.h"
#include "test/CreateEmptyWorld.h"
#include <boost/test/unit_test.hpp>
#include <boost/bind.hpp>
#include <vector>
#include <string>

typedef WorldFixture<CreateEmptyWorld, 1, 64, 64> EmptyWorldFixture1P;

BOOST_AUTO_TEST_SUITE(TerritoryRegionTestSuite)

BOOST_FIXTURE_TEST_CASE(IsPointValid, EmptyWorldFixture1P)
{
    std::vector<MapPoint> polygon;

    polygon.push_back(MapPoint(0, 0));
    polygon.push_back(MapPoint(1, 1));
    polygon.push_back(MapPoint(62, 1));
    polygon.push_back(MapPoint(62, 62));
    polygon.push_back(MapPoint(1, 62));
    polygon.push_back(MapPoint(1, 1));
    polygon.push_back(MapPoint(0, 0));
    polygon.push_back(MapPoint(35, 25));
    polygon.push_back(MapPoint(37, 25));
    polygon.push_back(MapPoint(37, 27));
    polygon.push_back(MapPoint(35, 27));
    polygon.push_back(MapPoint(35, 25));
    polygon.push_back(MapPoint(0, 0));

    // Point in hole
    BOOST_REQUIRE_EQUAL(TerritoryRegion::IsPointValid(world, polygon, MapPoint(35, 26)), false);

    // Left of hole
    BOOST_REQUIRE_EQUAL(TerritoryRegion::IsPointValid(world, polygon, MapPoint(34, 26)), true);

    // Right of hole
    BOOST_REQUIRE_EQUAL(TerritoryRegion::IsPointValid(world, polygon, MapPoint(38, 26)), true);

    // Above hole
    BOOST_REQUIRE_EQUAL(TerritoryRegion::IsPointValid(world, polygon, MapPoint(36, 24)), true);

    // Below hole
    BOOST_REQUIRE_EQUAL(TerritoryRegion::IsPointValid(world, polygon, MapPoint(36, 28)), true);
}

BOOST_AUTO_TEST_SUITE_END()
