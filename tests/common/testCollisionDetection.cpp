// Copyright (c) 2017 - 2017 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "CollisionDetection.h"
#include "Rect.h"
#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_SUITE(CollisionDetection)

BOOST_AUTO_TEST_CASE(PointInRect)
{
    Rect rect(Position(10, 20), Extent(5, 10));
    // Point on edge. Note: right/bottom is OUTSIDE
    BOOST_TEST(IsPointInRect(Position(10, 20), rect));
    BOOST_TEST(!IsPointInRect(Position(15, 20), rect));
    BOOST_TEST(!IsPointInRect(Position(10, 30), rect));
    BOOST_TEST(!IsPointInRect(Position(15, 30), rect));
    // Point on line
    // Top
    BOOST_TEST(IsPointInRect(Position(12, 20), rect));
    // Left
    BOOST_TEST(IsPointInRect(Position(10, 25), rect));
    // Bottom
    BOOST_TEST(!IsPointInRect(Position(12, 30), rect));
    // Right
    BOOST_TEST(!IsPointInRect(Position(15, 25), rect));
    // Point in middle
    BOOST_TEST(IsPointInRect(Position(12, 25), rect));
    // Point outside
    // Top
    BOOST_TEST(!IsPointInRect(Position(12, 19), rect));
    // Left
    BOOST_TEST(!IsPointInRect(Position(9, 25), rect));
    // Bottom
    BOOST_TEST(!IsPointInRect(Position(12, 31), rect));
    // Right
    BOOST_TEST(!IsPointInRect(Position(16, 25), rect));
}

BOOST_AUTO_TEST_CASE(RectIntersect)
{
    Rect rect1(Position(10, 20), Extent(5, 15));
    Rect rect2(rect1);
    // Identity
    BOOST_TEST(DoRectsIntersect(rect1, rect2));
    BOOST_TEST(DoRectsIntersect(rect2, rect1));
    // Just in and out on each line
    // Right
    rect2.move(Position(rect1.getSize().x, 0)); //-V807
    BOOST_TEST(!DoRectsIntersect(rect1, rect2));
    BOOST_TEST(!DoRectsIntersect(rect2, rect1));
    rect2.move(-Position(1, 0));
    BOOST_TEST(DoRectsIntersect(rect1, rect2));
    BOOST_TEST(DoRectsIntersect(rect2, rect1));
    // Left
    rect2 = rect1;
    rect2.move(-Position(rect1.getSize().x, 0));
    BOOST_TEST(!DoRectsIntersect(rect1, rect2));
    BOOST_TEST(!DoRectsIntersect(rect2, rect1));
    rect2.move(Position(1, 0));
    BOOST_TEST(DoRectsIntersect(rect1, rect2));
    BOOST_TEST(DoRectsIntersect(rect2, rect1));
    // Bottom
    rect2 = rect1;
    rect2.move(Position(0, rect1.getSize().y));
    BOOST_TEST(!DoRectsIntersect(rect1, rect2));
    BOOST_TEST(!DoRectsIntersect(rect2, rect1));
    rect2.move(-Position(0, 1));
    BOOST_TEST(DoRectsIntersect(rect1, rect2));
    BOOST_TEST(DoRectsIntersect(rect2, rect1));
    // Top
    rect2 = rect1;
    rect2.move(-Position(0, rect1.getSize().y));
    BOOST_TEST(!DoRectsIntersect(rect1, rect2));
    BOOST_TEST(!DoRectsIntersect(rect2, rect1));
    rect2.move(Position(0, 1));
    BOOST_TEST(DoRectsIntersect(rect1, rect2));
    BOOST_TEST(DoRectsIntersect(rect2, rect1));

    // Empty rect
    Rect emptyRect(rect1.getOrigin(), Extent(0, 0));
    BOOST_TEST(!DoRectsIntersect(rect1, emptyRect));
    BOOST_TEST(!DoRectsIntersect(emptyRect, rect1));
}

BOOST_AUTO_TEST_SUITE_END()
