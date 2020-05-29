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

#include "rttrDefines.h"
#include "mapGenerator/ValueMap.h"
#include <boost/test/unit_test.hpp>

using namespace rttr::mapGenerator;

BOOST_AUTO_TEST_SUITE(ValueTests)

BOOST_AUTO_TEST_CASE(GetDifference_ForRange_ReturnsDifferenceBetweenMaximumAndMinimum)
{
    ValueRange<int> range(4, 20);

    BOOST_REQUIRE(range.GetDifference() == 16);
}

BOOST_AUTO_TEST_CASE(MapValueToIndex_ForMinimumValue_ReturnsZero)
{
    ValueRange<int> range(4, 20);

    BOOST_REQUIRE(MapValueToIndex(4, range, 100) == 0);
}

BOOST_AUTO_TEST_CASE(MapValueToIndex_ForSpecificValue_ReturnsExpectedIndex)
{
    ValueRange<int> range(10, 20);

    BOOST_REQUIRE(MapValueToIndex(15, range, 100) == 50);
}

BOOST_AUTO_TEST_CASE(MapValueToIndex_ForMaximumValue_ReturnsLargestIndex)
{
    ValueRange<int> range(4, 20);

    BOOST_REQUIRE(MapValueToIndex(20, range, 100) == 99);
}

BOOST_AUTO_TEST_CASE(GetRange_ForResizedValueMap_ReturnsRangeOfMapValues)
{
    MapExtent size(16, 8);
    ValueMap<int> values(size, 2);

    values[3] = -1;
    values[7] = 7;

    auto range = values.GetRange();

    BOOST_REQUIRE(range.minimum == -1);
    BOOST_REQUIRE(range.maximum == 7);
}

BOOST_AUTO_TEST_CASE(GetMaximum_ForResizedValueMap_ReturnsMaximumValue)
{
    MapExtent size(16, 8);
    ValueMap<int> values(size, 4);

    values[7] = 5;

    BOOST_REQUIRE(values.GetMaximum() == 5);
}

BOOST_AUTO_TEST_CASE(GetMaximumPoint_ForResizedValueMap_ReturnsPointWithMaximumValue)
{
    MapExtent size(16, 8);
    ValueMap<int> values(size, 4);
    MapPoint maximumPoint(3, 5);

    values[maximumPoint] = 5;

    BOOST_REQUIRE(values.GetMaximumPoint() == maximumPoint);
}

BOOST_AUTO_TEST_SUITE_END()
