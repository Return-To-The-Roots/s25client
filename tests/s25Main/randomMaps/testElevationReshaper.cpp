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
#include "randomMaps/elevation/Reshaper.h"

#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_SUITE(ReshaperTest)

BOOST_AUTO_TEST_CASE(DoesntChangeNumberOfPoints)
{
    HeightSettings settings(0,32);
    MapExtent size(2, 2);
    RandomUtility rnd(0);
    Reshaper shaper(rnd, settings);
    
    std::vector<unsigned char> result {
        settings.minimum,
        settings.minimum,
        settings.maximum,
        settings.maximum
    };

    shaper.Elevate(result, Center, size, 1.0);
    BOOST_REQUIRE(result.size() == 4);
    
    shaper.Elevate(result, Edges, size, 1.0);
    BOOST_REQUIRE(result.size() == 4);

    shaper.Elevate(result, Corners, size, 1.0);
    BOOST_REQUIRE(result.size() == 4);

    shaper.Elevate(result, Contrast, size, 1.0);
    BOOST_REQUIRE(result.size() == 4);

    shaper.Elevate(result, Random, size, 1.0);
    BOOST_REQUIRE(result.size() == 4);
}

BOOST_AUTO_TEST_CASE(ElevationOfCornersDoesNotChangeNumberOfPoints)
{
    HeightSettings settings(0,32);
    MapExtent size(2, 2);
    RandomUtility rnd(0);
    Reshaper shaper(rnd, settings);
    
    std::vector<unsigned char> result {
        settings.minimum,
        settings.minimum,
        settings.maximum,
        settings.maximum
    };

    shaper.Elevate(result, North, 2.0, size);
    BOOST_REQUIRE(result.size() == 4);

    shaper.Elevate(result, NorthWest, 2.0, size);
    BOOST_REQUIRE(result.size() == 4);

    shaper.Elevate(result, NorthEast, 2.0, size);
    BOOST_REQUIRE(result.size() == 4);

    shaper.Elevate(result, West, 2.0, size);
    BOOST_REQUIRE(result.size() == 4);

    shaper.Elevate(result, South, 2.0, size);
    BOOST_REQUIRE(result.size() == 4);

    shaper.Elevate(result, SouthWest, 2.0, size);
    BOOST_REQUIRE(result.size() == 4);

    shaper.Elevate(result, SouthEast, 2.0, size);
    BOOST_REQUIRE(result.size() == 4);

    shaper.Elevate(result, East, 2.0, size);
    BOOST_REQUIRE(result.size() == 4);

    shaper.Elevate(result, Central, 2.0, size);
    BOOST_REQUIRE(result.size() == 4);
}

BOOST_AUTO_TEST_SUITE_END()
