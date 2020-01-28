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
#include "randomMaps/algorithm/BrushDirection.h"

#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_SUITE(BrushDirectionTests)

BOOST_AUTO_TEST_CASE(WestDirectionCoversExpectedTriangles)
{
    auto west = BrushDirection::West();
    
    BOOST_REQUIRE(west.rsu.size() == 3u);
    BOOST_REQUIRE(west.rsu[0].x == -1);
    BOOST_REQUIRE(west.rsu[0].y == -1);
    BOOST_REQUIRE(west.rsu[1].x == -1);
    BOOST_REQUIRE(west.rsu[1].y ==  0);
    BOOST_REQUIRE(west.rsu[2].x == -1);
    BOOST_REQUIRE(west.rsu[2].y ==  1);

    BOOST_REQUIRE(west.lsd.size() == 3u);
    BOOST_REQUIRE(west.lsd[0].x == -1);
    BOOST_REQUIRE(west.lsd[0].y ==  0);
    BOOST_REQUIRE(west.lsd[1].x == -2);
    BOOST_REQUIRE(west.lsd[1].y ==  1);
    BOOST_REQUIRE(west.lsd[2].x == -1);
    BOOST_REQUIRE(west.lsd[2].y ==  2);
}

BOOST_AUTO_TEST_CASE(EastDirectionCoversExpectedTriangles)
{
    auto east = BrushDirection::East();
    
    BOOST_REQUIRE(east.rsu.size() == 3u);
    BOOST_REQUIRE(east.rsu[0].x ==  1);
    BOOST_REQUIRE(east.rsu[0].y == -1);
    BOOST_REQUIRE(east.rsu[1].x ==  2);
    BOOST_REQUIRE(east.rsu[1].y ==  0);
    BOOST_REQUIRE(east.rsu[2].x ==  1);
    BOOST_REQUIRE(east.rsu[2].y ==  1);

    BOOST_REQUIRE(east.lsd.size() == 3u);
    BOOST_REQUIRE(east.lsd[0].x ==  1);
    BOOST_REQUIRE(east.lsd[0].y ==  0);
    BOOST_REQUIRE(east.lsd[1].x ==  1);
    BOOST_REQUIRE(east.lsd[1].y ==  1);
    BOOST_REQUIRE(east.lsd[2].x ==  1);
    BOOST_REQUIRE(east.lsd[2].y ==  2);
}

BOOST_AUTO_TEST_CASE(SouthDirectionCoversExpectedTriangles)
{
    auto south = BrushDirection::South();

    BOOST_REQUIRE(south.rsu.size() == 2u);
    BOOST_REQUIRE(south.rsu[0].x ==  0);
    BOOST_REQUIRE(south.rsu[0].y ==  2);
    BOOST_REQUIRE(south.rsu[1].x ==  1);
    BOOST_REQUIRE(south.rsu[1].y ==  2);

    BOOST_REQUIRE(south.lsd.size() == 3u);
    BOOST_REQUIRE(south.lsd[0].x == -1);
    BOOST_REQUIRE(south.lsd[0].y ==  2);
    BOOST_REQUIRE(south.lsd[1].x ==  0);
    BOOST_REQUIRE(south.lsd[1].y ==  2);
    BOOST_REQUIRE(south.lsd[2].x ==  1);
    BOOST_REQUIRE(south.lsd[2].y ==  2);
}

BOOST_AUTO_TEST_CASE(NorthDirectionCoversExpectedTriangles)
{
    auto north = BrushDirection::North();

    BOOST_REQUIRE(north.rsu.size() == 3u);
    BOOST_REQUIRE(north.rsu[0].x == -1);
    BOOST_REQUIRE(north.rsu[0].y == -1);
    BOOST_REQUIRE(north.rsu[1].x ==  0);
    BOOST_REQUIRE(north.rsu[1].y == -1);
    BOOST_REQUIRE(north.rsu[2].x ==  1);
    BOOST_REQUIRE(north.rsu[2].y == -1);

    BOOST_REQUIRE(north.lsd.size() == 2u);
    BOOST_REQUIRE(north.lsd[0].x == -1);
    BOOST_REQUIRE(north.lsd[0].y == -1);
    BOOST_REQUIRE(north.lsd[1].x ==  0);
    BOOST_REQUIRE(north.lsd[1].y == -1);
}

BOOST_AUTO_TEST_CASE(NorthEastDirectionCoversExpectedTriangles)
{
    auto northEast = BrushDirection::NorthEast();

    BOOST_REQUIRE(northEast.rsu.size() == 2u);
    BOOST_REQUIRE(northEast.rsu[0].x ==  1);
    BOOST_REQUIRE(northEast.rsu[0].y == -1);
    BOOST_REQUIRE(northEast.rsu[1].x ==  2);
    BOOST_REQUIRE(northEast.rsu[1].y ==  0);

    BOOST_REQUIRE(northEast.lsd.size() == 3u);
    BOOST_REQUIRE(northEast.lsd[0].x ==  0);
    BOOST_REQUIRE(northEast.lsd[0].y == -1);
    BOOST_REQUIRE(northEast.lsd[1].x ==  1);
    BOOST_REQUIRE(northEast.lsd[1].y ==  0);
    BOOST_REQUIRE(northEast.lsd[2].x ==  1);
    BOOST_REQUIRE(northEast.lsd[2].y ==  1);
}

BOOST_AUTO_TEST_CASE(NorthWestDirectionCoversExpectedTriangles)
{
    auto northWest = BrushDirection::NorthWest();
    
    BOOST_REQUIRE(northWest.rsu.size() == 2u);
    BOOST_REQUIRE(northWest.rsu[0].x == -1);
    BOOST_REQUIRE(northWest.rsu[0].y ==  0);
    BOOST_REQUIRE(northWest.rsu[1].x == -1);
    BOOST_REQUIRE(northWest.rsu[1].y == -1);

    BOOST_REQUIRE(northWest.lsd.size() == 3u);
    BOOST_REQUIRE(northWest.lsd[0].x == -2);
    BOOST_REQUIRE(northWest.lsd[0].y ==  1);
    BOOST_REQUIRE(northWest.lsd[1].x == -1);
    BOOST_REQUIRE(northWest.lsd[1].y ==  0);
    BOOST_REQUIRE(northWest.lsd[2].x == -1);
    BOOST_REQUIRE(northWest.lsd[2].y == -1);
}

BOOST_AUTO_TEST_CASE(SouthEastDirectionCoversExpectedTriangles)
{
    auto southEast = BrushDirection::SouthEast();
    
    BOOST_REQUIRE(southEast.rsu.size() == 3u);
    BOOST_REQUIRE(southEast.rsu[0].x == -1);
    BOOST_REQUIRE(southEast.rsu[0].y ==  0);
    BOOST_REQUIRE(southEast.rsu[1].x == -1);
    BOOST_REQUIRE(southEast.rsu[1].y ==  1);
    BOOST_REQUIRE(southEast.rsu[2].x ==  0);
    BOOST_REQUIRE(southEast.rsu[2].y ==  2);

    BOOST_REQUIRE(southEast.lsd.size() == 2u);
    BOOST_REQUIRE(southEast.lsd[0].x == -2);
    BOOST_REQUIRE(southEast.lsd[0].y ==  1);
    BOOST_REQUIRE(southEast.lsd[1].x == -1);
    BOOST_REQUIRE(southEast.lsd[1].y ==  2);
}

BOOST_AUTO_TEST_CASE(SouthWestDirectionCoversExpectedTriangles)
{
    auto southWest = BrushDirection::SouthWest();

    BOOST_REQUIRE(southWest.rsu.size() == 3u);
    BOOST_REQUIRE(southWest.rsu[0].x ==  2);
    BOOST_REQUIRE(southWest.rsu[0].y ==  0);
    BOOST_REQUIRE(southWest.rsu[1].x ==  1);
    BOOST_REQUIRE(southWest.rsu[1].y ==  1);
    BOOST_REQUIRE(southWest.rsu[2].x ==  1);
    BOOST_REQUIRE(southWest.rsu[2].y ==  2);

    BOOST_REQUIRE(southWest.lsd.size() == 2u);
    BOOST_REQUIRE(southWest.lsd[0].x ==  1);
    BOOST_REQUIRE(southWest.lsd[0].y ==  1);
    BOOST_REQUIRE(southWest.lsd[1].x ==  1);
    BOOST_REQUIRE(southWest.lsd[1].y ==  2);
}

BOOST_AUTO_TEST_SUITE_END()
