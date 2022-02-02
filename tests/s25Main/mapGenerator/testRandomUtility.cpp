// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "mapGenerator/RandomUtility.h"
#include <boost/test/unit_test.hpp>
#include <set>

using namespace rttr::mapGenerator;

BOOST_AUTO_TEST_SUITE(RandomUtilityTests)

BOOST_AUTO_TEST_CASE(Index_returns_values_gte_zero_and_lt_size)
{
    RandomUtility rnd(0u);

    for(auto size = 1u; size < 10u; size++)
    {
        auto result = rnd.Index(size);

        BOOST_TEST_REQUIRE(result < size);
    }
}

BOOST_AUTO_TEST_CASE(Point_returns_points_within_map_size)
{
    MapExtent size(12, 8);
    RandomUtility rnd(0u);
    MapPoint result = rnd.Point(size);
    BOOST_TEST(result.x < size.x);
    BOOST_TEST(result.y < size.y);
}

BOOST_AUTO_TEST_CASE(RandomInt_returns_value_within_thresholds)
{
    RandomUtility rnd(0u);

    int minimum = -10;
    int maximum = 7;

    auto result = rnd.RandomValue(minimum, maximum);

    BOOST_TEST(result >= minimum);
    BOOST_TEST(result <= maximum);
}

BOOST_AUTO_TEST_CASE(RandomDouble_returns_value_within_thresholds)
{
    RandomUtility rnd(0u);

    double minimum = -12.123;
    double maximum = 7.456;

    auto result = rnd.RandomDouble(minimum, maximum);

    BOOST_TEST(result >= minimum);
    BOOST_TEST(result <= maximum);
}

BOOST_AUTO_TEST_SUITE_END()
