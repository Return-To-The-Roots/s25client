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

#include "defines.h" // IWYU pragma: keep
#include "Rect.h"
#include "CollisionDetection.h"
#include "PointOutput.h"
#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_SUITE(CollisionDetection)

BOOST_AUTO_TEST_CASE(RectCtor)
{
    const Point<int> origin(10, 13);
    const Extent size(5, 7);
    // Compound ctor
    Rect rect1(origin, size);
    BOOST_REQUIRE_EQUAL(rect1.getOrigin(), origin);
    BOOST_REQUIRE_EQUAL(rect1.getSize(), size);
    // Semi-Individual ctor
    Rect rect2(origin, size.x, size.y);
    BOOST_REQUIRE_EQUAL(rect2.getOrigin(), origin);
    BOOST_REQUIRE_EQUAL(rect2.getSize(), size);
    // Individual ctor
    Rect rect3(origin.x, origin.y, size.x, size.y);
    BOOST_REQUIRE_EQUAL(rect3.getOrigin(), origin);
    BOOST_REQUIRE_EQUAL(rect3.getSize(), size);
}

BOOST_AUTO_TEST_CASE(Rectmove)
{
    const Rect rectOrig(Point<int>(10, 20), Extent(5, 10));
    Rect rect = Rect::move(rectOrig, Point<int>(5, 7));
    BOOST_REQUIRE_EQUAL(rect.getOrigin(), Point<int>(15, 27));
    BOOST_REQUIRE_EQUAL(rect.getSize(), rectOrig.getSize());
    rect.move(-Point<int>(5, 7));
    BOOST_REQUIRE_EQUAL(rect.getOrigin(), rectOrig.getOrigin());
    BOOST_REQUIRE_EQUAL(rect.getSize(), rectOrig.getSize());
}

BOOST_AUTO_TEST_CASE(PointInRect)
{
    Rect rect(Point<int>(10, 20), Extent(5, 10));
    // Point on edge. Note: right/bottom is OUTSIDE
    BOOST_REQUIRE(IsPointInRect(Point<int>(10, 20), rect));
    BOOST_REQUIRE(!IsPointInRect(Point<int>(15, 20), rect));
    BOOST_REQUIRE(!IsPointInRect(Point<int>(10, 30), rect));
    BOOST_REQUIRE(!IsPointInRect(Point<int>(15, 30), rect));
    // Point on line
    // Top
    BOOST_REQUIRE(IsPointInRect(Point<int>(12, 20), rect));
    // Left
    BOOST_REQUIRE(IsPointInRect(Point<int>(10, 25), rect));
    // Bottom
    BOOST_REQUIRE(!IsPointInRect(Point<int>(12, 30), rect));
    // Right
    BOOST_REQUIRE(!IsPointInRect(Point<int>(15, 25), rect));
    // Point in middle
    BOOST_REQUIRE(IsPointInRect(Point<int>(12, 25), rect));
    // Point outside
    // Top
    BOOST_REQUIRE(!IsPointInRect(Point<int>(12, 19), rect));
    // Left
    BOOST_REQUIRE(!IsPointInRect(Point<int>(9, 25), rect));
    // Bottom
    BOOST_REQUIRE(!IsPointInRect(Point<int>(12, 31), rect));
    // Right
    BOOST_REQUIRE(!IsPointInRect(Point<int>(16, 25), rect));
}

BOOST_AUTO_TEST_CASE(RectIntersect)
{
    Rect rect1(Point<int>(10, 20), Extent(5, 15));
    Rect rect2(rect1);
    // Identity
    BOOST_REQUIRE(DoRectsIntersect(rect1, rect2));
    BOOST_REQUIRE(DoRectsIntersect(rect2, rect1));
    // Just in and out on each line
    // Right
    rect2.move(Point<int>(rect1.getSize().x, 0));
    BOOST_REQUIRE(!DoRectsIntersect(rect1, rect2));
    BOOST_REQUIRE(!DoRectsIntersect(rect2, rect1));
    rect2.move(-Point<int>(1, 0));
    BOOST_REQUIRE(DoRectsIntersect(rect1, rect2));
    BOOST_REQUIRE(DoRectsIntersect(rect2, rect1));
    // Left
    rect2 = rect1;
    rect2.move(-Point<int>(rect1.getSize().x, 0));
    BOOST_REQUIRE(!DoRectsIntersect(rect1, rect2));
    BOOST_REQUIRE(!DoRectsIntersect(rect2, rect1));
    rect2.move(Point<int>(1, 0));
    BOOST_REQUIRE(DoRectsIntersect(rect1, rect2));
    BOOST_REQUIRE(DoRectsIntersect(rect2, rect1));
    // Bottom
    rect2 = rect1;
    rect2.move(Point<int>(0, rect1.getSize().y));
    BOOST_REQUIRE(!DoRectsIntersect(rect1, rect2));
    BOOST_REQUIRE(!DoRectsIntersect(rect2, rect1));
    rect2.move(-Point<int>(0, 1));
    BOOST_REQUIRE(DoRectsIntersect(rect1, rect2));
    BOOST_REQUIRE(DoRectsIntersect(rect2, rect1));
    // Top
    rect2 = rect1;
    rect2.move(-Point<int>(0, rect1.getSize().y));
    BOOST_REQUIRE(!DoRectsIntersect(rect1, rect2));
    BOOST_REQUIRE(!DoRectsIntersect(rect2, rect1));
    rect2.move(Point<int>(0, 1));
    BOOST_REQUIRE(DoRectsIntersect(rect1, rect2));
    BOOST_REQUIRE(DoRectsIntersect(rect2, rect1));

    // Empty rect
    Rect emptyRect(rect1.getOrigin(), Extent(0, 0));
    BOOST_REQUIRE(!DoRectsIntersect(rect1, emptyRect));
    BOOST_REQUIRE(!DoRectsIntersect(emptyRect, rect1));
}

BOOST_AUTO_TEST_SUITE_END()
