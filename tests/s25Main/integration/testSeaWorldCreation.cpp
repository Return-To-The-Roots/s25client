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

#include "RTTR_AssertError.h"
#include "worldFixtures/SeaWorldWithGCExecution.h"
#include "gameTypes/GameTypesOutput.h"
#include "gameTypes/ShipDirection.h"
#include <rttr/test/LogAccessor.hpp>
#include <boost/test/unit_test.hpp>

// LCOV_EXCL_START
static std::ostream& operator<<(std::ostream& out, const ShipDirection& dir)
{
    return out << static_cast<int>(rttr::enum_cast(dir));
}
// LCOV_EXCL_STOP

BOOST_AUTO_TEST_SUITE(SeaWorldCreationSuite)

namespace {

/// Return the ship dir from a point to an other point given by their difference
ShipDirection getShipDir(const MapBase& world, MapPoint fromPt, const Position& diff)
{
    MapPoint toPt = world.MakeMapPoint(Position(fromPt) + diff);
    return world.GetShipDir(fromPt, toPt);
}

/// Test getting the ship dir for the various cases coming from a single point
void testShipDir(const MapBase& world, const MapPoint fromPt)
{
    using DiffPt = Position;
    // General cases
    BOOST_REQUIRE_EQUAL(getShipDir(world, fromPt, DiffPt(0, -10)), ShipDirection::North);
    BOOST_REQUIRE_EQUAL(getShipDir(world, fromPt, DiffPt(10, -1)), ShipDirection::NorthEast);
    BOOST_REQUIRE_EQUAL(getShipDir(world, fromPt, DiffPt(10, 1)), ShipDirection::SouthEast);
    BOOST_REQUIRE_EQUAL(getShipDir(world, fromPt, DiffPt(0, 10)), ShipDirection::South);
    BOOST_REQUIRE_EQUAL(getShipDir(world, fromPt, DiffPt(-10, 1)), ShipDirection::SouthWest);
    BOOST_REQUIRE_EQUAL(getShipDir(world, fromPt, DiffPt(-10, -1)), ShipDirection::NorthWest);

    // y diff is zero -> Go south (convention)
    BOOST_REQUIRE_EQUAL(getShipDir(world, fromPt, DiffPt(-10, 0)), ShipDirection::SouthWest);
    BOOST_REQUIRE_EQUAL(getShipDir(world, fromPt, DiffPt(10, 0)), ShipDirection::SouthEast);
    BOOST_REQUIRE_EQUAL(getShipDir(world, fromPt, DiffPt(1, 0)), ShipDirection::SouthEast);
    BOOST_REQUIRE_EQUAL(getShipDir(world, fromPt, DiffPt(-1, 0)), ShipDirection::SouthWest);

    // 6 directions -> 60deg covered per direction, mainDir +- 30deg
    // Switch pt between north and south is simple: Above or below zero diff (already tested above)
    // But S to SE or SW (same for N) is harder. Dividing line as an angle of +- 60deg compared to x-axis
    // hence: |dy/dx| > tan(60deg) -> SOUTH, tan(60deg) ~= 1.732. Test here with |dy| = |dx| * 1.732 as the divider
    BOOST_REQUIRE_EQUAL(getShipDir(world, fromPt, DiffPt(100, 173)), ShipDirection::SouthEast);
    // Switch point
    BOOST_REQUIRE_EQUAL(getShipDir(world, fromPt, DiffPt(100, 174)), ShipDirection::South);
    // Same for other 3 switch points
    BOOST_REQUIRE_EQUAL(getShipDir(world, fromPt, DiffPt(-100, 173)), ShipDirection::SouthWest);
    BOOST_REQUIRE_EQUAL(getShipDir(world, fromPt, DiffPt(-100, 174)), ShipDirection::South);
    BOOST_REQUIRE_EQUAL(getShipDir(world, fromPt, DiffPt(-100, -173)), ShipDirection::NorthWest);
    BOOST_REQUIRE_EQUAL(getShipDir(world, fromPt, DiffPt(-100, -174)), ShipDirection::North);
    BOOST_REQUIRE_EQUAL(getShipDir(world, fromPt, DiffPt(100, -173)), ShipDirection::NorthEast);
    BOOST_REQUIRE_EQUAL(getShipDir(world, fromPt, DiffPt(100, -174)), ShipDirection::North);
}
} // namespace

BOOST_AUTO_TEST_CASE(GetShipDir)
{
    // Width must be > 100*2 and Height > 174 * 2 to avoid wrapping errors (see testShipDir)
    MapBase world;
    world.Resize(MapExtent(202, 350));
    // Basic case
    testShipDir(world, MapPoint(world.GetWidth() / 2, world.GetHeight() / 2));
    // Left/Right border
    testShipDir(world, MapPoint(0, world.GetHeight() / 2));
    testShipDir(world, MapPoint(world.GetWidth() - 1, world.GetHeight() / 2));
    // Top/Bottom border
    testShipDir(world, MapPoint(world.GetWidth() / 2, 0));
    testShipDir(world, MapPoint(world.GetWidth() / 2, world.GetHeight() - 1));
    // Diagonal ends
    testShipDir(world, MapPoint(0, 0));
    testShipDir(world, MapPoint(world.GetWidth() - 1, world.GetHeight() - 1));
}

BOOST_FIXTURE_TEST_CASE(HarborSpotCreation, SeaWorldWithGCExecution<>)
{
    rttr::test::LogAccessor logAcc;
    // Point 0,0 is definitely inside the sea
    BOOST_REQUIRE(world.IsWaterPoint(MapPoint(0, 0)));
    BOOST_REQUIRE(world.IsSeaPoint(MapPoint(0, 0)));
    // 2 harbors for each of the 4 player spots
    BOOST_REQUIRE_EQUAL(world.GetNumHarborPoints(), 8u);
    // 2 seas: 1 outside, 1 inside
    BOOST_REQUIRE_EQUAL(world.GetNumSeas(), 2u);
// Harbor ID 0 is means invalid harbor
#if RTTR_ENABLE_ASSERTS
    RTTR_REQUIRE_ASSERT(world.GetHarborPoint(0));
#else
    BOOST_REQUIRE(!world.GetHarborPoint(0).isValid());
#endif
    // Note: Dummy harbor not counted
    for(unsigned curHarborId = 1; curHarborId <= world.GetNumHarborPoints(); curHarborId++)
    {
        const MapPoint curHarborPt = world.GetHarborPoint(curHarborId);
        BOOST_REQUIRE(curHarborPt.isValid());
        // Reverse mapping must match
        BOOST_REQUIRE_EQUAL(world.GetHarborPointID(curHarborPt), curHarborId);
        // We must be at one of the 2 seas
        unsigned seaId = 2;
        if(world.IsHarborAtSea(curHarborId, 1))
            seaId = 1;
        else
            BOOST_REQUIRE(world.IsHarborAtSea(curHarborId, 2));
        // We must have a coast point at that sea
        const MapPoint coastPt = world.GetCoastalPoint(curHarborId, seaId);
        BOOST_REQUIRE(coastPt.isValid());
        BOOST_REQUIRE_NE(world.GetSeaFromCoastalPoint(coastPt), 0);
        // Sea in the direction of the coast must match
        bool coastPtFound = false;
        for(const auto dir : helpers::EnumRange<Direction>{})
        {
            if(world.GetNeighbour(curHarborPt, dir) == coastPt)
            {
                BOOST_REQUIRE_EQUAL(world.GetSeaId(curHarborId, dir), seaId);
                coastPtFound = true;
                break;
            }
        }
        BOOST_REQUIRE(coastPtFound);
        BOOST_REQUIRE_EQUAL(world.GetNode(curHarborPt).bq, BuildingQuality::Harbor);
    }
}

BOOST_FIXTURE_TEST_CASE(HarborNeighbors, SeaWorldWithGCExecution<>)
{
    // Now just test some assumptions: 2 harbor spots per possible HQ.
    // Square land, 1 HQ on each side, harbors top and bottom or left and right of it
    // 1) Compare those around each HQ
    BOOST_REQUIRE_LT(world.GetHarborPoint(1).y, world.GetHarborPoint(2).y); //-V807
    BOOST_REQUIRE_LT(world.GetHarborPoint(7).y, world.GetHarborPoint(8).y);
    BOOST_REQUIRE_LT(world.GetHarborPoint(3).x, world.GetHarborPoint(4).x);
    BOOST_REQUIRE_LT(world.GetHarborPoint(5).x, world.GetHarborPoint(6).x); //-V807
    // 2) Compare between them
    BOOST_REQUIRE_LT(world.GetHarborPoint(2).y, world.GetHarborPoint(7).y);
    BOOST_REQUIRE_LT(world.GetHarborPoint(4).x, world.GetHarborPoint(5).x);
    BOOST_REQUIRE_LT(world.GetHarborPoint(3).x, world.GetHarborPoint(1).x);
    BOOST_REQUIRE_LT(world.GetHarborPoint(1).x, world.GetHarborPoint(5).x);

    // Test neighbors
    std::vector<HarborPos::Neighbor> nb;
    // Hb 1 (outside)
    nb = world.GetHarborNeighbors(1, ShipDirection::SouthWest);
    BOOST_REQUIRE_EQUAL(nb.size(), 1u);
    BOOST_CHECK_EQUAL(nb[0].id, 3u);
    nb = world.GetHarborNeighbors(1, ShipDirection::SouthEast);
    BOOST_REQUIRE_EQUAL(nb.size(), 1u);
    BOOST_CHECK_EQUAL(nb[0].id, 6u);
    nb = world.GetHarborNeighbors(1, ShipDirection::North);
    BOOST_REQUIRE_EQUAL(nb.size(), 1u);
    BOOST_CHECK_EQUAL(nb[0].id, 8u);
    BOOST_REQUIRE_EQUAL(world.GetHarborNeighbors(1, ShipDirection::South).size(), 0u);
    BOOST_REQUIRE_EQUAL(world.GetHarborNeighbors(1, ShipDirection::NorthEast).size(), 0u);
    BOOST_REQUIRE_EQUAL(world.GetHarborNeighbors(1, ShipDirection::NorthWest).size(), 0u);

    // Hb 7 (inside)
    nb = world.GetHarborNeighbors(7, ShipDirection::NorthWest);
    BOOST_REQUIRE_EQUAL(nb.size(), 1u);
    BOOST_CHECK_EQUAL(nb[0].id, 4u);
    nb = world.GetHarborNeighbors(7, ShipDirection::NorthEast);
    BOOST_REQUIRE_EQUAL(nb.size(), 1u);
    BOOST_CHECK_EQUAL(nb[0].id, 5u);
    nb = world.GetHarborNeighbors(7, ShipDirection::North);
    BOOST_REQUIRE_EQUAL(nb.size(), 1u);
    BOOST_CHECK_EQUAL(nb[0].id, 2u);
    BOOST_REQUIRE_EQUAL(world.GetHarborNeighbors(7, ShipDirection::South).size(), 0u);
    BOOST_REQUIRE_EQUAL(world.GetHarborNeighbors(7, ShipDirection::SouthEast).size(), 0u);
    BOOST_REQUIRE_EQUAL(world.GetHarborNeighbors(7, ShipDirection::SouthWest).size(), 0u);
}

BOOST_AUTO_TEST_SUITE_END()
