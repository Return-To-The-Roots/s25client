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
    BOOST_REQUIRE_EQUAL(tr.ConvertCoords(Position(0, 0), &offset), MapPoint(0, 0));
    BOOST_REQUIRE_EQUAL(offset, Position(0, 0));
    BOOST_REQUIRE_EQUAL(tr.ConvertCoords(Position(w - 1, h - 1), &offset), MapPoint(w - 1, h - 1));
    BOOST_REQUIRE_EQUAL(offset, Position(0, 0));
    BOOST_REQUIRE_EQUAL(tr.ConvertCoords(Position(w, h - 1), &offset), MapPoint(0, h - 1));
    BOOST_REQUIRE_EQUAL(offset, Position(w * TR_W, 0));
    BOOST_REQUIRE_EQUAL(tr.ConvertCoords(Position(w - 1, h), &offset), MapPoint(w - 1, 0));
    BOOST_REQUIRE_EQUAL(offset, Position(0, h * TR_H));
    BOOST_REQUIRE_EQUAL(tr.ConvertCoords(Position(w, h), &offset), MapPoint(0, 0));
    BOOST_REQUIRE_EQUAL(offset, Position(w * TR_W, h * TR_H));
    BOOST_REQUIRE_EQUAL(tr.ConvertCoords(Position(w + w / 2, h + h / 2), &offset), MapPoint(w / 2, h / 2));
    BOOST_REQUIRE_EQUAL(offset, Position(w * TR_W, h * TR_H));
    // Big value
    BOOST_REQUIRE_EQUAL(tr.ConvertCoords(Position(10 * w + w / 2, 11 * h + h / 2), &offset), MapPoint(w / 2, h / 2));
    BOOST_REQUIRE_EQUAL(offset, Position(10 * w * TR_W, 11 * h * TR_H));

    // Negative cases
    BOOST_REQUIRE_EQUAL(tr.ConvertCoords(Position(-1, -1), &offset), MapPoint(w - 1, h - 1));
    BOOST_REQUIRE_EQUAL(offset, Position(-w * TR_W, -h * TR_H));
    BOOST_REQUIRE_EQUAL(tr.ConvertCoords(Position(-w + 1, -h + 2), &offset), MapPoint(1, 2));
    BOOST_REQUIRE_EQUAL(offset, Position(-w * TR_W, -h * TR_H));
    BOOST_REQUIRE_EQUAL(tr.ConvertCoords(Position(-w, -h), &offset), MapPoint(0, 0));
    BOOST_REQUIRE_EQUAL(offset, Position(-w * TR_W, -h * TR_H));
    BOOST_REQUIRE_EQUAL(tr.ConvertCoords(Position(-w - 1, -h - 2), &offset), MapPoint(w - 1, h - 2));
    BOOST_REQUIRE_EQUAL(offset, Position(-2 * w * TR_W, -2 * h * TR_H));
    // Big value
    BOOST_REQUIRE_EQUAL(tr.ConvertCoords(Position(-10 * w + w / 2, -11 * h + h / 2), &offset), MapPoint(w / 2, h / 2));
    BOOST_REQUIRE_EQUAL(offset, Position(-10 * w * TR_W, -11 * h * TR_H));
}
