// Copyright (c) 2016 - 2020 Settlers Freaks (sf-team at siedler25.org)
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
#include "RttrForeachPt.h"
#include "helpers/EnumRange.h"
#include "worldFixtures/CreateEmptyWorld.h"
#include "worldFixtures/WorldFixture.h"
#include "gameTypes/FoWNode.h"
#include "gameTypes/GameTypesOutput.h"
#include <boost/test/unit_test.hpp>

namespace {
using WorldFixtureEmpty0P = WorldFixture<CreateEmptyWorld, 0>;
boost::test_tools::predicate_result boundaryStonesMatch(GameWorldGame& world,
                                                        const std::vector<BoundaryStones>& expected)
{
    world.RecalcBorderStones(Position(0, 0), Extent(world.GetSize()));
    RTTR_FOREACH_PT(MapPoint, world.GetSize())
    {
        const BoundaryStones& isValue = world.GetNode(pt).boundary_stones;
        const BoundaryStones& expectedValue = expected[world.GetIdx(pt)];
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
    std::vector<BoundaryStones> expectedBoundaryStones(world.GetWidth() * world.GetHeight());
    // Sanity check
    BOOST_TEST(boundaryStonesMatch(world, expectedBoundaryStones));
    expectedBoundaryStones[world.GetIdx(MapPoint(7, 5))][BorderStonePos::HalfSouthEast] = 1u;
    BOOST_TEST(boundaryStonesMatch(world, expectedBoundaryStones).message().str() == "0!=1 at (7, 5)[HalfSouthEast]");

    // Check some point in the middle and at 0,0 which causes wrapping
    for(const auto middlePt : {MapPoint(5, 5), MapPoint(0, 0)})
    {
        // Reset owner to 0 (None) and boundary stones to nothing
        RTTR_FOREACH_PT(MapPoint, world.GetSize())
        {
            world.SetOwner(pt, 0);
            std::fill(expectedBoundaryStones[world.GetIdx(pt)].begin(), expectedBoundaryStones[world.GetIdx(pt)].end(),
                      0u);
        }
        // Get the minimum possible region where border stones would be placed
        const auto radius1Pts = helpers::toEnumArray<Direction>(world.GetNeighbours(middlePt));
        // Set only the middle pt and recalc
        world.SetOwner(middlePt, 1);
        // Only middle pt has a single boundary stone
        expectedBoundaryStones[world.GetIdx(middlePt)][BorderStonePos::OnPoint] = 1u;
        BOOST_REQUIRE(boundaryStonesMatch(world, expectedBoundaryStones));

        for(MapPoint pt : radius1Pts)
            world.SetOwner(pt, 1);
        // Midle pt lost its stone
        expectedBoundaryStones[world.GetIdx(middlePt)][BorderStonePos::OnPoint] = 0u;
        // Each border node should have a boundary stone at the center
        for(MapPoint pt : radius1Pts)
            expectedBoundaryStones[world.GetIdx(pt)][BorderStonePos::OnPoint] = 1u;
        expectedBoundaryStones[world.GetIdx(radius1Pts[Direction::WEST])][BorderStonePos::HalfSouthEast] = 1u;
        expectedBoundaryStones[world.GetIdx(radius1Pts[Direction::NORTHWEST])][BorderStonePos::HalfEast] = 1u;
        expectedBoundaryStones[world.GetIdx(radius1Pts[Direction::NORTHWEST])][BorderStonePos::HalfSouthWest] = 1u;
        expectedBoundaryStones[world.GetIdx(radius1Pts[Direction::NORTHEAST])][BorderStonePos::HalfSouthEast] = 1u;
        expectedBoundaryStones[world.GetIdx(radius1Pts[Direction::EAST])][BorderStonePos::HalfSouthWest] = 1u;
        // SE has no other stone
        expectedBoundaryStones[world.GetIdx(radius1Pts[Direction::SOUTHWEST])][BorderStonePos::HalfEast] = 1u;
        BOOST_REQUIRE(boundaryStonesMatch(world, expectedBoundaryStones));

        // Now obtain another node:
        const MapPoint doubleWestPt = world.GetNeighbour(radius1Pts[Direction::WEST], Direction::WEST);
        world.SetOwner(doubleWestPt, 1);
        // Still the same, but that node has 2 stones
        expectedBoundaryStones[world.GetIdx(doubleWestPt)][BorderStonePos::OnPoint] = 1u;
        expectedBoundaryStones[world.GetIdx(doubleWestPt)][BorderStonePos::HalfEast] = 1u;
        BOOST_REQUIRE(boundaryStonesMatch(world, expectedBoundaryStones));

        // Next, actually increasing our territory
        // Note: The NW node and its WEST neighbour have (temporarly) 3 neigbouring stones leading to issue #538 where
        // the half-way stone to E gets removed with prevent-blocking enabled
        const MapPoint doubleWestPt2 = world.GetNeighbour(doubleWestPt, Direction::NORTHEAST);
        const MapPoint doubleWestPt3 = world.GetNeighbour(doubleWestPt, Direction::SOUTHEAST);
        world.SetOwner(doubleWestPt2, 1);
        world.SetOwner(doubleWestPt3, 1);
        // New points get some stones: Top
        expectedBoundaryStones[world.GetIdx(doubleWestPt2)][BorderStonePos::OnPoint] = 1u;
        expectedBoundaryStones[world.GetIdx(doubleWestPt2)][BorderStonePos::HalfEast] = 1u;
        expectedBoundaryStones[world.GetIdx(doubleWestPt2)][BorderStonePos::HalfSouthWest] = 1u;
        // Bottom
        expectedBoundaryStones[world.GetIdx(doubleWestPt3)][BorderStonePos::OnPoint] = 1u;
        expectedBoundaryStones[world.GetIdx(doubleWestPt3)][BorderStonePos::HalfEast] = 1u;
        // Middle (gets one to bottom, looses one to right, as WEST point is now no border node anymore (affects 2 more)
        expectedBoundaryStones[world.GetIdx(doubleWestPt)][BorderStonePos::HalfSouthEast] = 1u;
        expectedBoundaryStones[world.GetIdx(doubleWestPt)][BorderStonePos::HalfEast] = 0u;
        expectedBoundaryStones[world.GetIdx(radius1Pts[Direction::WEST])][BorderStonePos::OnPoint] = 0u;
        expectedBoundaryStones[world.GetIdx(radius1Pts[Direction::WEST])][BorderStonePos::HalfSouthEast] = 0u;
        expectedBoundaryStones[world.GetIdx(radius1Pts[Direction::NORTHWEST])][BorderStonePos::HalfSouthWest] = 0u;
        BOOST_REQUIRE(boundaryStonesMatch(world, expectedBoundaryStones));
    }
}
