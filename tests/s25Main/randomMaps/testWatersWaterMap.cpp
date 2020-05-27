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

#include "rttrDefines.h" // IWYU pragma: keep
#include "randomMaps/waters/WaterMap.h"

#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_SUITE(WaterMapTest)

BOOST_AUTO_TEST_CASE(WaterIsDistributedAsExpected)
{
    std::vector<unsigned char> heightMap { 13, 222, 17, 18 };

    MapExtent size(2, 2);
    WaterMap water;
    std::vector<bool> result = water.Create(heightMap, size, 17);
    
    BOOST_REQUIRE(result[0] == true);
    BOOST_REQUIRE(result[1] == false);
    BOOST_REQUIRE(result[2] == true);
    BOOST_REQUIRE(result[3] == false);
}

BOOST_AUTO_TEST_SUITE_END()
