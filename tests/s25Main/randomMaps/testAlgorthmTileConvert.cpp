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
#include "randomMaps/algorithm/TileConvert.h"

#include <boost/test/unit_test.hpp>
#include <set>

BOOST_AUTO_TEST_SUITE(TileConvertTests)

BOOST_AUTO_TEST_CASE(TileConverterReturnsAVectorOfExpectedPositions)
{
    std::set<Tile, TileCompare> tiles;
    
    for (int x = 0; x < 10; x++)
    {
        for (int y = 0; y < 10; y++)
        {
            Tile tile(Position(x,y)); // tile with rsu & lsd at position x/y
            tiles.insert(tile);
        }
    }
    
    auto result = TileConvert::ToPosition(tiles);
    
    BOOST_REQUIRE(result.size() == 100u);
    
    for (int i = 0; i < 100; i++)
    {
        auto p = result[i];
        
        BOOST_REQUIRE(p.x == i % 10);
        BOOST_REQUIRE(p.y == i / 10);
    }
}

BOOST_AUTO_TEST_SUITE_END()
