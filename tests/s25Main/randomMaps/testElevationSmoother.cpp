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
#include "randomMaps/elevation/Smoother.h"

#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_SUITE(SmootherTest)

BOOST_AUTO_TEST_CASE(SmoothesThePoints)
{
    std::vector<unsigned char> result { 0, 0, 10, 10 };
    
    MapExtent size(2, 2);
    Smoother smoother(size, 2, 2);
    
    smoother.Smooth(result);
    
    BOOST_REQUIRE_GT(result[0], 0);
    BOOST_REQUIRE_GT(result[1], 0);
    BOOST_REQUIRE_LT(result[2], 10);
    BOOST_REQUIRE_LT(result[3], 10);
}

BOOST_AUTO_TEST_CASE(SmootingOnlyWithPointsInDistance)
{
    std::vector<unsigned char> result { 0, 0, 10, 10 };
    
    MapExtent size(2, 2);
    Smoother smoother(size, 2, 0); // zero distance
    
    smoother.Smooth(result);
    
    BOOST_REQUIRE_EQUAL(result[0], 0);
    BOOST_REQUIRE_EQUAL(result[1], 0);
    BOOST_REQUIRE_EQUAL(result[2], 10);
    BOOST_REQUIRE_EQUAL(result[3], 10);
}

BOOST_AUTO_TEST_SUITE_END()
