// Copyright (C) 2016 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

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
