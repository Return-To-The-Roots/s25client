// Copyright (C) 2005 - 2026 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "NodalObjectTypes.h"
#include "buildings/nobHQ.h"
#include "worldFixtures/WorldWithGCExecution.h"
#include "worldFixtures/initGameRNG.hpp"
#include <boost/test/unit_test.hpp>

/// Start with low wares and build 2 farms to trigger emergency protocol activation
struct EmergencyFixture : public WorldWithGCExecution1P
{
    nobHQ* hq = world.GetPlayer(0).GetHQ();
    EmergencyFixture()
    {
        hq->AddToInventory(hq->getStartInventory(StartWares::VLow), true);
        MapPoint pos;

        pos = hqPos + MapPoint(3, 0);
        world.SetBuildingSite(BuildingType::Farm, pos, 0);
        BuildRoadForBlds(pos, hqPos);

        pos = hqPos + MapPoint(-3, 0);
        world.SetBuildingSite(BuildingType::Farm, pos, 0);
        BuildRoadForBlds(pos, hqPos);

        // wait until emergency protocol should be activated
        RTTR_EXEC_TILL(500, hq->GetInventory()[GoodType::Boards] == 10);

        // activate program (with 10 boards it should trigger)
        world.GetPlayer(0).TestForEmergencyProgramm();

        // No more boards are carried out to the farms due to emergency protocol
        RTTR_SKIP_GFS(200);
        BOOST_TEST_CHECK(hq->GetInventory()[GoodType::Boards] == 10);

        initGameRNG();
    }
};

BOOST_FIXTURE_TEST_SUITE(EmergencyProtocol, EmergencyFixture)
BOOST_AUTO_TEST_CASE(CanBuildWoodcutterAndSawmill)
{
    const MapPoint posWoodcutter = hqPos + MapPoint(-1, 2);
    world.SetBuildingSite(BuildingType::Woodcutter, posWoodcutter, 0);
    BuildRoadForBlds(posWoodcutter, hqPos);

    const MapPoint posSawmill = hqPos + MapPoint(-2, 4);
    world.SetBuildingSite(BuildingType::Sawmill, posSawmill, 0);
    BuildRoadForBlds(posSawmill, hqPos);

    // check if inventory boards are given out
    RTTR_EXEC_TILL(200, hq->GetInventory()[GoodType::Boards] < 10);

    // check that buildings are built
    RTTR_EXEC_TILL(2000, world.GetNO(posWoodcutter)->GetType() == NodalObjectType::Building);
    RTTR_EXEC_TILL(2000, world.GetNO(posSawmill)->GetType() == NodalObjectType::Building);
}

BOOST_FIXTURE_TEST_CASE(CannotBuildOtherBuldings, EmergencyFixture)
{
    const MapPoint pos = hqPos + MapPoint(-3, 0);
    world.SetBuildingSite(BuildingType::Watchtower, pos, 0);

    BuildRoadForBlds(pos, hqPos);

    // No boards are carried out to the farms or watchtower due to emergency protocol
    RTTR_SKIP_GFS(500);
    BOOST_TEST_CHECK(hq->GetInventory()[GoodType::Boards] == 10);
}

BOOST_AUTO_TEST_SUITE_END()