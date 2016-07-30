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
#include <boost/assign/std/vector.hpp>
#include <boost/assign/std/set.hpp>
#include <vector>
#include <string>

typedef WorldFixture<CreateEmptyWorld, 1, 64, 64> EmptyWorldFixture1P;

using namespace boost::assign;

struct MapPointComp
{
    bool operator() (const MapPoint& lhs, const MapPoint& rhs) const
    {
        return (lhs.y < rhs.y) || ((lhs.y == rhs.y) && (lhs.x < rhs.x));
    }
};

BOOST_AUTO_TEST_SUITE(TerritoryRegionTestSuite)

BOOST_FIXTURE_TEST_CASE(IsPointValid, EmptyWorldFixture1P)
{
    std::vector<MapPoint> polygon;

    // Separator 1
    polygon += MapPoint(0, 0);

    // Outer polygon
    polygon += MapPoint(10, 10), MapPoint(20, 10), MapPoint(20, 20), MapPoint(10, 20), MapPoint(10, 10);

    // Separator 2
    polygon += MapPoint(0, 0);

    // Hole
    polygon += MapPoint(14, 14), MapPoint(16, 14), MapPoint(16, 16), MapPoint(14, 16), MapPoint(14, 14);

    // Separator 3
    polygon += MapPoint(0, 0);

    // Set of MapPoints that should return true
    std::set<MapPoint,MapPointComp> results;

    // Auto-generated data (from different implementation with tests)
    results += MapPoint(10,10), MapPoint(10,11), MapPoint(10,12), MapPoint(10,13), MapPoint(10,14);
    results += MapPoint(10,15), MapPoint(10,16), MapPoint(10,17), MapPoint(10,18), MapPoint(10,19);
    results += MapPoint(11,10), MapPoint(11,11), MapPoint(11,12), MapPoint(11,13), MapPoint(11,14);
    results += MapPoint(11,15), MapPoint(11,16), MapPoint(11,17), MapPoint(11,18), MapPoint(11,19);
    results += MapPoint(12,10), MapPoint(12,11), MapPoint(12,12), MapPoint(12,13), MapPoint(12,14);
    results += MapPoint(12,15), MapPoint(12,16), MapPoint(12,17), MapPoint(12,18), MapPoint(12,19);
    results += MapPoint(13,10), MapPoint(13,11), MapPoint(13,12), MapPoint(13,13), MapPoint(13,14);
    results += MapPoint(13,15), MapPoint(13,16), MapPoint(13,17), MapPoint(13,18), MapPoint(13,19);
    results += MapPoint(14,10), MapPoint(14,11), MapPoint(14,12), MapPoint(14,13), MapPoint(14,16);
    results += MapPoint(14,17), MapPoint(14,18), MapPoint(14,19), MapPoint(15,10), MapPoint(15,11);
    results += MapPoint(15,12), MapPoint(15,13), MapPoint(15,16), MapPoint(15,17), MapPoint(15,18);
    results += MapPoint(15,19), MapPoint(16,10), MapPoint(16,11), MapPoint(16,12), MapPoint(16,13);
    results += MapPoint(16,14), MapPoint(16,15), MapPoint(16,16), MapPoint(16,17), MapPoint(16,18);
    results += MapPoint(16,19), MapPoint(17,10), MapPoint(17,11), MapPoint(17,12), MapPoint(17,13);
    results += MapPoint(17,14), MapPoint(17,15), MapPoint(17,16), MapPoint(17,17), MapPoint(17,18);
    results += MapPoint(17,19), MapPoint(18,10), MapPoint(18,11), MapPoint(18,12), MapPoint(18,13);
    results += MapPoint(18,14), MapPoint(18,15), MapPoint(18,16), MapPoint(18,17), MapPoint(18,18);
    results += MapPoint(18,19), MapPoint(19,10), MapPoint(19,11), MapPoint(19,12), MapPoint(19,13);
    results += MapPoint(19,14), MapPoint(19,15), MapPoint(19,16), MapPoint(19,17), MapPoint(19,18);
    results += MapPoint(19,19);

    // check the whole area
    for (int x = 0; x < 30; ++x)
    {
        for (int y = 0; y < 30; ++y)
        {
            BOOST_REQUIRE_EQUAL(TerritoryRegion::IsPointValid(world, polygon, MapPoint(x, y)), results.find(MapPoint(x, y)) != results.end());
        }
    }
}

BOOST_AUTO_TEST_SUITE_END()
