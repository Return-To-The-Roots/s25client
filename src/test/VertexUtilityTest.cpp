// Copyright (c) 2005 - 2015 Settlers Freaks (sf-team at siedler25.org)
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

#include "mapGenerator/VertexUtility.h"
#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_SUITE(VertexUtilityTest)

/**
 * Tests the VertexUtility::GetPosition method for an index which is outside
 * of the area defined by the width and height. The method should still return
 * a valid position.
 */
BOOST_FIXTURE_TEST_CASE(GetPosition_IndexOutOfBounds, VertexUtility)
{
    Vec2 v = VertexUtility::GetPosition(10, 2, 2);
        
    BOOST_REQUIRE_EQUAL(v.x, 0);
    BOOST_REQUIRE_EQUAL(v.y, 5);
}

/**
 * Tests the VertexUtility::GetPosition method for an index which is expected to be
 * the bottom, right corner position according to the width and height.
 */
BOOST_FIXTURE_TEST_CASE(GetPosition_CornerIndex, VertexUtility)
{
    Vec2 v = VertexUtility::GetPosition(8, 3, 3);
        
    BOOST_REQUIRE_EQUAL(v.x, 2);
    BOOST_REQUIRE_EQUAL(v.y, 2);
}
    
/**
 * Tests the VertexUtility::GetPosition method for a zero-index. Then the method should
 * always return 0 independ of width and height.
 */
BOOST_FIXTURE_TEST_CASE(GetPosition_ZeroIndex, VertexUtility)
{
    for (int width = 1; width < 100; width++)
    {
        for (int height = 1; height < 100; height++)
        {
            Vec2 v = VertexUtility::GetPosition(0, width, height);
            BOOST_REQUIRE_EQUAL(v.x, 0);
            BOOST_REQUIRE_EQUAL(v.y, 0);
        }
    }
}

/**
 * Tests the VertexUtility::GetIndexOf method for a negative x-coordinate for the position.
 * The resulting index should be pointing to a valid entry inside of the bounds given by width
 * and height.
 */
BOOST_FIXTURE_TEST_CASE(GetIndexOf_NegativePositionX, VertexUtility)
{
    const int width = 4;
    const int height = 5;
    const int index = VertexUtility::GetIndexOf(-2 * width, 0, width, height);
    
    BOOST_REQUIRE(index >= 0 && index < width * height);
}

/**
 * Tests the VertexUtility::GetIndexOf method for a negative y-coordinate for the position.
 * The resulting index should be pointing to a valid entry inside of the bounds given by width
 * and height.
 */
BOOST_FIXTURE_TEST_CASE(GetIndexOf_NegativePositionY, VertexUtility)
{
    const int width = 4;
    const int height = 5;
    const int index = VertexUtility::GetIndexOf(0, -2 * height, width, height);
    
    BOOST_REQUIRE(index >= 0 && index < width * height);
}

/**
 * Tests the VertexUtility::GetIndexOf method for a position outside of the bounds given by 
 * width and height.
 */
BOOST_FIXTURE_TEST_CASE(GetIndexOf_OutsideOfBounds, VertexUtility)
{
    const int width = 4;
    const int height = 5;
    const int index = VertexUtility::GetIndexOf(2 * width, 2 * height, width, height);
    
    BOOST_REQUIRE(index >= 0 && index < width * height);
}

/**
 * Tests the VertexUtility::GetIndexOf method for a position inside of the bounds given by
 * width and height. The resulting index must match the scheme: index = y * width + x.
 */
BOOST_FIXTURE_TEST_CASE(GetIndexOf_InsideOfBounds, VertexUtility)
{
    const int x = 2;
    const int y = 3;
    const int width = 2 * x;
    const int height = 2 * y;
    const int index = VertexUtility::GetIndexOf(x, y, width, height);
    
    BOOST_REQUIRE_EQUAL(index, y * width + x);
}

BOOST_AUTO_TEST_SUITE_END()

/*
 TODO

 std::vector<int> VertexUtility::GetNeighbors(const int x,
                                             const int y,
                                             const int width,
                                             const int height,
                                             const int radius);

 double VertexUtility::Distance(const int x1,
                               const int y1,
                               const int x2,
                               const int y2,
                               const int width,
                               const int height)
*/


