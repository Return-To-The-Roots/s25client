// Copyright (c) 2016 - 2017 Settlers Freaks (sf-team at siedler25.org)
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
#include "CreateEmptyWorld.h"
#include "GamePlayer.h"
#include "PointOutput.h"
#include "WorldFixture.h"
#include "world/MapGeometry.h"
#include "nodeObjs/noBase.h"
#include <boost/assign/std/vector.hpp>
#include <boost/foreach.hpp>
#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_SUITE(WorldCreationSuite)

BOOST_AUTO_TEST_CASE(NeighbourPts)
{
    using namespace boost::assign;
    MapBase world;
    world.Resize(MapExtent(20, 30));

    /*  Note that every 2nd row is shifted by half a triangle to the left, therefore:
    Modifications for the dirs: */
    std::vector<Position> evenPtMod(6);
    std::vector<Position> oddPtMod(6);
    // In x dir there is no difference
    evenPtMod[Direction::WEST] = Position(-1, 0);
    oddPtMod[Direction::WEST] = Position(-1, 0);
    evenPtMod[Direction::EAST] = Position(1, 0);
    oddPtMod[Direction::EAST] = Position(1, 0);
    // To north we decrease y and may change x. x changed in NW <=> not changed in NE
    evenPtMod[Direction::NORTHWEST] = Position(-1, -1);
    oddPtMod[Direction::NORTHWEST] = Position(0, -1);
    evenPtMod[Direction::NORTHEAST] = Position(0, -1);
    oddPtMod[Direction::NORTHEAST] = Position(1, -1);
    // And similar for south. X offsets stay the same!
    evenPtMod[Direction::SOUTHWEST] = Position(-1, 1);
    oddPtMod[Direction::SOUTHWEST] = Position(0, 1);
    evenPtMod[Direction::SOUTHEAST] = Position(0, 1);
    oddPtMod[Direction::SOUTHEAST] = Position(1, 1);
    std::vector<Position> testPoints;
    // Test a simple even and odd point
    testPoints += Position(10, 10), Position(10, 9);
    // Test border points
    testPoints += Position(0, 0), Position(0, world.GetHeight() - 1), Position(world.GetWidth() - 1, 0),
      Position(world.GetWidth() - 1, world.GetHeight() - 1);
    // Test border points with 1 offset in Y
    testPoints += Position(0, 1), Position(0, world.GetHeight() - 2), Position(world.GetWidth() - 1, 1),
      Position(world.GetWidth() - 1, world.GetHeight() - 2);
    BOOST_FOREACH(const Position& pt, testPoints)
    {
        for(unsigned dir = 0; dir < Direction::COUNT; dir++)
        {
            const bool isEvenRow = pt.y % 2 == 0;
            const Position targetPointRaw = pt + (isEvenRow ? evenPtMod : oddPtMod)[dir];
            const MapPoint targetPoint((targetPointRaw.x + world.GetWidth()) % world.GetWidth(),
                                       (targetPointRaw.y + world.GetHeight()) % world.GetHeight());
            BOOST_REQUIRE_EQUAL(world.CalcDistance(MapPoint(pt), targetPoint), 1u);

            BOOST_REQUIRE_EQUAL(world.GetNeighbour(MapPoint(pt), Direction::fromInt(dir)), targetPoint);
            // Consistency check: The inverse must also match
            BOOST_REQUIRE_EQUAL(world.GetNeighbour(targetPoint, Direction(dir + 3)), MapPoint(pt));
            // Also the global function must return the same:
            BOOST_REQUIRE_EQUAL(::GetNeighbour(Position(pt), Direction::fromInt(dir)), targetPointRaw);
            BOOST_REQUIRE_EQUAL(world.MakeMapPoint(::GetNeighbour(Position(pt), Direction::fromInt(dir))), targetPoint);
        }

        // Neighbour 2 -> Radius 2, right circle
        MapPoint curPt(pt);
        MapPoint curTargetPoint = world.GetNeighbour(world.GetNeighbour(curPt, Direction::WEST), Direction::WEST);
        BOOST_REQUIRE_EQUAL(world.CalcDistance(curPt, curTargetPoint), 2u);
        BOOST_REQUIRE_EQUAL(world.GetNeighbour2(curPt, 0), curTargetPoint);
        // We now go 2 steps in each direction to describe the circle
        // (Note: Could iterator over directions, but to easily indentifiy errors and make intentions clear we do it explicitely)
        std::vector<Direction> steps;
        steps += Direction::NORTHEAST, Direction::NORTHEAST, Direction::EAST, Direction::EAST, Direction::SOUTHEAST, Direction::SOUTHEAST;
        steps += Direction::SOUTHWEST, Direction::SOUTHWEST, Direction::WEST, Direction::WEST, Direction::NORTHWEST;
        // Note: 0==12 already handled
        for(unsigned j = 1; j < 12; j++)
        {
            curTargetPoint = world.GetNeighbour(curTargetPoint, steps.at(j - 1));
            BOOST_REQUIRE_EQUAL(world.CalcDistance(curPt, curTargetPoint), 2u);
            BOOST_REQUIRE_EQUAL(world.GetNeighbour2(curPt, j), curTargetPoint);
        }

        // And finally the points in radius
        std::vector<MapPoint> radiusPts = world.GetPointsInRadiusWithCenter(curPt, 3);
        BOOST_REQUIRE_EQUAL(radiusPts.size(), 1u + 6u + 12u + 18u);
        BOOST_REQUIRE_EQUAL(radiusPts[0], curPt);
        for(unsigned j = 0; j < 6; j++)
            BOOST_REQUIRE_EQUAL(radiusPts[j + 1], world.GetNeighbour(curPt, Direction::fromInt(j)));
        for(unsigned j = 0; j < 12; j++)
            BOOST_REQUIRE_EQUAL(radiusPts[j + 7], world.GetNeighbour2(curPt, j));
        for(unsigned j = 0; j < 18; j++)
            BOOST_REQUIRE_EQUAL(world.CalcDistance(curPt, radiusPts[j + 19]), 3u);
    }
}

BOOST_AUTO_TEST_CASE(GetIdx)
{
    MapBase world;
    // Small world
    world.Resize(MapExtent(100, 50));
    BOOST_REQUIRE_EQUAL(world.GetIdx(MapPoint(0, 0)), 0u);
    BOOST_REQUIRE_EQUAL(world.GetIdx(MapPoint(99, 0)), 99u);
    BOOST_REQUIRE_EQUAL(world.GetIdx(MapPoint(0, 1)), 100u);
    BOOST_REQUIRE_EQUAL(world.GetIdx(MapPoint(1, 1)), 101u);
    BOOST_REQUIRE_EQUAL(world.GetIdx(MapPoint(99, 49)), 50u * 100u - 1u);
    // Big world. Index will exceed uint16_t
    world.Resize(MapExtent(0xFF00, 0xEEEE));
    BOOST_REQUIRE_EQUAL(world.GetIdx(MapPoint(0, 0)), 0u);
    BOOST_REQUIRE_EQUAL(world.GetIdx(MapPoint(0xFF00u - 1, 0)), 0xFF00u - 1);
    BOOST_REQUIRE_EQUAL(world.GetIdx(MapPoint(0, 1)), 0xFF00u);
    BOOST_REQUIRE_EQUAL(world.GetIdx(MapPoint(1, 1)), 0xFF01u);
    BOOST_REQUIRE_EQUAL(world.GetIdx(MapPoint(0x100, 1)), 0x10000u);
    BOOST_REQUIRE_EQUAL(world.GetIdx(MapPoint(0xFF00 - 1, 0xEEEE - 1)), 0xFF00u * 0xEEEEu - 1u);
}

/// HQ radius is 9 -> min size is 20
typedef WorldFixture<CreateEmptyWorld, 1, 20, 20> WorldFixtureEmpty1P;
BOOST_FIXTURE_TEST_CASE(HQPlacement, WorldFixtureEmpty1P)
{
    GamePlayer& player = world.GetPlayer(0);
    BOOST_REQUIRE(player.isUsed());
    const MapPoint hqPos = player.GetHQPos();
    BOOST_REQUIRE(hqPos.isValid());
    BOOST_REQUIRE_EQUAL(world.GetNO(player.GetHQPos())->GetGOT(), GOT_NOB_HQ);
    // Check ownership of points
    std::vector<MapPoint> ownerPts = world.GetPointsInRadius(hqPos, HQ_RADIUS);
    BOOST_FOREACH(MapPoint pt, ownerPts)
    {
        // This should be ensured by `GetPointsInRadius`
        BOOST_REQUIRE_LE(world.CalcDistance(pt, hqPos), HQ_RADIUS);
        // We must own this point
        BOOST_REQUIRE_EQUAL(world.GetNode(pt).owner, 1);
        // Points at radius are border nodes, others player territory
        if(world.CalcDistance(pt, hqPos) == HQ_RADIUS)
            BOOST_REQUIRE(world.IsBorderNode(pt, 1));
        else
            BOOST_REQUIRE(world.IsPlayerTerritory(pt));
    }
}

BOOST_AUTO_TEST_SUITE_END()
