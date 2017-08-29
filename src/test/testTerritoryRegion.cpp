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
#include "world/TerritoryRegion.h"
#include "test/CreateEmptyWorld.h"
#include "test/WorldFixture.h"
#include <boost/assign/std/set.hpp>
#include <boost/assign/std/vector.hpp>
#include <boost/bind.hpp>
#include <boost/foreach.hpp>
#include <boost/range/algorithm_ext/push_back.hpp>
#include <boost/test/unit_test.hpp>
#include <iostream>
#include <string>
#include <vector>

typedef WorldFixture<CreateEmptyWorld, 0, 32, 32> EmptyWorldFixture0P;

using namespace boost::assign;

struct MapPointComp
{
    bool operator()(const MapPoint& lhs, const MapPoint& rhs) const { return (lhs.y < rhs.y) || ((lhs.y == rhs.y) && (lhs.x < rhs.x)); }
};

BOOST_AUTO_TEST_SUITE(TerritoryRegionTestSuite)

BOOST_FIXTURE_TEST_CASE(IsPointValid, EmptyWorldFixture0P)
{
    std::vector<MapPoint> hole, hole_reversed;
    std::vector<MapPoint> outer, outer_reversed;

    // Hole
    hole += MapPoint(14, 14), MapPoint(16, 14), MapPoint(16, 16), MapPoint(14, 16), MapPoint(14, 14);

    // Reverse it...
    hole_reversed = hole;
    std::reverse(hole_reversed.begin(), hole_reversed.end());

    // Outer polygon
    outer += MapPoint(10, 10), MapPoint(20, 10), MapPoint(20, 20), MapPoint(10, 20), MapPoint(10, 10);

    // Reverse it...
    outer_reversed = outer;
    std::reverse(outer_reversed.begin(), outer_reversed.end());

    // Set of MapPoints that should return true
    std::set<MapPoint, MapPointComp> results;

    // Auto-generated data (from different implementation with tests)
    results += MapPoint(10, 10), MapPoint(10, 11), MapPoint(10, 12), MapPoint(10, 13), MapPoint(10, 14);
    results += MapPoint(10, 15), MapPoint(10, 16), MapPoint(10, 17), MapPoint(10, 18), MapPoint(10, 19);
    results += MapPoint(11, 10), MapPoint(11, 11), MapPoint(11, 12), MapPoint(11, 13), MapPoint(11, 14);
    results += MapPoint(11, 15), MapPoint(11, 16), MapPoint(11, 17), MapPoint(11, 18), MapPoint(11, 19);
    results += MapPoint(12, 10), MapPoint(12, 11), MapPoint(12, 12), MapPoint(12, 13), MapPoint(12, 14);
    results += MapPoint(12, 15), MapPoint(12, 16), MapPoint(12, 17), MapPoint(12, 18), MapPoint(12, 19);
    results += MapPoint(13, 10), MapPoint(13, 11), MapPoint(13, 12), MapPoint(13, 13), MapPoint(13, 14);
    results += MapPoint(13, 15), MapPoint(13, 16), MapPoint(13, 17), MapPoint(13, 18), MapPoint(13, 19);
    results += MapPoint(14, 10), MapPoint(14, 11), MapPoint(14, 12), MapPoint(14, 13), MapPoint(14, 16);
    results += MapPoint(14, 17), MapPoint(14, 18), MapPoint(14, 19), MapPoint(15, 10), MapPoint(15, 11);
    results += MapPoint(15, 12), MapPoint(15, 13), MapPoint(15, 16), MapPoint(15, 17), MapPoint(15, 18);
    results += MapPoint(15, 19), MapPoint(16, 10), MapPoint(16, 11), MapPoint(16, 12), MapPoint(16, 13);
    results += MapPoint(16, 14), MapPoint(16, 15), MapPoint(16, 16), MapPoint(16, 17), MapPoint(16, 18);
    results += MapPoint(16, 19), MapPoint(17, 10), MapPoint(17, 11), MapPoint(17, 12), MapPoint(17, 13);
    results += MapPoint(17, 14), MapPoint(17, 15), MapPoint(17, 16), MapPoint(17, 17), MapPoint(17, 18);
    results += MapPoint(17, 19), MapPoint(18, 10), MapPoint(18, 11), MapPoint(18, 12), MapPoint(18, 13);
    results += MapPoint(18, 14), MapPoint(18, 15), MapPoint(18, 16), MapPoint(18, 17), MapPoint(18, 18);
    results += MapPoint(18, 19), MapPoint(19, 10), MapPoint(19, 11), MapPoint(19, 12), MapPoint(19, 13);
    results += MapPoint(19, 14), MapPoint(19, 15), MapPoint(19, 16), MapPoint(19, 17), MapPoint(19, 18);
    results += MapPoint(19, 19);

    // check the whole area
    std::vector<MapPoint> polygon[8];

    // Generate polygons for all eight cases of ordering
    for(int i = 0; i < 8; ++i)
    {
        // i = 1, 3, 5, 7 -> hole, then outer
        // i = 2, 3, 6, 7 -> hole reversed
        // i = 4, 5, 6, 7 -> outer reversed

        polygon[i] += MapPoint(0, 0);
        polygon[i] = boost::push_back(polygon[i],
                                      (i & (1 << 0)) ? ((i & (1 << 1)) ? hole : hole_reversed) : ((i & (1 << 2)) ? outer : outer_reversed));
        polygon[i] += MapPoint(0, 0);
        polygon[i] = boost::push_back(polygon[i],
                                      (i & (1 << 0)) ? ((i & (1 << 2)) ? outer : outer_reversed) : ((i & (1 << 1)) ? hole : hole_reversed));
        polygon[i] += MapPoint(0, 0);
    }

    for(int i = 0; i < 8; ++i)
    {
        // check the whole area
        for(int x = 0; x < 30; ++x)
        {
            for(int y = 0; y < 30; ++y)
            {
                // Result for this particular point
                bool result = results.find(MapPoint(x, y)) != results.end();
                BOOST_REQUIRE_EQUAL(TerritoryRegion::IsPointValid(world, polygon[i], MapPoint(x, y)), result);
            }
        }
    }

    // Consistency checks

    // Make a rectangle (10,5)->(15,10) and 2 parallelograms below
    boost::array<std::vector<MapPoint>, 4> rectAreas;
    rectAreas[0] += MapPoint(10, 5), MapPoint(15, 5), MapPoint(15, 10);
    rectAreas[0] += MapPoint(18, 13), MapPoint(15, 15);
    rectAreas[0] += MapPoint(10, 15);
    rectAreas[0] += MapPoint(13, 13), MapPoint(10, 10);
    // With duplicate start point
    rectAreas[1].clear();
    boost::push_back(rectAreas[1], rectAreas[0]) += rectAreas[0][0];
    rectAreas[2].resize(rectAreas[0].size());
    std::reverse_copy(rectAreas[0].begin(), rectAreas[0].end(), rectAreas[2].begin());
    rectAreas[3].resize(rectAreas[1].size());
    std::reverse_copy(rectAreas[1].begin(), rectAreas[1].end(), rectAreas[3].begin());

    results.clear();
    // All points inside must be inside
    for(unsigned i = 0; i < rectAreas.size(); i++)
    {
        for(int x = 11; x < 15; ++x)
        {
            for(int y = 6; y < 15; ++y)
            {
                MapPoint pt(x, y);
                if(y > 10 && y < 13)
                    pt.x += y - 10;
                else if(y >= 13)
                    pt.x += 15 - y;
                BOOST_REQUIRE(TerritoryRegion::IsPointValid(world, rectAreas[i], pt));
                if(i == 0)
                    results.insert(pt);
            }
        }
    }
    // Get the points exactly at the border and just outside of it
    std::vector<MapPoint> borderPts;
    std::vector<MapPoint> outsidePts;
    outsidePts += MapPoint(9, 4), MapPoint(16, 4), MapPoint(16, 16), MapPoint(9, 16);
    outsidePts += MapPoint(10, 12), MapPoint(10, 13), MapPoint(11, 13), MapPoint(10, 14);
    for(int x = 10; x <= 15; x++)
    {
        borderPts += MapPoint(x, 5), MapPoint(x, 15);
        outsidePts += MapPoint(x, 5 - 1), MapPoint(x, 15 + 1);
    }
    for(int y = 5; y <= 10; y++)
    {
        borderPts += MapPoint(10, y), MapPoint(15, y);
        outsidePts += MapPoint(10 - 1, y), MapPoint(15 + 1, y);
    }
    // Border at the parallelogram (not really all border, but hard to figure out which are outside
    for(int y = 11; y < 15; y++)
    {
        for(int x = 0; x <= 3; x++)
            borderPts += MapPoint(x + 11, y), MapPoint(x + 16, y);
    }
    for(unsigned i = 0; i < rectAreas.size(); i++)
    {
        // Those must be outside
        BOOST_FOREACH(MapPoint pt, outsidePts)
        {
            BOOST_REQUIRE(!TerritoryRegion::IsPointValid(world, rectAreas[i], pt));
        }
    }
    // Border points are unspecified, but must be consistently either inside or outside
    BOOST_FOREACH(MapPoint pt, borderPts)
    {
        const bool isValid = TerritoryRegion::IsPointValid(world, rectAreas[0], pt);
        if(isValid)
            results.insert(pt);
        for(unsigned i = 1; i < rectAreas.size(); i++)
            BOOST_REQUIRE_EQUAL(TerritoryRegion::IsPointValid(world, rectAreas[i], pt), isValid);
    }

    std::vector<MapPoint> fullMapArea;
    fullMapArea += MapPoint(0, 0), MapPoint(0, world.GetHeight() - 1), MapPoint(world.GetWidth() - 1, world.GetHeight() - 1),
      MapPoint(world.GetWidth() - 1, 0), MapPoint(0, 0);
    std::vector<MapPoint> fullMapAreaReversed(fullMapArea.size());
    std::reverse_copy(fullMapArea.begin(), fullMapArea.end(), fullMapAreaReversed.begin());

    for(unsigned i = 0; i < 4; i++)
    {
        std::vector<MapPoint> fullArea;
        fullArea += MapPoint(0, 0);
        if(i < 2)
            boost::push_back(fullArea, fullMapArea);
        else
            boost::push_back(fullArea, fullMapAreaReversed);
        fullArea += MapPoint(0, 0);
        if(i / 2 < 2)
            boost::push_back(fullArea, rectAreas[1]);
        else
            boost::push_back(fullArea, rectAreas[3]);
        fullArea += MapPoint(0, 0);

        // check the whole area
        for(int x = 1; x < 30; ++x)
        {
            for(int y = 1; y < 30; ++y)
            {
                // If the point is in the set, it is in the small rect and should not be in the big rect with this as a hole
                const bool result = results.find(MapPoint(x, y)) == results.end();
                BOOST_REQUIRE_EQUAL(TerritoryRegion::IsPointValid(world, fullArea, MapPoint(x, y)), result);
            }
        }
    }
}

BOOST_AUTO_TEST_SUITE_END()
