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
#include "randomMaps/terrain/CoastlineCalculator.h"

#include <boost/test/unit_test.hpp>
#include <iostream>

BOOST_AUTO_TEST_SUITE(CoastlineCalulatorTests)

BOOST_AUTO_TEST_CASE(ReturnsExpectedNumberOfCoastTilesForIsland)
{
    MapExtent size(8,8);
    std::vector<Position> island = {
        Position(3,3), Position(4,3), Position(5,3),
        Position(3,4), Position(4,4), Position(5,4),
        Position(3,5), Position(4,5), Position(5,5)
    };
    
    std::vector<bool> water = {
        true, true, true, true, true, true, true, true,
        true, true, true, true, true, true, true, true,
        true, true, true, true, true, true, true, true,
        true, true, true,false,false,false, true, true,
        true, true, true,false,false,false, true, true,
        true, true, true,false,false,false, true, true,
        true, true, true, true, true, true, true, true,
        true, true, true, true, true, true, true, true
    };
    
    
    auto coastLine = CoastlineCalculator().For(island, water, size);
    
    BOOST_REQUIRE(coastLine.size() == 8u);
}

BOOST_AUTO_TEST_CASE(DoNotConsiderRiverBanksAsCoastline)
{
    MapExtent size(8,8);
    std::vector<Position> island = {
        Position(3,0), Position(4,0), Position(5,0), Position(6,0), Position(7,0),
        Position(3,1), Position(4,1), Position(5,1), Position(6,1), Position(7,1),
        Position(3,2), Position(4,2), Position(5,2), Position(6,2), Position(7,2),
        Position(3,3), Position(4,3), Position(5,3), Position(6,3), Position(7,3),
        Position(3,4), Position(4,4), Position(5,4), Position(6,4), Position(7,4),
        Position(3,5), Position(4,5), Position(5,5), Position(6,5), Position(7,5),
        // ================= r i v e r ==========================================
        Position(3,7), Position(4,7), Position(5,6), Position(6,7), Position(7,7)
    };
    
    std::vector<bool> water = {
        true, true, true,false,false,false,false,false,
        true, true, true,false,false,false,false,false,
        true, true, true,false,false,false,false,false,
        true, true, true,false,false,false,false,false,
        true, true, true,false,false,false,false,false,
        true, true, true,false,false,false,false,false,
        true, true, true, true, true, true, true, true,
        true, true, true,false,false,false,false,false
    };
    
    auto coastLine = CoastlineCalculator().For(island, water, size);
    
    BOOST_REQUIRE(coastLine.size() <= 15u);
}

BOOST_AUTO_TEST_SUITE_END()
