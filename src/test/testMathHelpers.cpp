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

#include "rttrDefines.h" // IWYU pragma: keep
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

BOOST_AUTO_TEST_SUITE_END()
