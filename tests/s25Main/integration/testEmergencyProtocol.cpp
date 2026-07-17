// Copyright (C) 2005 - 2026 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "EconomyModeHandler.h"
#include "EventManager.h"
#include "GamePlayer.h"
#include "Savegame.h"
#include "SerializedGameData.h"
#include "addons/AddonEconomyModeGameLength.h"
#include "factories/BuildingFactory.h"
#include "worldFixtures/MockLocalGameState.h"
#include "worldFixtures/WorldFixture.h"
#include "worldFixtures/WorldWithGCExecution.h"
#include "worldFixtures/initGameRNG.hpp"
#include "gameTypes/GO_Type.h"
#include <boost/test/unit_test.hpp>
#include <buildings/nobUsual.h>
#include <buildings/nobHQ.h>

struct EmergencyFixture : public WorldFixture<CreateEmptyWorld, 1>
{
    nobHQ * HQ = world.GetPlayer(0).GetHQ();
    EmergencyFixture()
    {
        HQ->AddToInventory(HQ->getStartInventory(StartWares::VLow), true);
        MapPoint pos;

        pos = world.GetPlayer(0).GetHQPos() + MapPoint(3, 0);
        world.SetBuildingSite(BuildingType::Farm, pos, 0);
        world.BuildRoad(0, false, world.GetNeighbour(pos, Direction::SouthEast),
                        std::vector<Direction>(3, Direction::West));


        pos = world.GetPlayer(0).GetHQPos() + MapPoint(-3, 0);
        world.SetBuildingSite(BuildingType::Farm, pos, 0);
        world.BuildRoad(0, false, world.GetNeighbour(pos, Direction::SouthEast),
                        std::vector<Direction>(3, Direction::East));


        //wait until emergency protocol should be activated
        RTTR_EXEC_TILL(500,HQ->GetInventory()[GoodType::Boards] == 10);

        //activate program (with 10 boards it should trigger)
        world.GetPlayer(0).TestForEmergencyProgramm();

        //wait for some more ticks to give time if not working to deliver more boards
        RTTR_SKIP_GFS(200);
        //check boards are still fine and protocol working
        BOOST_TEST_CHECK(world.GetPlayer(0).GetHQ()->GetInventory()[GoodType::Boards] == 10);
    }
};

BOOST_FIXTURE_TEST_CASE(EmergencyProtoclActiveWoodcutterAndSawmillCanBuild, EmergencyFixture)
{
    initGameRNG();

    MapPoint posWoodcutter = world.GetPlayer(0).GetHQPos() + MapPoint(-1, 2);
    world.SetBuildingSite(BuildingType::Woodcutter, posWoodcutter, 0);
    world.BuildRoad(0, false, world.GetNeighbour(posWoodcutter, Direction::SouthEast),
                    std::vector<Direction>(2, Direction::NorthEast));

    MapPoint posSawmill = world.GetPlayer(0).GetHQPos() + MapPoint(-2, 4);
    world.SetBuildingSite(BuildingType::Sawmill, posSawmill, 0);
    world.BuildRoad(0, false, world.GetNeighbour(posSawmill, Direction::SouthEast),
                    std::vector<Direction>(2, Direction::NorthEast));

    //check if inventory boards are given out
    RTTR_EXEC_TILL(500,HQ->GetInventory()[GoodType::Boards] < 10);

    //check if building where found
    RTTR_EXEC_TILL(10000,world.GetNO(posWoodcutter)->GetType() == NodalObjectType::Building);
    RTTR_EXEC_TILL(10000,world.GetNO(posSawmill)->GetType() == NodalObjectType::Building);
}

BOOST_FIXTURE_TEST_CASE(EmergencyProtoclActiveOtherBuldingsnotBuild, EmergencyFixture)
{
    initGameRNG();

    MapPoint pos = world.GetPlayer(0).GetHQPos() + MapPoint(-1, 2);
    world.SetBuildingSite(BuildingType::Watchtower, pos, 0);
    world.BuildRoad(0, false, world.GetNeighbour(pos, Direction::SouthEast),
                    std::vector<Direction>(2, Direction::NorthEast));

    //wait for some more ticks to give time if not working to deliver more boards
    RTTR_SKIP_GFS(200);
    //check boards are still fine and protocol working
    BOOST_TEST_CHECK(world.GetPlayer(0).GetHQ()->GetInventory()[GoodType::Boards] == 10);
}
