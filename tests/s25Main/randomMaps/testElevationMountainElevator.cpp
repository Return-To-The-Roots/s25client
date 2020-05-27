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
#include "randomMaps/elevation/MountainElevator.h"

#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_SUITE(MountainElevatorTests)

BOOST_AUTO_TEST_CASE(MountainElevatorDoesNotChangeHeightValuesBelowMountainLevel)
{
    MapExtent size(8, 8);
    HeightSettings settings(0u, 7u);
    MountainElevator elevator(settings);
    
    std::vector<unsigned char> original = {
        0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u,
        0u, 1u, 1u, 1u, 1u, 0u, 0u, 0u,
        1u, 2u, 2u, 2u, 2u, 1u, 0u, 0u,
        1u, 2u, 3u, 3u, 3u, 2u, 1u, 0u,
        1u, 2u, 3u, 4u, 3u, 2u, 1u, 0u,
        0u, 1u, 2u, 3u, 3u, 3u, 2u, 1u,
        0u, 0u, 1u, 2u, 2u, 2u, 2u, 1u,
        0u, 0u, 0u, 1u, 1u, 1u, 1u, 0u
    };
    std::vector<unsigned char> heightMap(original);
    
    unsigned char mountainLevel = 3u;
    
    elevator.Increase(heightMap, mountainLevel, size);
    
    for (int i = 0; i < 64; i++)
    {
        if (original[i] < mountainLevel)
        {
            BOOST_REQUIRE(heightMap[i] == original[i]);
        }
    }
}

BOOST_AUTO_TEST_CASE(MountainElevatorIncreasesHeightValuesAboveMountainLevel)
{
    MapExtent size(8, 8);
    HeightSettings settings(0u, 7u);
    MountainElevator elevator(settings);
    
    std::vector<unsigned char> original = {
        0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u,
        0u, 1u, 1u, 1u, 1u, 0u, 0u, 0u,
        1u, 2u, 2u, 2u, 2u, 1u, 0u, 0u,
        1u, 2u, 3u, 3u, 3u, 2u, 1u, 0u,
        1u, 2u, 3u, 4u, 3u, 2u, 1u, 0u,
        0u, 1u, 2u, 3u, 3u, 3u, 2u, 1u,
        0u, 0u, 1u, 2u, 2u, 2u, 2u, 1u,
        0u, 0u, 0u, 1u, 1u, 1u, 1u, 0u
    };
    std::vector<unsigned char> heightMap(original);
    
    unsigned char mountainLevel = 3u;
    
    elevator.Increase(heightMap, mountainLevel, size);
    
    for (int i = 0; i < 64; i++)
    {
        if (original[i] >= mountainLevel)
        {
            BOOST_REQUIRE(heightMap[i] > original[i]);
        }
    }
}

BOOST_AUTO_TEST_SUITE_END()
