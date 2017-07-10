// Copyright (c) 2016 Settlers Freaks (sf-team at siedler25.org)
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
#include "WorldFixture.h"
#include "CreateEmptyWorld.h"
#include "GamePlayer.h"
#include "nodeObjs/noBase.h"
#include "test/PointOutput.h"
#include <boost/test/unit_test.hpp>
#include <boost/foreach.hpp>
#include <boost/assign/std/vector.hpp>
#include "world/MapGeometry.h"

BOOST_AUTO_TEST_SUITE(WorldCreationSuite)

typedef WorldFixture<CreateEmptyWorld, 0, 20, 20> WorldFixtureEmpty0P;
typedef WorldFixture<CreateEmptyWorld, 1, 20, 20> WorldFixtureEmpty1P;

BOOST_FIXTURE_TEST_CASE(NeighbourPts, WorldFixtureEmpty0P)
{
    using namespace boost::assign;

    typedef Point<int> PointI;
    /*  Note that every 2nd row is shifted by half a triangle to the left, therefore:
    Modifications for the dirs: */
    std::vector<PointI> evenPtMod(6);
    std::vector<PointI> oddPtMod(6);
    // In x dir there is no difference
    evenPtMod[Direction::WEST]      = PointI(-1,  0);
    oddPtMod [Direction::WEST]      = PointI(-1,  0);
    evenPtMod[Direction::EAST]      = PointI( 1,  0);
    oddPtMod [Direction::EAST]      = PointI( 1,  0);
    // To north we decrease y and may change x. x changed in NW <=> not changed in NE
    evenPtMod[Direction::NORTHWEST] = PointI(-1, -1);
    oddPtMod [Direction::NORTHWEST] = PointI( 0, -1);
    evenPtMod[Direction::NORTHEAST] = PointI( 0, -1);
    oddPtMod [Direction::NORTHEAST] = PointI( 1, -1);
    // And similar for south. X offsets stay the same!
    evenPtMod[Direction::SOUTHWEST] = PointI(-1,  1);
    oddPtMod [Direction::SOUTHWEST] = PointI( 0,  1);
    evenPtMod[Direction::SOUTHEAST] = PointI( 0,  1);
    oddPtMod [Direction::SOUTHEAST] = PointI( 1,  1);
    std::vector<PointI> testPoints;
    // Test a simple even and odd point
    testPoints += PointI(10, 10), PointI(10, 9);
    // Test border points
    testPoints += PointI(0, 0), PointI(0, world.GetHeight() - 1), PointI(world.GetWidth() - 1, 0), PointI(world.GetWidth() - 1, world.GetHeight() - 1);
    // Test border points with 1 offset in Y
    testPoints += PointI(0, 1), PointI(0, world.GetHeight() - 2), PointI(world.GetWidth() - 1, 1), PointI(world.GetWidth() - 1, world.GetHeight() - 2);
    BOOST_FOREACH(const PointI& pt, testPoints)
    {
        for(unsigned dir = 0; dir < Direction::COUNT; dir++)
        {
            const bool isEvenRow = pt.y % 2 == 0;
            const PointI targetPointRaw = pt + (isEvenRow ? evenPtMod : oddPtMod)[dir];
            const MapPoint targetPoint((targetPointRaw.x + world.GetWidth()) % world.GetWidth(), (targetPointRaw.y + world.GetHeight()) % world.GetHeight());
            BOOST_REQUIRE_EQUAL(world.CalcDistance(MapPoint(pt), targetPoint), 1u);

            BOOST_REQUIRE_EQUAL(world.GetNeighbour(MapPoint(pt), Direction::fromInt(dir)), targetPoint);
            // Consistency check: The inverse must also match
            BOOST_REQUIRE_EQUAL(world.GetNeighbour(targetPoint, Direction(dir + 3)), MapPoint(pt));
            // Also the global function must return the same:
            BOOST_REQUIRE_EQUAL(::GetNeighbour(PointI(pt), Direction::fromInt(dir)), targetPointRaw);
            BOOST_REQUIRE_EQUAL(world.MakeMapPoint(::GetNeighbour(PointI(pt), Direction::fromInt(dir))), targetPoint);
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
