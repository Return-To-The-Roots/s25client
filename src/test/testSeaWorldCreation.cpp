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
#include "GamePlayer.h"
#include "RTTR_AssertError.h"
#include <boost/test/unit_test.hpp>
#include <boost/foreach.hpp>

BOOST_AUTO_TEST_SUITE(SeaWorldCreationSuite)

BOOST_FIXTURE_TEST_CASE(TestHarborSpotCreation, SeaWorldWithGCExecution)
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
    BOOST_REQUIRE(!world.GetHarborPoint(0).IsValid());
#endif
    // Note: Dummy harbor not counted
    for(unsigned curHarborId = 1; curHarborId <= world.GetHarborPointCount(); curHarborId++)
    {
        const MapPoint curHarborPt = world.GetHarborPoint(curHarborId);
        BOOST_REQUIRE(curHarborPt.isValid());
        // Reverse mapping must match
        BOOST_REQUIRE_EQUAL(world.GetHarborPointID(curHarborPt), curHarborId);
        // We must be at one of the 2 seas
        unsigned seaId;
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

BOOST_AUTO_TEST_SUITE_END()
