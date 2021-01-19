// Copyright (c) 2016 - 2018 Settlers Freaks (sf-team at siedler25.org)
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

#include "PointOutput.h"
#include "Rect.h"
#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_CASE(RectCtor)
{
    const Position origin(10, 13);
    const Extent size(5, 7);
    // Compound ctor
    Rect rect1(origin, size);
    BOOST_TEST_REQUIRE(rect1.getOrigin() == origin);
    BOOST_TEST_REQUIRE(rect1.getSize() == size);
    // Semi-Individual ctor
    Rect rect2(origin, size.x, size.y);
    BOOST_TEST_REQUIRE(rect2.getOrigin() == origin);
    BOOST_TEST_REQUIRE(rect2.getSize() == size);
    // Individual ctor
    Rect rect3(origin.x, origin.y, size.x, size.y);
    BOOST_TEST_REQUIRE(rect3.getOrigin() == origin);
    BOOST_TEST_REQUIRE(rect3.getSize() == size);
}

BOOST_AUTO_TEST_CASE(Rectmove)
{
    const Rect rectOrig(Position(10, 20), Extent(5, 10));
    Rect rect = Rect::move(rectOrig, Position(5, 7));
    BOOST_TEST_REQUIRE(rect.getOrigin() == Position(15, 27));
    BOOST_TEST_REQUIRE(rect.getSize() == rectOrig.getSize());
    rect.move(-Position(5, 7));
    BOOST_TEST_REQUIRE(rect.getOrigin() == rectOrig.getOrigin());
    BOOST_TEST_REQUIRE(rect.getSize() == rectOrig.getSize());
    Position newOrig(4, 9);
    rect.setOrigin(newOrig);
    BOOST_TEST_REQUIRE(rect.getOrigin() == newOrig);
    BOOST_TEST_REQUIRE(rect.getSize() == rectOrig.getSize());
}
