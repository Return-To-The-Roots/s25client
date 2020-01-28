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
#include "randomMaps/elevation/Scaler.h"

#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_SUITE(ScalerTest)

BOOST_AUTO_TEST_CASE(ScalerDoesntChangePointCount)
{
    HeightSettings settings(0,32);
    std::vector<unsigned char> points { 1, 3, 7, 21 };

    Scaler scaler(settings);
    scaler.Scale(points);
    
    BOOST_REQUIRE(points.size() == 4);
}

BOOST_AUTO_TEST_CASE(MinimumValueOfResultEqualsMinimumHeight)
{
    HeightSettings settings(0,32);
    std::vector<unsigned char> points { 1, 3, 7, 21 };

    Scaler scaler(settings);
    scaler.Scale(points);

    unsigned char minimum = *std::min_element(points.begin(), points.end());
    
    BOOST_REQUIRE_EQUAL(minimum, settings.minimum);
}

BOOST_AUTO_TEST_CASE(MaximumValueOfResultEqualsMaximumHeight)
{
    HeightSettings settings(0,32);
    std::vector<unsigned char> points { 1, 3, 7, 21 };
    
    Scaler scaler(settings);
    scaler.Scale(points);
    
    auto maximum = *std::max_element(points.begin(), points.end());
    
    BOOST_REQUIRE_EQUAL(maximum, settings.maximum);
}

BOOST_AUTO_TEST_SUITE_END()
