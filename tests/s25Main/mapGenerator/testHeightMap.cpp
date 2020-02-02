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

#include "mapGenerator/HeightMap.h"

#include <boost/test/unit_test.hpp>

using namespace rttr::mapGenerator;

BOOST_AUTO_TEST_SUITE(HeightMapTest)

BOOST_AUTO_TEST_CASE(SeaLevel_ForHeightMapAndCoverage_ReturnsExpectedHeight)
{
    HeightMap z { 3, 3, 2, 2, 3, 3, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2 };
    
    BOOST_REQUIRE(SeaLevel(z, 0.25) == 2u);
}

BOOST_AUTO_TEST_CASE(MountainLevel_ForHeightMapAndCoverage_ReturnsExpectedHeight)
{
    HeightMap z { 3, 3, 2, 2, 3, 3, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2 };
    
    BOOST_REQUIRE(MountainLevel(z, 0.25) == 3u);
}

BOOST_AUTO_TEST_CASE(MountainLevel_ForHeightMapAndCoverageAndArea_ReturnsExpectedHeight)
{
    HeightMap z { 5, 3, 2, 1, 5, 3, 2, 1, 5, 3, 2, 1, 5, 3, 3, 2 };
    
    std::set<int> area = { 10, 11, 14, 15 };
    
    BOOST_REQUIRE(MountainLevel(z, 0.25, area) == 3u);
}


BOOST_AUTO_TEST_CASE(ScaleToFit_ForHeightMap_DoesNotChangeHeightMapSize)
{
    HeightMap z { 1, 3, 7, 21 };
    Range settings(0,32);
    
    ScaleToFit(z, settings);
    
    BOOST_REQUIRE(z.size() == 4);
}

BOOST_AUTO_TEST_CASE(ScaleToFit_ForHeightMap_SetsMinimumValueToMinimumHeight)
{
    HeightMap z { 1, 3, 7, 21 };
    Range settings(0,32);
    
    ScaleToFit(z, settings);

    auto minimum = *std::min_element(z.begin(), z.end());
    
    BOOST_REQUIRE_EQUAL(minimum, settings.min);
}

BOOST_AUTO_TEST_CASE(ScaleToFit_ForHeightMap_SetsMaximumValueToMaximumHeight)
{
    HeightMap z { 1, 3, 7, 21 };
    Range height(0,32);

    ScaleToFit(z, height);
    
    auto maximum = *std::max_element(z.begin(), z.end());
    
    BOOST_REQUIRE_EQUAL(maximum, height.max);
}

BOOST_AUTO_TEST_CASE(Smooth_ForRadius2AndSize2x2_MovesHeightOfEachPointTowardsAverage)
{
    HeightMap z = { 0, 0, 10, 10 };
    MapExtent size(2, 2);
    
    Smooth(2, 2.0, z, size);
    
    BOOST_REQUIRE_GT(z[0], 0);
    BOOST_REQUIRE_GT(z[1], 0);
    BOOST_REQUIRE_LT(z[2], 10);
    BOOST_REQUIRE_LT(z[3], 10);
}

BOOST_AUTO_TEST_CASE(Smooth_ForRadiusZero_DoesNotAlterTheHeightMap)
{
    HeightMap z = { 0, 0, 10, 10 };
    MapExtent size(2, 2);
    
    Smooth(2, 0.0, z, size);
    
    BOOST_REQUIRE_EQUAL(z[0], 0);
    BOOST_REQUIRE_EQUAL(z[1], 0);
    BOOST_REQUIRE_EQUAL(z[2], 10);
    BOOST_REQUIRE_EQUAL(z[3], 10);
}

BOOST_AUTO_TEST_SUITE_END()
