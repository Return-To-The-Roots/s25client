// Copyright (C) 2016 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "PointOutput.h"
#include "TerrainRenderer.h"
#include "gameData/MapConsts.h"
#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_CASE(TR_ConvertCoords)
{
    TerrainRenderer tr;
    const int w = 23;
    const int h = 32;
    tr.Init(MapExtent(w, h));

    Position offset;
    // Test border cases
    BOOST_TEST_REQUIRE(tr.ConvertCoords(Position(0, 0), &offset) == MapPoint(0, 0));
    BOOST_TEST_REQUIRE(offset == Position(0, 0));
    BOOST_TEST_REQUIRE(tr.ConvertCoords(Position(w - 1, h - 1), &offset) == MapPoint(w - 1, h - 1));
    BOOST_TEST_REQUIRE(offset == Position(0, 0));
    BOOST_TEST_REQUIRE(tr.ConvertCoords(Position(w, h - 1), &offset) == MapPoint(0, h - 1));
    BOOST_TEST_REQUIRE(offset == Position(w * TR_W, 0));
    BOOST_TEST_REQUIRE(tr.ConvertCoords(Position(w - 1, h), &offset) == MapPoint(w - 1, 0));
    BOOST_TEST_REQUIRE(offset == Position(0, h * TR_H));
    BOOST_TEST_REQUIRE(tr.ConvertCoords(Position(w, h), &offset) == MapPoint(0, 0));
    BOOST_TEST_REQUIRE(offset == Position(w * TR_W, h * TR_H));
    BOOST_TEST_REQUIRE(tr.ConvertCoords(Position(w + w / 2, h + h / 2), &offset) == MapPoint(w / 2, h / 2));
    BOOST_TEST_REQUIRE(offset == Position(w * TR_W, h * TR_H));
    // Big value
    BOOST_TEST_REQUIRE(tr.ConvertCoords(Position(10 * w + w / 2, 11 * h + h / 2), &offset) == MapPoint(w / 2, h / 2));
    BOOST_TEST_REQUIRE(offset == Position(10 * w * TR_W, 11 * h * TR_H));

    // Negative cases
    BOOST_TEST_REQUIRE(tr.ConvertCoords(Position(-1, -1), &offset) == MapPoint(w - 1, h - 1));
    BOOST_TEST_REQUIRE(offset == Position(-w * TR_W, -h * TR_H));
    BOOST_TEST_REQUIRE(tr.ConvertCoords(Position(-w + 1, -h + 2), &offset) == MapPoint(1, 2));
    BOOST_TEST_REQUIRE(offset == Position(-w * TR_W, -h * TR_H));
    BOOST_TEST_REQUIRE(tr.ConvertCoords(Position(-w, -h), &offset) == MapPoint(0, 0));
    BOOST_TEST_REQUIRE(offset == Position(-w * TR_W, -h * TR_H));
    BOOST_TEST_REQUIRE(tr.ConvertCoords(Position(-w - 1, -h - 2), &offset) == MapPoint(w - 1, h - 2));
    BOOST_TEST_REQUIRE(offset == Position(-2 * w * TR_W, -2 * h * TR_H));
    // Big value
    BOOST_TEST_REQUIRE(tr.ConvertCoords(Position(-10 * w + w / 2, -11 * h + h / 2), &offset) == MapPoint(w / 2, h / 2));
    BOOST_TEST_REQUIRE(offset == Position(-10 * w * TR_W, -11 * h * TR_H));
}
