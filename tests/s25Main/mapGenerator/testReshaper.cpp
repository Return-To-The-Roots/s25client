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

#include "mapGenerator/Reshaper.h"
#include <boost/test/unit_test.hpp>

using namespace rttr::mapGenerator;

BOOST_AUTO_TEST_SUITE(ReshaperTest)

BOOST_AUTO_TEST_CASE(Elevate_ForAnyStrategy_DoesNotChangeSizeOfHeightMap)
{
    Range height(0,5);
    MapExtent size(2, 2);
    RandomUtility rnd(0);
    Reshaper shaper(rnd, height);
    
    HeightMap z { 1, 2, 3, 4 };

    shaper.Elevate(z, Reshaper::Center, size, 1.0);
    BOOST_REQUIRE(z.size() == 4);
    
    shaper.Elevate(z, Reshaper::Edges, size, 1.0);
    BOOST_REQUIRE(z.size() == 4);

    shaper.Elevate(z, Reshaper::Corners, size, 1.0);
    BOOST_REQUIRE(z.size() == 4);

    shaper.Elevate(z, Reshaper::Contrast, size, 1.0);
    BOOST_REQUIRE(z.size() == 4);

    shaper.Elevate(z, Reshaper::Random, size, 1.0);
    BOOST_REQUIRE(z.size() == 4);
}

BOOST_AUTO_TEST_CASE(Elevate_ForAnyCorner_DoesNotChangeSizeOfHeightMap)
{
    Range height(0,5);
    MapExtent size(2, 2);
    RandomUtility rnd(0);
    Reshaper shaper(rnd, height);
    
    HeightMap z { 1, 2, 3, 4 };

    shaper.Elevate(z, Reshaper::North, 2.0, size);
    BOOST_REQUIRE(z.size() == 4);

    shaper.Elevate(z, Reshaper::NorthWest, 2.0, size);
    BOOST_REQUIRE(z.size() == 4);

    shaper.Elevate(z, Reshaper::NorthEast, 2.0, size);
    BOOST_REQUIRE(z.size() == 4);

    shaper.Elevate(z, Reshaper::West, 2.0, size);
    BOOST_REQUIRE(z.size() == 4);

    shaper.Elevate(z, Reshaper::South, 2.0, size);
    BOOST_REQUIRE(z.size() == 4);

    shaper.Elevate(z, Reshaper::SouthWest, 2.0, size);
    BOOST_REQUIRE(z.size() == 4);

    shaper.Elevate(z, Reshaper::SouthEast, 2.0, size);
    BOOST_REQUIRE(z.size() == 4);

    shaper.Elevate(z, Reshaper::East, 2.0, size);
    BOOST_REQUIRE(z.size() == 4);

    shaper.Elevate(z, Reshaper::Central, 2.0, size);
    BOOST_REQUIRE(z.size() == 4);
}

BOOST_AUTO_TEST_SUITE_END()
