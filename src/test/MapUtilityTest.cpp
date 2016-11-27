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

#include "mapGenerator/MapUtility.h"
#include "mapGenerator/Map.h"
#include <boost/test/unit_test.hpp>
#include <vector>

BOOST_AUTO_TEST_SUITE(MapUtilityTest)

/**
 * Tests the MapUtility::ComputeWaterSize method for no water.
 */
BOOST_FIXTURE_TEST_CASE(ComputeWaterSize_NoWater, MapUtility)
{
    Map* map = new Map(64, 64, "map", "author");

    const int water = MapUtility::ComputeWaterSize(map, Vec2(10, 10), 100);
    
    BOOST_REQUIRE_EQUAL(water, 0);
    
    delete map;
}

/**
 * Tests the MapUtility::ComputeWaterSize method for a water map.
 */
BOOST_FIXTURE_TEST_CASE(ComputeWaterSize_Water, MapUtility)
{
    Map* map = new Map(16, 16, "map", "author");
    for (int i = 0; i < 256; i++) map->textureLsd[i] = 0x05;
    for (int i = 0; i < 256; i++) map->textureRsu[i] = 0x05;
    
    const int water = MapUtility::ComputeWaterSize(map, Vec2(10, 10), 300);
    
    BOOST_REQUIRE_EQUAL(water, 256);
    
    delete map;
}

/**
 * Tests the MapUtility::ComputeWaterSize method for a water map with maximum limit.
 */
BOOST_FIXTURE_TEST_CASE(ComputeWaterSize_Limit, MapUtility)
{
    Map* map = new Map(16, 16, "map", "author");
    for (int i = 0; i < 256; i++) map->textureLsd[i] = 0x05;
    for (int i = 0; i < 256; i++) map->textureRsu[i] = 0x05;
    
    const int water = MapUtility::ComputeWaterSize(map, Vec2(10, 10), 100);
    
    BOOST_REQUIRE_EQUAL(water, 100);
    
    delete map;
}

BOOST_AUTO_TEST_SUITE_END()

