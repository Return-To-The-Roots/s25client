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
#include "randomMaps/elevation/Level.h"

#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_SUITE(LevelTests)

BOOST_AUTO_TEST_CASE(CountNodesBelowLevelCorrectly)
{
    std::vector<unsigned char> z = {
        3u, 3u, 2u, 2u,
        3u, 3u, 2u, 2u,
        2u, 2u, 2u, 2u,
        2u, 2u, 2u, 2u
    };
    
    BOOST_REQUIRE(Level::CountBelowOrEqual(z, 2u) == 12);
}

BOOST_AUTO_TEST_CASE(WaterLevelCoversExpectedNumberOfTiles)
{
    std::vector<unsigned char> heightMap = {
        3u, 3u, 2u, 2u,
        3u, 3u, 2u, 2u,
        2u, 2u, 2u, 2u,
        2u, 2u, 2u, 2u
    };
    
    auto result = Level::Water(heightMap, 0.25);
    
    BOOST_REQUIRE(result == 2u);
}

BOOST_AUTO_TEST_CASE(MountainLevelCoversExpectedNumberOfTiles)
{
    std::vector<unsigned char> heightMap = {
        3u, 3u, 2u, 2u,
        3u, 3u, 2u, 2u,
        2u, 2u, 2u, 2u,
        2u, 2u, 2u, 2u
    };
    
    auto result = Level::Mountain(heightMap, 0.25);
    
    BOOST_REQUIRE(result == 3u);
}

BOOST_AUTO_TEST_CASE(MountainLevelForAreaCoversExpectedNumberOfTiles)
{
    std::vector<unsigned char> heightMap = {
        5u, 3u, 2u, 1u,
        5u, 3u, 2u, 1u,
        5u, 3u, 2u, 1u,
        5u, 3u, 3u, 2u
    };
    
    std::set<int> area = { 10, 11, 14, 15 };
    
    auto result = Level::Mountain(heightMap, 0.25, area);
    
    BOOST_REQUIRE(result == 3u);
}

BOOST_AUTO_TEST_SUITE_END()
