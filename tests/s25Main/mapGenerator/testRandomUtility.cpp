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

BOOST_AUTO_TEST_CASE(Index_ForSize_ReturnsValuesGreaterOrEqualToZeroAndLessThanSize)
{
    RandomUtility rnd(0u);
    
    for (auto size = 1u; size < 10u; size++)
    {
        auto result = rnd.Index(size);
        
        BOOST_REQUIRE(result >= 0 && result < (int)size);
    }
}

BOOST_AUTO_TEST_CASE(PRand_ForMapSize_ReturnsPositionOnTheMap)
{
    RandomUtility rnd(0u);
    MapExtent size(23,12);
    
    auto result = rnd.RandomPoint(size);
    
    BOOST_REQUIRE(result.x >= 0 && result.x < size.x);
    BOOST_REQUIRE(result.y >= 0 && result.y < size.y);
}

BOOST_AUTO_TEST_CASE(Rand_ForMinimumAndMaximum_ReturnsValueBetweenRange)
{
    RandomUtility rnd(0u);
    
    int minimum = -10;
    int maximum = 7;
    
    auto result = rnd.Rand(minimum, maximum);
    
    BOOST_REQUIRE(result >= minimum);
    BOOST_REQUIRE(result <= maximum);
}

BOOST_AUTO_TEST_CASE(DRand_ReturnsValuesBetweenMinAndMax)
{
    RandomUtility rnd(0u);

    double minimum = -12.123;
    double maximum = 7.456;
    
    auto result = rnd.DRand(minimum, maximum);
    
    BOOST_REQUIRE(result >= minimum);
    BOOST_REQUIRE(result <= maximum);
}

BOOST_AUTO_TEST_CASE(IRand_ForAmount_ReturnsSpecifiedNumberOfIndices)
{
    RandomUtility rnd(0u);
    
    BOOST_REQUIRE(rnd.ShuffledRange(10).size() == 10u);
}

BOOST_AUTO_TEST_CASE(IRand_ForAmount_ReturnsOnlyUniqueValues)
{
    RandomUtility rnd(0u);

    auto result = rnd.ShuffledRange(10);
    
    std::set<int> values;
    
    for (auto value : result)
    {
        BOOST_REQUIRE(values.insert(value).second);
    }
}

BOOST_AUTO_TEST_CASE(IRand_ForAmount_ReturnsOnlyValuesBetweenZeroAndAmountMinusOne)
{
    RandomUtility rnd(0u);

    auto result = rnd.ShuffledRange(10);

    for (auto value : result)
    {
        BOOST_REQUIRE(value >= 0 && value < 10);
    }
}

BOOST_AUTO_TEST_CASE(Shuffle_ForPositionVector_DoesNotAddOrRemovePositions)
{
    RandomUtility rnd(0u);

    std::vector<Position> original = {Position(1,2), Position(2,3), Position(3,4)};
    std::vector<Position> area(original);
    
    rnd.Shuffle(area);

    BOOST_REQUIRE(area.size() == original.size());
    
    for (auto p : original)
    {
        bool found = false;
        for (auto q : area)
        {
            if (p.x == q.x && p.y == q.y)
            {
                found = true;
                break;
            }
        }
        BOOST_REQUIRE(found);
    }
}

BOOST_AUTO_TEST_SUITE_END()
