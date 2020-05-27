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
#include "randomMaps/algorithm/Filter.h"

#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_SUITE(FilterTests)

BOOST_AUTO_TEST_CASE(ConditionFilterTest)
{
    MapExtent size(4,4);
    DistancePredicate condition(Position(0,0), 1.1);
    ConditionFilter filter(condition, size);
    
    std::vector<Position> before { Position(1,0), Position(0,0), Position(1,1), Position(0,1) };
    
    auto after = filter.Apply(before);
    
    BOOST_REQUIRE(after.size() == 3);
    BOOST_REQUIRE(after[0] == Position(1,0));
    BOOST_REQUIRE(after[1] == Position(0,0));
    BOOST_REQUIRE(after[2] == Position(0,1));
}

BOOST_AUTO_TEST_CASE(ItemFilterTest)
{
    std::vector<Position> before { Position(1,0), Position(0,0), Position(1,1), Position(0,1) };
    std::vector<Position> remove { Position(1,0), Position(0,0), Position(1,1) };
    ItemFilter filter(remove);

    auto after = filter.Apply(before);
    
    BOOST_REQUIRE(after.size() == 1);
    BOOST_REQUIRE(after[0] == Position(0,1));
}

BOOST_AUTO_TEST_SUITE_END()
