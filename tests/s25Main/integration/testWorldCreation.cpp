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

#include "GamePlayer.h"
#include "worldFixtures/CreateEmptyWorld.h"
#include "worldFixtures/WorldFixture.h"
#include "gameData/MilitaryConsts.h"
#include <boost/test/unit_test.hpp>

/// HQ radius is 9 -> min size is 20
using WorldFixtureEmpty1P = WorldFixture<CreateEmptyWorld, 1, 20, 20>;
BOOST_FIXTURE_TEST_CASE(HQPlacement, WorldFixtureEmpty1P)
{
    GamePlayer& player = world.GetPlayer(0);
    BOOST_REQUIRE(player.isUsed());
    const MapPoint hqPos = player.GetHQPos();
    BOOST_REQUIRE(hqPos.isValid());
    BOOST_REQUIRE_EQUAL(world.GetNO(player.GetHQPos())->GetGOT(), GOT_NOB_HQ);
    // Check ownership of points
    std::vector<MapPoint> ownerPts = world.GetPointsInRadius(hqPos, HQ_RADIUS);
    for(MapPoint pt : ownerPts)
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
