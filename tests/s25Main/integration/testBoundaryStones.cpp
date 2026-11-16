// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "PointOutput.h"
#include "RttrForeachPt.h"
#include "helpers/EnumRange.h"
#include "worldFixtures/CreateEmptyWorld.h"
#include "worldFixtures/WorldFixture.h"
#include "gameTypes/FoWNode.h"
#include "gameTypes/GameTypesOutput.h"
#include <boost/test/unit_test.hpp>

namespace {
using WorldFixtureEmpty0P = WorldFixture<CreateEmptyWorld, 0>;
boost::test_tools::predicate_result boundaryStonesMatch(GameWorld& world, const std::vector<BoundaryStones>& expected)
{
    world.RecalcBorderStones(Position(0, 0), Extent(world.getSize()));
    RTTR_FOREACH_PT(MapPoint, world.getSize())
    {
        const BoundaryStones& isValue = world.getNode(pt).boundary_stones;
        const BoundaryStones& expectedValue = expected[world.getIdx(pt)];
        for(const auto bPos : helpers::EnumRange<BorderStonePos>{})
        {
            if(isValue[bPos] != expectedValue[bPos])
            {
                boost::test_tools::predicate_result result(false);
                result.message() << unsigned(isValue[bPos]) << "!=" << unsigned(expectedValue[bPos]) << " at " << pt
                                 << "[" << bPos << "]";
                return result;
            }
        }
    }
    return true;
}
} // namespace

BOOST_FIXTURE_TEST_CASE(BorderStones, WorldFixtureEmpty0P)
{
    std::vector<BoundaryStones> expectedBoundaryStones(world.getWidth() * world.getHeight());
    // Sanity check
    BOOST_TEST(boundaryStonesMatch(world, expectedBoundaryStones));
    expectedBoundaryStones[world.getIdx(MapPoint(7, 5))][BorderStonePos::HalfSouthEast] = 1u;
    BOOST_TEST(boundaryStonesMatch(world, expectedBoundaryStones).message().str()
               == "0!=1 at (7, 5)[BorderStonePos::HalfSouthEast]");

    // Check some point in the middle and at 0,0 which causes wrapping
    for(const auto middlePt : {MapPoint(5, 5), MapPoint(0, 0)})
    {
        // Reset owner to 0 (None) and boundary stones to nothing
        RTTR_FOREACH_PT(MapPoint, world.getSize())
        {
            world.setOwner(pt, 0);
            std::fill(expectedBoundaryStones[world.getIdx(pt)].begin(), expectedBoundaryStones[world.getIdx(pt)].end(),
                      0u);
        }
        // Get the minimum possible region where border stones would be placed
        const auto radius1Pts = world.getNeighbours(middlePt);
        // Set only the middle pt and recalc
        world.setOwner(middlePt, 1);
        // Only middle pt has a single boundary stone
        expectedBoundaryStones[world.getIdx(middlePt)][BorderStonePos::OnPoint] = 1u;
        BOOST_TEST_REQUIRE(boundaryStonesMatch(world, expectedBoundaryStones));

        for(MapPoint pt : radius1Pts)
            world.setOwner(pt, 1);
        // Midle pt lost its stone
        expectedBoundaryStones[world.getIdx(middlePt)][BorderStonePos::OnPoint] = 0u;
        // Each border node should have a boundary stone at the center
        for(MapPoint pt : radius1Pts)
            expectedBoundaryStones[world.getIdx(pt)][BorderStonePos::OnPoint] = 1u;
        expectedBoundaryStones[world.getIdx(radius1Pts[Direction::West])][BorderStonePos::HalfSouthEast] = 1u;
        expectedBoundaryStones[world.getIdx(radius1Pts[Direction::NorthWest])][BorderStonePos::HalfEast] = 1u;
        expectedBoundaryStones[world.getIdx(radius1Pts[Direction::NorthWest])][BorderStonePos::HalfSouthWest] = 1u;
        expectedBoundaryStones[world.getIdx(radius1Pts[Direction::NorthEast])][BorderStonePos::HalfSouthEast] = 1u;
        expectedBoundaryStones[world.getIdx(radius1Pts[Direction::East])][BorderStonePos::HalfSouthWest] = 1u;
        // SE has no other stone
        expectedBoundaryStones[world.getIdx(radius1Pts[Direction::SouthWest])][BorderStonePos::HalfEast] = 1u;
        BOOST_TEST_REQUIRE(boundaryStonesMatch(world, expectedBoundaryStones));

        // Now obtain another node:
        const MapPoint doubleWestPt = world.getNeighbour(radius1Pts[Direction::West], Direction::West);
        world.setOwner(doubleWestPt, 1);
        // Still the same, but that node has 2 stones
        expectedBoundaryStones[world.getIdx(doubleWestPt)][BorderStonePos::OnPoint] = 1u;
        expectedBoundaryStones[world.getIdx(doubleWestPt)][BorderStonePos::HalfEast] = 1u;
        BOOST_TEST_REQUIRE(boundaryStonesMatch(world, expectedBoundaryStones));

        // Next, actually increasing our territory
        // Note: The NW node and its West neighbour have (temporarly) 3 neigbouring stones leading to issue #538 where
        // the half-way stone to E gets removed with prevent-blocking enabled
        const MapPoint doubleWestPt2 = world.getNeighbour(doubleWestPt, Direction::NorthEast);
        const MapPoint doubleWestPt3 = world.getNeighbour(doubleWestPt, Direction::SouthEast);
        world.setOwner(doubleWestPt2, 1);
        world.setOwner(doubleWestPt3, 1);
        // New points get some stones: Top
        expectedBoundaryStones[world.getIdx(doubleWestPt2)][BorderStonePos::OnPoint] = 1u;
        expectedBoundaryStones[world.getIdx(doubleWestPt2)][BorderStonePos::HalfEast] = 1u;
        expectedBoundaryStones[world.getIdx(doubleWestPt2)][BorderStonePos::HalfSouthWest] = 1u;
        // Bottom
        expectedBoundaryStones[world.getIdx(doubleWestPt3)][BorderStonePos::OnPoint] = 1u;
        expectedBoundaryStones[world.getIdx(doubleWestPt3)][BorderStonePos::HalfEast] = 1u;
        // Middle (gets one to bottom, looses one to right, as West point is now no border node anymore (affects 2 more)
        expectedBoundaryStones[world.getIdx(doubleWestPt)][BorderStonePos::HalfSouthEast] = 1u;
        expectedBoundaryStones[world.getIdx(doubleWestPt)][BorderStonePos::HalfEast] = 0u;
        expectedBoundaryStones[world.getIdx(radius1Pts[Direction::West])][BorderStonePos::OnPoint] = 0u;
        expectedBoundaryStones[world.getIdx(radius1Pts[Direction::West])][BorderStonePos::HalfSouthEast] = 0u;
        expectedBoundaryStones[world.getIdx(radius1Pts[Direction::NorthWest])][BorderStonePos::HalfSouthWest] = 0u;
        BOOST_TEST_REQUIRE(boundaryStonesMatch(world, expectedBoundaryStones));
    }
}
