// Copyright (c) 2016 - 2017 Settlers Freaks (sf-team at siedler25.org)
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

#include "commonDefines.h" // IWYU pragma: keep
#include "helpers/SmoothedValue.hpp"
#include "helpers/mathFuncs.h"
#include "helpers/roundToNextPow2.h"
#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_SUITE(MathHelperTests)

BOOST_AUTO_TEST_CASE(NextPow2)
{
    BOOST_REQUIRE_EQUAL(helpers::roundToNextPowerOfTwo(0), 1u);
    BOOST_REQUIRE_EQUAL(helpers::roundToNextPowerOfTwo(1), 1u);
    BOOST_REQUIRE_EQUAL(helpers::roundToNextPowerOfTwo(2), 2u);
    BOOST_REQUIRE_EQUAL(helpers::roundToNextPowerOfTwo(3), 4u);
    BOOST_REQUIRE_EQUAL(helpers::roundToNextPowerOfTwo(4), 4u);
    BOOST_REQUIRE_EQUAL(helpers::roundToNextPowerOfTwo(5), 8u);
    // Start with 2^3=8 to 2^30
    for(unsigned shift = 3; shift < 30; shift++)
    {
        unsigned val = 1u << shift; // 2^shift
        // Pow2 stays unchanged
        BOOST_REQUIRE_EQUAL(helpers::roundToNextPowerOfTwo(val), val);
        // -1 -> val
        BOOST_REQUIRE_EQUAL(helpers::roundToNextPowerOfTwo(val - 1u), val);
        // +1 -> val * 2
        BOOST_REQUIRE_EQUAL(helpers::roundToNextPowerOfTwo(val + 1u), val * 2);
    }
    BOOST_REQUIRE_EQUAL(helpers::roundToNextPowerOfTwo(1u << 31), 1u << 31);
    // TODO: Max unsigned is 2^32 - 1. What to do with values greater than 2^31?
}

BOOST_AUTO_TEST_CASE(clamp)
{
    // Simple
    BOOST_REQUIRE_EQUAL(helpers::clamp(5, 1, 10), 5);
    BOOST_REQUIRE_EQUAL(helpers::clamp(-5, 1, 10), 1);
    BOOST_REQUIRE_EQUAL(helpers::clamp(0, 1, 10), 1);
    BOOST_REQUIRE_EQUAL(helpers::clamp(1, 1, 10), 1);
    BOOST_REQUIRE_EQUAL(helpers::clamp(10, 1, 10), 10);
    BOOST_REQUIRE_EQUAL(helpers::clamp(11, 1, 10), 10);
    // Signed value, unsigned min/max
    BOOST_REQUIRE_EQUAL(helpers::clamp(5, 1u, 10u), 5u);
    BOOST_REQUIRE_EQUAL(helpers::clamp(-5, 1u, 10u), 1u);
    BOOST_REQUIRE_EQUAL(helpers::clamp(std::numeric_limits<int>::min(), 1u, 10u), 1u);
    BOOST_REQUIRE_EQUAL(helpers::clamp(std::numeric_limits<int>::max(), 1u, 10u), 10u);
    // unsigned value, signed min/max
    BOOST_REQUIRE_EQUAL(helpers::clamp(5u, -5, 6), 5);
    BOOST_REQUIRE_EQUAL(helpers::clamp(10u, -5, 6), 6);
    BOOST_REQUIRE_EQUAL(helpers::clamp(std::numeric_limits<unsigned>::min(), -5, 6), 0);
    BOOST_REQUIRE_EQUAL(helpers::clamp(std::numeric_limits<unsigned>::max(), -5, 6), 6);
    BOOST_REQUIRE_EQUAL(helpers::clamp(std::numeric_limits<unsigned>::min(), -15, -2), -2);
    BOOST_REQUIRE_EQUAL(helpers::clamp(std::numeric_limits<unsigned>::max(), -15, -2), -2);
    BOOST_REQUIRE_EQUAL(helpers::clamp(std::numeric_limits<unsigned>::min(), std::numeric_limits<int>::min(), 10), 0);
    BOOST_REQUIRE_EQUAL(helpers::clamp(std::numeric_limits<unsigned>::max(), std::numeric_limits<int>::min(), 10), 10);
    BOOST_REQUIRE_EQUAL(
      helpers::clamp(std::numeric_limits<unsigned>::min(), std::numeric_limits<int>::min(), std::numeric_limits<int>::max()), 0);
    BOOST_REQUIRE_EQUAL(
      helpers::clamp(std::numeric_limits<unsigned>::max(), std::numeric_limits<int>::min(), std::numeric_limits<int>::max()),
      std::numeric_limits<int>::max());
    // short value, long min/max
    BOOST_REQUIRE_EQUAL(helpers::clamp(static_cast<short>(-3), -5, 6), -3);
    BOOST_REQUIRE_EQUAL(helpers::clamp(static_cast<short>(5), -5, 6), 5);
    BOOST_REQUIRE_EQUAL(helpers::clamp(std::numeric_limits<short>::min(), -5, 6), -5);
    BOOST_REQUIRE_EQUAL(helpers::clamp(std::numeric_limits<short>::max(), -5, 6), 6);
    // long value, short min/max
    BOOST_REQUIRE_EQUAL(helpers::clamp(-1, static_cast<short>(-3), static_cast<short>(6)), -1);
    BOOST_REQUIRE_EQUAL(helpers::clamp(4, static_cast<short>(-3), static_cast<short>(6)), 4);
    BOOST_REQUIRE_EQUAL(helpers::clamp(std::numeric_limits<int>::min(), static_cast<short>(-3), static_cast<short>(6)), -3);
    BOOST_REQUIRE_EQUAL(helpers::clamp(std::numeric_limits<int>::max(), static_cast<short>(-3), static_cast<short>(6)), 6);
}

BOOST_AUTO_TEST_CASE(SmoothedValue)
{
    helpers::SmoothedValue<int> val(5);
    BOOST_REQUIRE_EQUAL(val.size(), 0u);
    BOOST_REQUIRE_EQUAL(val.get(), 0);
    val.add(4);
    BOOST_REQUIRE_EQUAL(val.size(), 1u);
    BOOST_REQUIRE_EQUAL(val.get(), 4);
    val.add(2);
    BOOST_REQUIRE_EQUAL(val.size(), 2u);
    BOOST_REQUIRE_EQUAL(val.get(), (4 + 2) / 2);
    val.add(11);
    BOOST_REQUIRE_EQUAL(val.size(), 3u);
    BOOST_REQUIRE_EQUAL(val.get(), (4 + 2 + 11) / 3);
    val.add(102);
    BOOST_REQUIRE_EQUAL(val.size(), 4u);
    BOOST_REQUIRE_EQUAL(val.get(), (4 + 2 + 11 + 102) / 4);
    val.add(1);
    BOOST_REQUIRE_EQUAL(val.size(), 5u);
    BOOST_REQUIRE_EQUAL(val.get(), (4 + 2 + 11 + 102 + 1) / 5);
    val.add(7);
    BOOST_REQUIRE_EQUAL(val.size(), 5u);
    BOOST_REQUIRE_EQUAL(val.get(), (2 + 11 + 102 + 1 + 7) / 5);
}

BOOST_AUTO_TEST_SUITE_END()
