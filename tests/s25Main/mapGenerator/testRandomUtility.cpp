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
    BOOST_REQUIRE(result.x < size.x && result.y < size.y);
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
