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
#include "SeaWorldWithGCExecution.h"
#include "RTTR_AssertError.h"
#include <boost/test/unit_test.hpp>

std::ostream& operator<<(std::ostream &out, const ShipDirection& dir)
{
    return out << dir.toUInt();
}

BOOST_AUTO_TEST_SUITE(SeaWorldCreationSuite)

namespace{
    /// Create a world, that has just default initialized nodes
    struct CreateDummyWorld
    {
        CreateDummyWorld(unsigned width, unsigned height, unsigned numPlayers):
            width_(width), height_(height)
        {}
        bool operator()(GameWorldGame& world) const
        {
            world.Init(width_, height_, LT_GREENLAND);
            return true;
        }
    private:
        unsigned width_, height_;
    };
    typedef WorldFixture<CreateDummyWorld, 0, 512, 512> DummyWorldFixture;

    /// Return the ship dir from a point to an other point given by their difference
    ShipDirection getShipDir(const World& world, MapPoint fromPt, const Point<int>& diff)
    {
        MapPoint toPt = world.MakeMapPoint(Point<int>(fromPt) + diff);
        return world.GetShipDir(fromPt, toPt);
    }

    /// Test getting the ship dir for the various cases coming from a single point
    void testShipDir(const World& world, const MapPoint fromPt)
    {
        typedef Point<int> DiffPt;
        // General cases
        BOOST_REQUIRE_EQUAL(getShipDir(world, fromPt, DiffPt(0, -10)), ShipDirection::NORTH);
        BOOST_REQUIRE_EQUAL(getShipDir(world, fromPt, DiffPt(10, -1)), ShipDirection::NORTHEAST);
        BOOST_REQUIRE_EQUAL(getShipDir(world, fromPt, DiffPt(10, 1)), ShipDirection::SOUTHEAST);
        BOOST_REQUIRE_EQUAL(getShipDir(world, fromPt, DiffPt(0, 10)), ShipDirection::SOUTH);
        BOOST_REQUIRE_EQUAL(getShipDir(world, fromPt, DiffPt(-10, 1)), ShipDirection::SOUTHWEST);
        BOOST_REQUIRE_EQUAL(getShipDir(world, fromPt, DiffPt(-10, -1)), ShipDirection::NORTHWEST);

        // y diff is zero -> Go south (convention)
        BOOST_REQUIRE_EQUAL(getShipDir(world, fromPt, DiffPt(-10, 0)), ShipDirection::SOUTHWEST);
        BOOST_REQUIRE_EQUAL(getShipDir(world, fromPt, DiffPt(10, 0)), ShipDirection::SOUTHEAST);
        BOOST_REQUIRE_EQUAL(getShipDir(world, fromPt, DiffPt(1, 0)), ShipDirection::SOUTHEAST);
        BOOST_REQUIRE_EQUAL(getShipDir(world, fromPt, DiffPt(-1, 0)), ShipDirection::SOUTHWEST);

        // 6 directions -> 60deg covered per direction, mainDir +- 30deg
        // Switch pt between north and south is simple: Above or below zero diff (already tested above)
        // But S to SE or SW (same for N) is harder. Dividing line as an angle of +- 60deg compared to x-axis
        // hence: |dy/dx| > tan(60deg) -> SOUTH, tan(60deg) ~= 1.732. Test here with |dy| = |dx| * 1.732 as the divider
        BOOST_REQUIRE_EQUAL(getShipDir(world, fromPt, DiffPt(100, 173)), ShipDirection::SOUTHEAST);
        // Switch point
        BOOST_REQUIRE_EQUAL(getShipDir(world, fromPt, DiffPt(100, 174)), ShipDirection::SOUTH);
        // Same for other 3 switch points
        BOOST_REQUIRE_EQUAL(getShipDir(world, fromPt, DiffPt(-100, 173)), ShipDirection::SOUTHWEST);
        BOOST_REQUIRE_EQUAL(getShipDir(world, fromPt, DiffPt(-100, 174)), ShipDirection::SOUTH);
        BOOST_REQUIRE_EQUAL(getShipDir(world, fromPt, DiffPt(-100, -173)), ShipDirection::NORTHWEST);
        BOOST_REQUIRE_EQUAL(getShipDir(world, fromPt, DiffPt(-100, -174)), ShipDirection::NORTH);
        BOOST_REQUIRE_EQUAL(getShipDir(world, fromPt, DiffPt(100, -173)), ShipDirection::NORTHEAST);
        BOOST_REQUIRE_EQUAL(getShipDir(world, fromPt, DiffPt(100, -174)), ShipDirection::NORTH);
    }
}

BOOST_FIXTURE_TEST_CASE(GetShipDir, DummyWorldFixture)
{
    // Basic case
    testShipDir(this->world, MapPoint(world.GetWidth() / 2, world.GetHeight() / 2));
    // Left/Right border
    testShipDir(this->world, MapPoint(0, world.GetHeight() / 2));
    testShipDir(this->world, MapPoint(world.GetWidth() - 1, world.GetHeight() / 2));
    // Top/Bottom border
    testShipDir(this->world, MapPoint(world.GetWidth() / 2, 0));
    testShipDir(this->world, MapPoint(world.GetWidth() / 2, world.GetHeight() - 1));
    // Diagonal ends
    testShipDir(this->world, MapPoint(0, 0));
    testShipDir(this->world, MapPoint(world.GetWidth() - 1, world.GetHeight() - 1));
}

BOOST_FIXTURE_TEST_CASE(HarborSpotCreation, SeaWorldWithGCExecution)
{
    // Point 0,0 is definitely inside the sea
    BOOST_REQUIRE(world.IsWaterPoint(MapPoint(0, 0)));
    BOOST_REQUIRE(world.IsSeaPoint(MapPoint(0, 0)));
    // 2 harbors for each of the 4 player spots
    BOOST_REQUIRE_EQUAL(world.GetHarborPointCount(), 8u);
    // 2 seas: 1 outside, 1 inside
    BOOST_REQUIRE_EQUAL(world.GetNumSeas(), 2u);
    // Harbor ID 0 is means invalid harbor
#if RTTR_ENABLE_ASSERTS
    RTTR_AssertEnableBreak = false;
    BOOST_REQUIRE_THROW(world.GetHarborPoint(0), RTTR_AssertError);
    RTTR_AssertEnableBreak = true;
#else
    BOOST_REQUIRE(!world.GetHarborPoint(0).isValid());
#endif
    // Note: Dummy harbor not counted
    for(unsigned curHarborId = 1; curHarborId <= world.GetHarborPointCount(); curHarborId++)
    {
        const MapPoint curHarborPt = world.GetHarborPoint(curHarborId);
        BOOST_REQUIRE(curHarborPt.isValid());
        // Reverse mapping must match
        BOOST_REQUIRE_EQUAL(world.GetHarborPointID(curHarborPt), curHarborId);
        // We must be at one of the 2 seas
        unsigned seaId = 0;
        if(world.IsHarborAtSea(curHarborId, 1))
            seaId = 1;
        else if(world.IsHarborAtSea(curHarborId, 2))
            seaId = 2;
        else
            BOOST_FAIL("Harbor is at no sea");
        // We must have a coast point at that sea
        const MapPoint coastPt = world.GetCoastalPoint(curHarborId, seaId);
        BOOST_REQUIRE(coastPt.isValid());
        BOOST_REQUIRE_NE(world.GetSeaFromCoastalPoint(coastPt), 0);
        // Sea in the direction of the coast must match
        bool coastPtFound = false;
        for(unsigned dir = 0; dir < 6; dir++)
        {
            if(world.GetNeighbour(curHarborPt, Direction::fromInt(dir)) == coastPt)
            {
                BOOST_REQUIRE_EQUAL(world.GetSeaId(curHarborId, Direction::fromInt(dir)), seaId);
                coastPtFound = true;
                break;
            }
        }
        BOOST_REQUIRE(coastPtFound);
        BOOST_REQUIRE_EQUAL(world.GetNode(curHarborPt).bq, BQ_HARBOR);
    }
}

BOOST_FIXTURE_TEST_CASE(HarborNeighbors, SeaWorldWithGCExecution)
{
    // Now just test some assumptions: 2 harbor spots per possible HQ.
    // Square land, 1 HQ on each side, harbors top and bottom or left and right of it
    // 1) Compare those around each HQ
    BOOST_REQUIRE_LT(world.GetHarborPoint(1).y, world.GetHarborPoint(2).y);
    BOOST_REQUIRE_LT(world.GetHarborPoint(7).y, world.GetHarborPoint(8).y);
    BOOST_REQUIRE_LT(world.GetHarborPoint(3).x, world.GetHarborPoint(4).x);
    BOOST_REQUIRE_LT(world.GetHarborPoint(5).x, world.GetHarborPoint(6).x);
    // 2) Compare betweem them
    BOOST_REQUIRE_LT(world.GetHarborPoint(2).y, world.GetHarborPoint(7).y);
    BOOST_REQUIRE_LT(world.GetHarborPoint(4).x, world.GetHarborPoint(5).x);
    BOOST_REQUIRE_LT(world.GetHarborPoint(3).x, world.GetHarborPoint(1).x);
    BOOST_REQUIRE_LT(world.GetHarborPoint(1).x, world.GetHarborPoint(5).x);

    // Test neighbors
    std::vector<HarborPos::Neighbor> nb;
    // Hb 1 (outside)
    nb = world.GetHarborNeighbor(1, ShipDirection::SOUTHWEST);
    BOOST_REQUIRE_EQUAL(nb.size(), 1u);
    BOOST_CHECK_EQUAL(nb[0].id, 3u);
    nb = world.GetHarborNeighbor(1, ShipDirection::SOUTHEAST);
    BOOST_REQUIRE_EQUAL(nb.size(), 1u);
    BOOST_CHECK_EQUAL(nb[0].id, 6u);
    nb = world.GetHarborNeighbor(1, ShipDirection::NORTH);
    BOOST_REQUIRE_EQUAL(nb.size(), 1u);
    BOOST_CHECK_EQUAL(nb[0].id, 8u);
    BOOST_REQUIRE_EQUAL(world.GetHarborNeighbor(1, ShipDirection::SOUTH).size(), 0u);
    BOOST_REQUIRE_EQUAL(world.GetHarborNeighbor(1, ShipDirection::NORTHEAST).size(), 0u);
    BOOST_REQUIRE_EQUAL(world.GetHarborNeighbor(1, ShipDirection::NORTHWEST).size(), 0u);

    // Hb 7 (inside)
    nb = world.GetHarborNeighbor(7, ShipDirection::NORTHWEST);
    BOOST_REQUIRE_EQUAL(nb.size(), 1u);
    BOOST_CHECK_EQUAL(nb[0].id, 4u);
    nb = world.GetHarborNeighbor(7, ShipDirection::NORTHEAST);
    BOOST_REQUIRE_EQUAL(nb.size(), 1u);
    BOOST_CHECK_EQUAL(nb[0].id, 5u);
    nb = world.GetHarborNeighbor(7, ShipDirection::NORTH);
    BOOST_REQUIRE_EQUAL(nb.size(), 1u);
    BOOST_CHECK_EQUAL(nb[0].id, 2u);
    BOOST_REQUIRE_EQUAL(world.GetHarborNeighbor(7, ShipDirection::SOUTH).size(), 0u);
    BOOST_REQUIRE_EQUAL(world.GetHarborNeighbor(7, ShipDirection::SOUTHEAST).size(), 0u);
    BOOST_REQUIRE_EQUAL(world.GetHarborNeighbor(7, ShipDirection::SOUTHWEST).size(), 0u);
}

BOOST_AUTO_TEST_SUITE_END()
