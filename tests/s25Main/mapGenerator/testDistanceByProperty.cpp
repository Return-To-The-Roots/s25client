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

#include "mapGenerator/DistanceByProperty.h"
#include <boost/test/unit_test.hpp>

using namespace rttr::mapGenerator;

BOOST_AUTO_TEST_SUITE(DistanceByPropertyTests)

// OLD WORLD

bool Evaluator(int index);
bool Evaluator(int index)
{
    std::vector<bool> field {
                                    false, false, false, false, false, false, false, false,
                             false, true,  true,  true,  true,  true,  true,  false,
                             false, false, true,  true,  true,  true,  true,  false,
                      false, false, true,  true,  true,  true,  true,  false,
                      false, false, true,  true,  true,  true,  true,  false,
               false, false, true,  true,  true,  true,  true,  false,
               false, true,  true,  true,  false, true,  true,  false,
        false, false, false, false, false, false, false, false
    };
    
    return !field[index];
}

BOOST_AUTO_TEST_CASE(DistanceByProperty_ForMapAndEvaluator_ReturnsExpectedDistanceMap)
{
    MapExtent size(8, 8);
    
    auto distance = DistanceByProperty(size, Evaluator);
    
    std::vector<int> expectedDistance {
                    0, 0, 0, 0, 0, 0, 0, 0,
                 0, 1, 1, 1, 1, 1, 1, 0,
                 0, 0, 1, 2, 2, 2, 1, 0,
              0, 0, 1, 2, 3, 2, 1, 0,
              0, 0, 1, 2, 3, 2, 1, 0,
           0, 0, 1, 2, 1, 2, 1, 0,
           0, 1, 1, 1, 0, 1, 1, 0,
        0, 0, 0, 0, 0, 0, 0, 0 };
    
    BOOST_REQUIRE_EQUAL(64u, distance.size());
    
    for (int i = 0; i < 64; i++)
    {
        BOOST_REQUIRE_EQUAL(distance[i], expectedDistance[i]);
    }
}

BOOST_AUTO_TEST_CASE(DistanceByProperty_ForSubset_ReturnsExpectedDistanceMapForSubset)
{
    MapExtent size(8, 8);

    std::vector<Position> subset = { Position(0,0), Position(1,0), Position(0,1), Position(1,1) };
    
    auto distance = DistanceByProperty(subset, size, Evaluator);

    std::vector<int> expectedDistance = { 0, 0, 0, 1 };
    
    BOOST_REQUIRE_EQUAL(expectedDistance.size(), distance.size());
    
    for (int i = 0; i < 4; i++)
    {
        BOOST_REQUIRE_EQUAL(expectedDistance[i], distance[i]);
    }
}

BOOST_AUTO_TEST_SUITE_END()
