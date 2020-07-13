// Copyright (c) 2017 - 2020 Settlers Freaks (sf-team at siedler25.org)
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

#include "RttrForeachPt.h"
#include "mapGenerator/VertexUtility.h"
#include <boost/test/unit_test.hpp>
#include <vector>

BOOST_AUTO_TEST_SUITE(VertexUtilityTest)

/**
 * Tests the VertexUtility::GetPosition method for an index which is outside
 * of the area defined by the width and height. The method should still return
 * a valid position.
 */
BOOST_FIXTURE_TEST_CASE(GetPosition_IndexOutOfBounds, VertexUtility)
{
    Position v = VertexUtility::GetPosition(10, MapExtent(2, 2));

    BOOST_REQUIRE_EQUAL(v.x, 0);
    BOOST_REQUIRE_EQUAL(v.y, 5);
}

/**
 * Tests the VertexUtility::GetPosition method for an index which is expected to be
 * the bottom, right corner position according to the width and height.
 */
BOOST_FIXTURE_TEST_CASE(GetPosition_CornerIndex, VertexUtility)
{
    Position v = VertexUtility::GetPosition(8, MapExtent(3, 3));

    BOOST_REQUIRE_EQUAL(v.x, 2);
    BOOST_REQUIRE_EQUAL(v.y, 2);
}

/**
 * Tests the VertexUtility::GetPosition method for a zero-index. Then the method should
 * always return 0 independent of width and height.
 */
BOOST_FIXTURE_TEST_CASE(GetPosition_ZeroIndex, VertexUtility)
{
    for(int width = 1; width < 100; width++)
    {
        for(int height = 1; height < 100; height++)
        {
            Position v = VertexUtility::GetPosition(0, MapExtent(width, height));
            BOOST_REQUIRE_EQUAL(v.x, 0);
            BOOST_REQUIRE_EQUAL(v.y, 0);
        }
    }
}

/**
 * Tests the VertexUtility::GetIndexOf method for a negative y-coordinate for the position.
 * The resulting index should be pointing to a valid entry inside of the bounds given by width
 * and height.
 */
BOOST_FIXTURE_TEST_CASE(GetIndexOf_NegativePosition, VertexUtility)
{
    const MapExtent size(64, 60);

    RTTR_FOREACH_PT(Position, size)
    {
        BOOST_REQUIRE_EQUAL(VertexUtility::GetIndexOf(pt - size, size), pt.y * size.x + pt.x);
    }
}

/**
 * Tests the VertexUtility::GetIndexOf method for a position inside of the bounds given by
 * width and height. The resulting index must match the scheme: index = y * width + x.
 */
BOOST_FIXTURE_TEST_CASE(GetIndexOf_InsideOfBounds, VertexUtility)
{
    const MapExtent size(64, 60);

    RTTR_FOREACH_PT(Position, size)
    {
        BOOST_REQUIRE_EQUAL(VertexUtility::GetIndexOf(pt, size), pt.y * size.x + pt.x);
    }
}

/**
 * Tests the VertexUtility::GetIndexOf method for a position outside of the bounds given by
 * width and height.
 */
BOOST_FIXTURE_TEST_CASE(GetIndexOf_OutsideOfBounds, VertexUtility)
{
    const MapExtent size(64, 60);
    RTTR_FOREACH_PT(Position, size)
    {
        BOOST_REQUIRE_EQUAL(VertexUtility::GetIndexOf(pt + size, size), pt.y * size.x + pt.x);
    }
}

/**
 * Tests the VertexUtility::GetNeighbors function for neighbors outside of the bounds given
 * by width and height. The resulting neighbor indices must contain the ones from the other
 * side of the map.
 */
BOOST_FIXTURE_TEST_CASE(GetNeighbors_OutOfBounds, VertexUtility)
{
    std::vector<int> neighbors = VertexUtility::GetNeighbors(Position(0, 0), MapExtent(10, 10), 1);
    const unsigned expectedSize = 5;

    BOOST_REQUIRE_EQUAL(neighbors.size(), expectedSize);
}

/**
 * Tests the VertexUtility::Distance function for neighboring vertices at the map's boundaries.
 */
BOOST_FIXTURE_TEST_CASE(Distance_BoundaryNeighbors, VertexUtility)
{
    const double distance = VertexUtility::Distance(Position(0, 0), Position(0, 9), MapExtent(10, 10));

    BOOST_REQUIRE_LT(distance, 2.0);
}

BOOST_AUTO_TEST_SUITE_END()
