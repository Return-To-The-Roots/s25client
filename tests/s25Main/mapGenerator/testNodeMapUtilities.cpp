// Copyright (c) 2017 - 2017 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "PointOutput.h"
#include "helpers/containerUtils.h"
#include "mapGenerator/NodeMapUtilities.h"
#include <boost/test/unit_test.hpp>

using namespace rttr::mapGenerator;

BOOST_AUTO_TEST_SUITE(NodeMapUtilitiesTests)

BOOST_AUTO_TEST_CASE(GetDifference_returns_maximum_difference_between_range)
{
    ValueRange<int> range(4, 20);

    BOOST_TEST(range.GetDifference() == 16);
}

BOOST_AUTO_TEST_CASE(MapValueToIndex_returns_zero_for_minimum_value)
{
    ValueRange<int> range(4, 20);

    BOOST_TEST(MapValueToIndex(4, range, 100) == 0u);
}

BOOST_AUTO_TEST_CASE(MapValueToIndex_returns_zero_for_zero_range)
{
    ValueRange<int> range(4, 4);

    BOOST_TEST(MapValueToIndex(4, range, 100) == 0u);
}

BOOST_AUTO_TEST_CASE(MapValueToIndex_returns_expected_index_for_range)
{
    ValueRange<int> range(10, 20);

    BOOST_TEST(MapValueToIndex(15, range, 100) == 50u);
}

BOOST_AUTO_TEST_CASE(MapValueToIndex_returns_largest_index_for_maximum_value)
{
    ValueRange<int> range(4, 20);

    BOOST_TEST(MapValueToIndex(20, range, 100) == 99u);
}

BOOST_AUTO_TEST_CASE(GetRange_returns_range_of_map_values)
{
    MapExtent size(16, 8);
    NodeMapBase<int> values;
    values.Resize(size, 2);
    values[3] = -1;
    values[7] = 7;

    auto range = GetRange(values);

    BOOST_TEST(range.minimum == -1);
    BOOST_TEST(range.maximum == 7);
}

BOOST_AUTO_TEST_CASE(GetMaximumPoint_returns_map_point_for_maximum_value)
{
    MapExtent size(16, 8);
    NodeMapBase<int> values;
    values.Resize(size, 4);
    MapPoint maximumPoint(3, 5);
    values[maximumPoint] = 5;

    BOOST_TEST(GetMaximumPoint(values) == maximumPoint);
}

BOOST_AUTO_TEST_CASE(SelectPoints_returns_all_points_which_fulfill_predicate)
{
    MapExtent size(16, 8);
    auto result = SelectPoints([](const MapPoint& pt) { return pt.x == 0; }, size);
    std::vector<MapPoint> expectedResult;
    for(unsigned y = 0; y < size.y; y++)
    {
        expectedResult.push_back(MapPoint(0, y));
    }
    BOOST_TEST_REQUIRE(expectedResult.size() == result.size());
    for(const MapPoint& expectedPoint : expectedResult)
    {
        BOOST_TEST_REQUIRE(helpers::contains(result, expectedPoint));
    }
}

BOOST_AUTO_TEST_SUITE_END()
