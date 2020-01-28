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
#include "randomMaps/algorithm/GridPredicate.h"

#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_SUITE(GridPredicateTests)

BOOST_AUTO_TEST_CASE(DistancePredicateTest)
{
    MapExtent size(16,32);
    Position position(1,1);
    double distance = 1.5;
    
    DistancePredicate pred(position, distance);

    BOOST_REQUIRE(pred.Check(Position(4,4), size) == false);
    BOOST_REQUIRE(pred.Check(Position(1,2), size) == true);
}

BOOST_AUTO_TEST_CASE(ThresholdPredicateTest)
{
    MapExtent size(2,2);
    std::vector<double> values {
        0.0, 1.5,
        1.0, 2.0
    };
    
    double threshold = 1.5;
    
    ThresholdPredicate pred(values, threshold);
    
    BOOST_REQUIRE(pred.Check(Position(0,0), size) == true);
    BOOST_REQUIRE(pred.Check(Position(1,0), size) == false);
    BOOST_REQUIRE(pred.Check(Position(0,1), size) == true);
    BOOST_REQUIRE(pred.Check(Position(1,1), size) == false);
}

BOOST_AUTO_TEST_CASE(CharThresholdPredicateTest)
{
    MapExtent size(2,2);
    std::vector<unsigned char> values {
        1, 4,
        3, 5
    };
    
    unsigned char threshold = 4;
    
    CharThresholdPredicate pred(values, threshold);
    
    BOOST_REQUIRE(pred.Check(Position(0,0), size) == true);
    BOOST_REQUIRE(pred.Check(Position(1,0), size) == false);
    BOOST_REQUIRE(pred.Check(Position(0,1), size) == true);
    BOOST_REQUIRE(pred.Check(Position(1,1), size) == false);
}

BOOST_AUTO_TEST_CASE(ExcludePredicateTest)
{
    MapExtent size(2,2);
    std::vector<bool> excludes {
        true, false,
        true, false
    };
    
    ExclusionPredicate pred(excludes);
    
    BOOST_REQUIRE(pred.Check(Position(0,0), size) == false);
    BOOST_REQUIRE(pred.Check(Position(1,0), size) == true);
    BOOST_REQUIRE(pred.Check(Position(0,1), size) == false);
    BOOST_REQUIRE(pred.Check(Position(1,1), size) == true);
}

BOOST_AUTO_TEST_CASE(IncludePredicateTest)
{
    MapExtent size(2,2);
    std::vector<bool> includes {
        true, false,
        true, false
    };
    
    IncludePredicate pred(includes);
    
    BOOST_REQUIRE(pred.Check(Position(0,0), size) == true);
    BOOST_REQUIRE(pred.Check(Position(1,0), size) == false);
    BOOST_REQUIRE(pred.Check(Position(0,1), size) == true);
    BOOST_REQUIRE(pred.Check(Position(1,1), size) == false);
}

BOOST_AUTO_TEST_SUITE_END()
