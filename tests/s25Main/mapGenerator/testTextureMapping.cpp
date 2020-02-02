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

#include "mapGenerator/TextureMapping.h"
#include <boost/test/unit_test.hpp>
#include <iostream>

using namespace rttr::mapGenerator;

BOOST_AUTO_TEST_SUITE(TextureMappingTests)

BOOST_AUTO_TEST_CASE(MapRangeToIndex_ForMinimumValue_ReturnsZero)
{
    size_t size = 20;

    BOOST_REQUIRE(MapRangeToIndex(0, 0, 10, size) == 0);
    BOOST_REQUIRE(MapRangeToIndex(4, 4, 10, size) == 0);
    BOOST_REQUIRE(MapRangeToIndex(7, 7, 82, size) == 0);
}

BOOST_AUTO_TEST_CASE(MapRangeToIndex_ForMaximumValue_ReturnsLastIndex)
{
    size_t size = 23;
    int lastIndex = size - 1;
    
    BOOST_REQUIRE(MapRangeToIndex(10, 0, 10, size) == lastIndex);
    BOOST_REQUIRE(MapRangeToIndex(21, 4, 21, size) == lastIndex);
    BOOST_REQUIRE(MapRangeToIndex(99, 7, 99, size) == lastIndex);
}

BOOST_AUTO_TEST_CASE(MapRangeToIndex_ForValuesOfShiftedRange_ReturnsExpectedIndex)
{
    size_t size = 24;
    int lastIndex = size - 1;
    
    for (int i = lastIndex; i <= 46; ++i)
    {
        BOOST_REQUIRE(MapRangeToIndex(i, lastIndex, 46, size) == i - lastIndex);
    }
}

BOOST_AUTO_TEST_CASE(MapRangeToIndex_ForMiddleValue_ReturnsExpectedIndex)
{
    size_t size = 50;

    const int minimum = 100;
    const int maximum = 200;
    
    const int middleValue = 150;
    
    BOOST_REQUIRE(MapRangeToIndex(middleValue, minimum, maximum, size) == 25);
}

BOOST_AUTO_TEST_SUITE_END()
