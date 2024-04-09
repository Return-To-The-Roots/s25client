// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "PointOutput.h"
#include "enum_cast.hpp"
#include "helpers/EnumArray.h"
#include "world/MapBase.h"
#include "world/MapGeometry.h"
#include "gameData/MapConsts.h"
#include <rttr/test/random.hpp>
#include <boost/test/unit_test.hpp>
#include <array>

BOOST_AUTO_TEST_SUITE(WorldCreationSuite)

BOOST_AUTO_TEST_CASE(GetAllNeighboursUnion)
{
    MapBase world;

    // No points -> empty result
    BOOST_TEST(world.GetAllNeighboursUnion(std::vector<MapPoint>{}).empty());

    // Two compontents of 1 and 2 vertices
    const std::vector<MapPoint> testPoints{MapPoint(1, 1), MapPoint(10, 10), MapPoint(10, 11)};
    // ((center + hexagon (6 points)) * 3 points input) - 4 common points = 17 points
    const std::vector<MapPoint> expectedResultPoints{
      // Original point
      MapPoint(1, 1),
      // Neighbours
      MapPoint(1, 0), MapPoint(2, 0), MapPoint(0, 1), MapPoint(2, 1), MapPoint(1, 2), MapPoint(2, 2),
      // Originals points
      MapPoint(10, 10), // Duplicate
      MapPoint(10, 11), // Duplicate
      // Neighbours
      MapPoint(9, 9), MapPoint(10, 9), MapPoint(9, 10), MapPoint(11, 11), MapPoint(10, 12), MapPoint(11, 12),
      // Duplicates
      MapPoint(11, 10), MapPoint(9, 11)};

    const auto resultPoints = world.GetAllNeighboursUnion(testPoints);

    BOOST_TEST(resultPoints == expectedResultPoints, boost::test_tools::per_element());
}

BOOST_AUTO_TEST_CASE(NeighbourPts)
{
    MapBase world;
    world.Resize(MapExtent(20, 30));

    /*  Note that every 2nd row is shifted by half a triangle to the left, therefore:
    Modifications for the dirs: */
    helpers::EnumArray<Position, Direction> evenPtMod, oddPtMod;
    // In x dir there is no difference
    evenPtMod[Direction::West] = Position(-1, 0);
    oddPtMod[Direction::West] = Position(-1, 0);
    evenPtMod[Direction::East] = Position(1, 0);
    oddPtMod[Direction::East] = Position(1, 0);
    // To north we decrease y and may change x. x changed in NW <=> not changed in NE
    evenPtMod[Direction::NorthWest] = Position(-1, -1);
    oddPtMod[Direction::NorthWest] = Position(0, -1);
    evenPtMod[Direction::NorthEast] = Position(0, -1);
    oddPtMod[Direction::NorthEast] = Position(1, -1);
    // And similar for south. X offsets stay the same!
    evenPtMod[Direction::SouthWest] = Position(-1, 1);
    oddPtMod[Direction::SouthWest] = Position(0, 1);
    evenPtMod[Direction::SouthEast] = Position(0, 1);
    oddPtMod[Direction::SouthEast] = Position(1, 1);
    std::vector<MapPoint> testPoints{
      // Test a simple even and odd point
      MapPoint(10, 10), MapPoint(10, 9),
      // Test border points
      MapPoint(0, 0), MapPoint(0, world.GetHeight() - 1), MapPoint(world.GetWidth() - 1, 0),
      MapPoint(world.GetWidth() - 1, world.GetHeight() - 1),
      // Test border points with 1 offset in Y
      MapPoint(0, 1), MapPoint(0, world.GetHeight() - 2), MapPoint(world.GetWidth() - 1, 1),
      MapPoint(world.GetWidth() - 1, world.GetHeight() - 2)};
    for(const MapPoint& pt : testPoints)
    {
        for(const auto dir : helpers::EnumRange<Direction>{})
        {
            const bool isEvenRow = pt.y % 2 == 0;
            const Position targetPointRaw = pt + (isEvenRow ? evenPtMod : oddPtMod)[dir];
            const MapPoint targetPoint((targetPointRaw.x + world.GetWidth()) % world.GetWidth(),
                                       (targetPointRaw.y + world.GetHeight()) % world.GetHeight());
            BOOST_TEST_REQUIRE(world.CalcDistance(pt, targetPoint) == 1u);

            BOOST_TEST_REQUIRE(world.GetNeighbour(pt, dir) == targetPoint);
            // Consistency check: The inverse must also match
            BOOST_TEST_REQUIRE(world.GetNeighbour(targetPoint, Direction(dir + 3)) == pt);
            // Also the global function must return the same:
            BOOST_TEST_REQUIRE(::GetNeighbour(Position(pt), dir) == targetPointRaw);
            BOOST_TEST_REQUIRE(world.MakeMapPoint(::GetNeighbour(Position(pt), dir)) == targetPoint);
        }

        // Neighbour 2 -> Radius 2, right circle
        MapPoint curTargetPoint = world.GetNeighbour(world.GetNeighbour(pt, Direction::West), Direction::West);
        BOOST_TEST_REQUIRE(world.CalcDistance(pt, curTargetPoint) == 2u);
        BOOST_TEST_REQUIRE(world.GetNeighbour2(pt, 0) == curTargetPoint);
        // We now go 2 steps in each direction to describe the circle
        // (Note: Could iterator over directions, but to easily identify errors and make intentions clear we do it
        // explicitely)
        const std::array<Direction, 11> steps{Direction::NorthEast, Direction::NorthEast, Direction::East,
                                              Direction::East,      Direction::SouthEast, Direction::SouthEast,
                                              Direction::SouthWest, Direction::SouthWest, Direction::West,
                                              Direction::West,      Direction::NorthWest};
        // Note: 0==12 already handled
        for(unsigned j = 1; j < 12; j++)
        {
            curTargetPoint = world.GetNeighbour(curTargetPoint, steps.at(j - 1));
            BOOST_TEST_REQUIRE(world.CalcDistance(pt, curTargetPoint) == 2u);
            BOOST_TEST_REQUIRE(world.GetNeighbour2(pt, j) == curTargetPoint);
        }

        // And finally the points in radius
        std::vector<MapPoint> radiusPts = world.GetPointsInRadiusWithCenter(pt, 3);
        BOOST_TEST_REQUIRE(radiusPts.size() == 1u + 6u + 12u + 18u);
        BOOST_TEST_REQUIRE(radiusPts[0] == pt);
        for(const auto j : helpers::EnumRange<Direction>{})
            BOOST_TEST_REQUIRE(radiusPts[rttr::enum_cast(j) + 1] == world.GetNeighbour(pt, j));
        for(unsigned j = 0; j < 12; j++)
            BOOST_TEST_REQUIRE(radiusPts[j + 7] == world.GetNeighbour2(pt, j));
        for(unsigned j = 0; j < 18; j++)
            BOOST_TEST_REQUIRE(world.CalcDistance(pt, radiusPts[j + 19]) == 3u);

        const helpers::EnumArray<MapPoint, Direction> neighbours = world.GetNeighbours(pt);
        for(const auto dir : helpers::EnumRange<Direction>{})
            BOOST_TEST(neighbours[dir] == world.GetNeighbour(pt, dir));
    }
}

BOOST_AUTO_TEST_CASE(GetMatchingPointsInRadius)
{
    MapBase world;
    world.Resize(MapExtent(20, 30));
    const MapPoint center = rttr::test::randomPoint<MapPoint>(0, 19);
    const std::vector<MapPoint> oddPts =
      world.GetMatchingPointsInRadius(center, 5, [](const MapPoint pt) { return pt.x % 2 == 1; });
    const std::vector<MapPoint> evenPts =
      world.GetMatchingPointsInRadius(center, 5, [](const MapPoint pt) { return pt.x % 2 == 0; });
    BOOST_TEST_REQUIRE(oddPts.size() + evenPts.size() == 6u + 12u + 18u + 24u + 30u);
    for(const MapPoint pt : oddPts)
        BOOST_TEST(pt.x % 2 == 1);
    for(const MapPoint pt : evenPts)
        BOOST_TEST(pt.x % 2 == 0);
    const std::vector<MapPoint> firstEvenPt =
      world.GetMatchingPointsInRadius<1>(center, 5, [](const MapPoint pt) { return pt.x % 2 == 0; });
    BOOST_TEST_REQUIRE(firstEvenPt.size() == 1u);
    BOOST_TEST(firstEvenPt.front() == evenPts.front());
}

BOOST_AUTO_TEST_CASE(GetIdx)
{
    MapBase world;
    // Small world
    world.Resize(MapExtent(100, 50));
    BOOST_TEST_REQUIRE(world.GetIdx(MapPoint(0, 0)) == 0u);
    BOOST_TEST_REQUIRE(world.GetIdx(MapPoint(99, 0)) == 99u);
    BOOST_TEST_REQUIRE(world.GetIdx(MapPoint(0, 1)) == 100u);
    BOOST_TEST_REQUIRE(world.GetIdx(MapPoint(1, 1)) == 101u);
    BOOST_TEST_REQUIRE(world.GetIdx(MapPoint(99, 49)) == 50u * 100u - 1u);
    // Big world.
    world.Resize(MapExtent(MAX_MAP_SIZE, MAX_MAP_SIZE - 2));
    BOOST_TEST_REQUIRE(world.GetIdx(MapPoint(0, 0)) == 0u);
    BOOST_TEST_REQUIRE(world.GetIdx(MapPoint(MAX_MAP_SIZE - 1, 0)) == MAX_MAP_SIZE - 1);
    BOOST_TEST_REQUIRE(world.GetIdx(MapPoint(0, 1)) == MAX_MAP_SIZE);
    BOOST_TEST_REQUIRE(world.GetIdx(MapPoint(1, 1)) == MAX_MAP_SIZE + 1u);
    BOOST_TEST_REQUIRE(world.GetIdx(MapPoint(MAX_MAP_SIZE - 1, MAX_MAP_SIZE - 3))
                       == MAX_MAP_SIZE * (MAX_MAP_SIZE - 2u) - 1u);
}

BOOST_AUTO_TEST_SUITE_END()
