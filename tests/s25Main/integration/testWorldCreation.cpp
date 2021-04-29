// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "GamePlayer.h"
#include "worldFixtures/CreateEmptyWorld.h"
#include "worldFixtures/WorldFixture.h"
#include "gameTypes/GameTypesOutput.h"
#include "gameData/MilitaryConsts.h"
#include <boost/test/unit_test.hpp>

/// HQ radius is 9 -> min size is 20
using WorldFixtureEmpty1P = WorldFixture<CreateEmptyWorld, 1, 20, 20>;
BOOST_FIXTURE_TEST_CASE(HQPlacement, WorldFixtureEmpty1P)
{
    GamePlayer& player = world.GetPlayer(0);
    BOOST_TEST_REQUIRE(player.isUsed());
    const MapPoint hqPos = player.GetHQPos();
    BOOST_TEST_REQUIRE(hqPos.isValid());
    BOOST_TEST_REQUIRE(world.GetNO(player.GetHQPos())->GetGOT() == GO_Type::NobHq);
    // Check ownership of points
    std::vector<MapPoint> ownerPts = world.GetPointsInRadius(hqPos, HQ_RADIUS);
    for(MapPoint pt : ownerPts)
    {
        // This should be ensured by `GetPointsInRadius`
        BOOST_TEST_REQUIRE(world.CalcDistance(pt, hqPos) <= HQ_RADIUS);
        // We must own this point
        BOOST_TEST_REQUIRE(world.GetNode(pt).owner == 1);
        // Points at radius are border nodes, others player territory
        if(world.CalcDistance(pt, hqPos) == HQ_RADIUS)
            BOOST_TEST_REQUIRE(world.IsBorderNode(pt, 1));
        else
            BOOST_TEST_REQUIRE(world.IsPlayerTerritory(pt));
    }
}
