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

#include "mapGenerator/AreaDesc.h"
#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_SUITE(MapTest)

/**
 * Tests the AreaDesc::IsInArea method for a point inside of the area independent of 
 * the player positions.
 */
BOOST_FIXTURE_TEST_CASE(Constructor_PointInsideIgnorePlayer, AreaDesc)
{
    AreaDesc* area = new AreaDesc(0.5, 0.5, 0.0, 0.2, 100.0, 0, 0, 10, 10, 0);
    const bool isInArea = area->IsInArea(8, 8, 0.0, 16, 16);
    
    BOOST_REQUIRE(isInArea);
    
    delete area;
}

/**
 * Tests the AreaDesc::IsInArea method for a point outside of the area independent of
 * the player positions.
 */
BOOST_FIXTURE_TEST_CASE(Constructor_PointOutsideIgnorePlayer, AreaDesc)
{
    AreaDesc* area = new AreaDesc(0.5, 0.5, 0.0, 0.2, 100.0, 0, 0, 10, 10, 0);
    const bool isInArea = area->IsInArea(0, 0, 0.0, 16, 16);
    
    BOOST_REQUIRE(!isInArea);
    
    delete area;
}

/**
 * Tests the AreaDesc::IsInArea method for a point outside of the area because it is too
 * close to a player.
 */
BOOST_FIXTURE_TEST_CASE(Constructor_PointTooCloseAtPlayer, AreaDesc)
{
    AreaDesc* area = new AreaDesc(0.5, 0.5, 0.0, 0.2, 100.0, 0, 0, 10, 10, 8);
    const bool isInArea = area->IsInArea(8, 8, 0.1, 16, 16);
    
    BOOST_REQUIRE(!isInArea);
    
    delete area;
}

/**
 * Tests the AreaDesc::IsInArea method for a point inside of the area with a given minimum
 * player distance.
 */
BOOST_FIXTURE_TEST_CASE(Constructor_PointFarFromPlayer, AreaDesc)
{
    AreaDesc* area = new AreaDesc(0.5, 0.5, 0.0, 0.2, 100.0, 0, 0, 10, 10, 1);
    const bool isInArea = area->IsInArea(8, 8, 0.1, 16, 16);
    
    BOOST_REQUIRE(isInArea);
    
    delete area;
}

/**
 * Tests the AreaDesc::IsInArea method for a point inside of the area with a given maximum
 * player distance.
 */
BOOST_FIXTURE_TEST_CASE(Constructor_PointNearPlayer, AreaDesc)
{
    AreaDesc* area = new AreaDesc(0.5, 0.5, 0.0, 0.2, 100.0, 0, 0, 10, 10, 0, 8);
    const bool isInArea = area->IsInArea(8, 8, 0.1, 16, 16);
    
    BOOST_REQUIRE(isInArea);
    
    delete area;
}

BOOST_AUTO_TEST_SUITE_END()

