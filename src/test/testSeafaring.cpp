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
#include "pathfinding/FindPathForRoad.h"
#include "factories/BuildingFactory.h"
#include "buildings/noBuildingSite.h"
#include "buildings/nobHarborBuilding.h"
#include "buildings/nobShipYard.h"
#include "postSystem/PostBox.h"
#include "postSystem/ShipPostMsg.h"
#include "nodeObjs/noShip.h"
#include "PointOutput.h"
#include "Random.h"
#include <boost/test/unit_test.hpp>
#include <iostream>

namespace{
    std::vector<unsigned char> FindRoadPath(const MapPoint fromPt, const MapPoint toPt, const GameWorldBase& world)
    {
        return FindPathForRoad(world, fromPt, toPt, world, false);
    }
}

BOOST_AUTO_TEST_SUITE(SeafaringTestSuite)

BOOST_FIXTURE_TEST_CASE(HarborPlacing, SeaWorldWithGCExecution)
{
    const GamePlayer& player = world.GetPlayer(curPlayer);
    const MapPoint hqPos = player.GetHQPos();
    const unsigned seaId = 1;
    const unsigned hbId = 1;
    const MapPoint hbPos = world.GetHarborPoint(hbId);
    BOOST_REQUIRE_LT(world.CalcDistance(hqPos, hbPos), HQ_RADIUS);
    
    nobHarborBuilding* harbor = dynamic_cast<nobHarborBuilding*>(BuildingFactory::CreateBuilding(&world, BLD_HARBORBUILDING, hbPos, curPlayer, NAT_ROMANS));
    BOOST_REQUIRE(harbor);
    BOOST_REQUIRE_EQUAL(player.GetHarbors().size(), 1u);
    BOOST_REQUIRE_EQUAL(player.GetHarbors().front(), harbor);
    // A harbor is also a storehouse
    BOOST_REQUIRE_EQUAL(player.GetStorehouses().size(), 2u);
    BOOST_REQUIRE_EQUAL(player.GetHarbors().back(), harbor);
    std::vector<nobHarborBuilding*> harbors;
    BOOST_REQUIRE_EQUAL(world.GetNode(MapPoint(0, 0)).seaId, seaId);
    BOOST_REQUIRE_EQUAL(world.GetSeaId(hbId, Direction::NORTHWEST), seaId);
    player.GetHarborsAtSea(harbors, seaId);
    BOOST_REQUIRE_EQUAL(harbors.size(), 1u);
    BOOST_REQUIRE_EQUAL(harbors.front(), harbor);

    const std::vector<unsigned char> road = FindRoadPath(world.GetNeighbour(hqPos, Direction::SOUTHEAST), world.GetNeighbour(hbPos, Direction::SOUTHEAST), world);
    BOOST_REQUIRE(!road.empty());
}

BOOST_FIXTURE_TEST_CASE(ShipBuilding, SeaWorldWithGCExecution)
{
    std::cout << "Random: " << RANDOM.GetCurrentRandomValue() << std::endl;
    const GamePlayer& player = world.GetPlayer(curPlayer);
    const MapPoint hqPos = player.GetHQPos();
    const MapPoint hqFlagPos = world.GetNeighbour(hqPos, Direction::SOUTHEAST);
    const unsigned hbId = 1;
    const MapPoint hbPos = world.GetHarborPoint(hbId);
    const MapPoint shipyardPos(hqPos.x + 3, hqPos.y - 5);

    nobHarborBuilding* harbor = dynamic_cast<nobHarborBuilding*>(BuildingFactory::CreateBuilding(&world, BLD_HARBORBUILDING, hbPos, curPlayer, NAT_ROMANS));
    BOOST_REQUIRE(harbor);
    std::vector<unsigned char> road = FindRoadPath(hqFlagPos, world.GetNeighbour(hbPos, Direction::SOUTHEAST), world);
    BOOST_REQUIRE(!road.empty());
    this->BuildRoad(hqFlagPos, false, road);
    MapPoint curPt = hqFlagPos;
    for(unsigned i=0; i<road.size(); i++)
    {
        curPt = world.GetNeighbour(curPt, road[i]);
        this->SetFlag(curPt);
    }
    BOOST_REQUIRE_EQUAL(world.GetBQ(shipyardPos, curPlayer), BQ_CASTLE);
    nobShipYard* shipYard = dynamic_cast<nobShipYard*>(BuildingFactory::CreateBuilding(&world, BLD_SHIPYARD, shipyardPos, curPlayer, NAT_ROMANS));
    BOOST_REQUIRE(shipYard);
    road = FindRoadPath(hqFlagPos, world.GetNeighbour(shipyardPos, Direction::SOUTHEAST), world);
    BOOST_REQUIRE(!road.empty());
    this->BuildRoad(hqFlagPos, false, road);
    BOOST_REQUIRE_EQUAL(shipYard->GetMode(), nobShipYard::BOATS);
    this->ToggleShipYardMode(shipyardPos);
    BOOST_REQUIRE_EQUAL(shipYard->GetMode(), nobShipYard::SHIPS);

    world.GetPostMgr().AddPostBox(curPlayer);
    PostBox& postBox = *world.GetPostMgr().GetPostBox(curPlayer);
    postBox.Clear();
    // Ship building takes 10 steps with ~500 GFs each. +600 GF to let wares and people reach the site
    for(unsigned gf = 0; gf < 5600; gf++)
    {
        this->em.ExecuteNextGF();
        if(postBox.GetNumMsgs() > 0)
            break;
    }
    // There should be a msg telling the player about the new ship
    BOOST_REQUIRE_EQUAL(postBox.GetNumMsgs(), 1u);
    const ShipPostMsg* msg = dynamic_cast<const ShipPostMsg*>(postBox.GetMsg(0));
    BOOST_REQUIRE(msg);
    BOOST_REQUIRE_EQUAL(player.GetShipCount(), 1u);
    BOOST_REQUIRE_EQUAL(player.GetShips().size(), 1u);
    noShip* ship = player.GetShipByID(0);
    BOOST_REQUIRE(ship);
    BOOST_REQUIRE_EQUAL(player.GetShipID(ship), 0u);
}

struct ShipReadyFixture: public SeaWorldWithGCExecution
{
    PostBox* postBox;
    ShipReadyFixture()
    {
        GamePlayer& player = world.GetPlayer(curPlayer);
        const MapPoint hqPos = player.GetHQPos();
        const MapPoint hqFlagPos = world.GetNeighbour(hqPos, Direction::SOUTHEAST);
        const unsigned hbId = 1;
        const MapPoint hbPos = world.GetHarborPoint(hbId);
        world.GetPostMgr().AddPostBox(curPlayer);
        postBox = world.GetPostMgr().GetPostBox(curPlayer);

        nobHarborBuilding* harbor = dynamic_cast<nobHarborBuilding*>(BuildingFactory::CreateBuilding(&world, BLD_HARBORBUILDING, hbPos, curPlayer, NAT_ROMANS));
        BOOST_REQUIRE(harbor);
        world.RecalcBQAroundPointBig(hbPos);
        std::vector<unsigned char> road = FindRoadPath(hqFlagPos, world.GetNeighbour(hbPos, Direction::SOUTHEAST), world);
        BOOST_REQUIRE(!road.empty());
        this->BuildRoad(hqFlagPos, false, road);
        MapPoint curPt = hqFlagPos;
        for(unsigned i = 0; i < road.size(); i++)
        {
            curPt = world.GetNeighbour(curPt, road[i]);
            this->SetFlag(curPt);
        }

        noShip* ship = new noShip(MapPoint(hbPos.x, hbPos.y - 7), curPlayer);
        world.AddFigure(ship, ship->GetPos());
        player.RegisterShip(ship);

        BOOST_REQUIRE_EQUAL(player.GetShipCount(), 1u);
        postBox->Clear();
    }
};

BOOST_FIXTURE_TEST_CASE(ExplorationExpedition, ShipReadyFixture)
{
    std::cout << "Random: " << RANDOM.GetCurrentRandomValue() << std::endl;
    const GamePlayer& player = world.GetPlayer(curPlayer);
    const noShip* ship = player.GetShipByID(0);
    const nobHarborBuilding& harbor = *player.GetHarbors().front();
    const MapPoint hbPos = harbor.GetPos();
    BOOST_REQUIRE(ship);
    BOOST_REQUIRE(ship->IsIdling());
    BOOST_REQUIRE(!harbor.IsExplorationExpeditionActive());
    this->StartExplorationExpedition(hbPos);
    BOOST_REQUIRE(harbor.IsExplorationExpeditionActive());
    // Expedition not ready, ship still idling
    BOOST_REQUIRE(ship->IsIdling());

    // Wait till scouts arrive
    for(unsigned gf = 0; gf < 380; gf++)
    {
        this->em.ExecuteNextGF();
        if(!ship->IsIdling())
            break;
    }
    // Expedition ready, ship ordered
    BOOST_REQUIRE(!ship->IsIdling());
    BOOST_REQUIRE(ship->IsGoingToHarbor(harbor));
    BOOST_REQUIRE_EQUAL(player.GetShipsToHarbor(harbor), 1u);

    // No available scouts
    BOOST_REQUIRE_EQUAL(harbor.GetRealFiguresCount(JOB_SCOUT), 0u);
    // Stop it
    this->StartExplorationExpedition(hbPos);
    BOOST_REQUIRE(!harbor.IsExplorationExpeditionActive());
    // Scouts available again
    BOOST_REQUIRE_EQUAL(harbor.GetRealFiguresCount(JOB_SCOUT), 3u);

    // Let ship arrive
    for(unsigned gf = 0; gf < 180; gf++)
    {
        this->em.ExecuteNextGF();
        if(ship->IsIdling())
            break;
    }
    BOOST_REQUIRE(ship->IsIdling());
    BOOST_REQUIRE(!ship->IsGoingToHarbor(harbor));
    BOOST_REQUIRE_EQUAL(player.GetShipsToHarbor(harbor), 0u);

    // Start again (everything is here)
    this->StartExplorationExpedition(hbPos);
    // ...so we can start right now
    BOOST_REQUIRE(ship->IsOnExplorationExpedition());

    // Let the ship scout a bit
    for(unsigned gf = 0; gf < 200+20*20; gf++)
        this->em.ExecuteNextGF();
    BOOST_REQUIRE(ship->IsOnExplorationExpedition());
    BOOST_REQUIRE_GT(world.CalcDistance(hbPos, ship->GetPos()), 10u);
    // And at some time it should return home
    for(unsigned gf = 0; gf < 5600; gf++)
    {
        this->em.ExecuteNextGF();
        if(ship->IsIdling())
            break;
    }
    BOOST_REQUIRE(ship->IsIdling());
    BOOST_REQUIRE_EQUAL(ship->GetSeaID(), 1u);
    BOOST_REQUIRE_EQUAL(ship->GetPos(), world.GetCoastalPoint(harbor.GetHarborPosID(), 1));
}

BOOST_FIXTURE_TEST_CASE(Expedition, ShipReadyFixture)
{
    std::cout << "Random: " << RANDOM.GetCurrentRandomValue() << std::endl;
    const GamePlayer& player = world.GetPlayer(curPlayer);
    const noShip* ship = player.GetShipByID(0);
    const nobHarborBuilding& harbor = *player.GetHarbors().front();
    const MapPoint hbPos = harbor.GetPos();
    BOOST_REQUIRE(ship);
    BOOST_REQUIRE(ship->IsIdling());
    BOOST_REQUIRE(!harbor.IsExpeditionActive());
    this->StartExpedition(hbPos);
    BOOST_REQUIRE(harbor.IsExpeditionActive());
    // Expedition not ready, ship still idling
    BOOST_REQUIRE(ship->IsIdling());

    // Wait till wares arrive
    for(unsigned gf = 0; gf < 3400; gf++)
    {
        this->em.ExecuteNextGF();
        if(!ship->IsIdling())
            break;
    }
    // Expedition ready, ship ordered
    BOOST_REQUIRE(!ship->IsIdling());
    BOOST_REQUIRE(ship->IsGoingToHarbor(harbor));
    BOOST_REQUIRE_EQUAL(player.GetShipsToHarbor(harbor), 1u);

    // No available boards
    BOOST_REQUIRE_EQUAL(harbor.GetRealWaresCount(GD_BOARDS), 0u);
    // Stop it
    this->StartExpedition(hbPos);
    BOOST_REQUIRE(!harbor.IsExpeditionActive());
    // Boards available again
    BOOST_REQUIRE_GT(harbor.GetRealWaresCount(GD_BOARDS), 0u);

    // Let ship arrive
    for(unsigned gf = 0; gf < 180; gf++)
    {
        this->em.ExecuteNextGF();
        if(ship->IsIdling())
            break;
    }
    BOOST_REQUIRE(ship->IsIdling());
    BOOST_REQUIRE(!ship->IsGoingToHarbor(harbor));
    BOOST_REQUIRE_EQUAL(player.GetShipsToHarbor(harbor), 0u);

    // Start again (everything is here)
    this->StartExpedition(hbPos);
    // ...so we can start right now
    BOOST_REQUIRE(ship->IsOnExpedition());

    // Wait for ship to be "loaded"
    for(unsigned gf = 0; gf < 200; gf++)
    {
        this->em.ExecuteNextGF();
        if(ship->IsWaitingForExpeditionInstructions())
            break;
    }
    // Ship should be waiting for expedition instructions (where to go) and player should have received a message
    BOOST_REQUIRE(ship->IsWaitingForExpeditionInstructions());
    BOOST_REQUIRE_EQUAL(ship->GetCurrentHarbor(), harbor.GetHarborPosID());
    BOOST_REQUIRE_EQUAL(postBox->GetNumMsgs(), 1u);
    const ShipPostMsg* msg = dynamic_cast<const ShipPostMsg*>(postBox->GetMsg(0));
    BOOST_REQUIRE(msg);
    BOOST_REQUIRE_EQUAL(msg->GetPos(), ship->GetPos());

    // Harbor pos taken by other player
    this->TravelToNextSpot(ShipDirection::SOUTHEAST, player.GetShipID(ship));
    BOOST_REQUIRE(ship->IsWaitingForExpeditionInstructions());

    // Last free one (far south -> North is closer)
    this->TravelToNextSpot(ShipDirection::NORTH, player.GetShipID(ship));
    BOOST_REQUIRE(!ship->IsWaitingForExpeditionInstructions());
    BOOST_REQUIRE(ship->IsMoving());

    unsigned gfsToDest;
    for(gfsToDest = 0; gfsToDest < 1000; gfsToDest++)
    {
        this->em.ExecuteNextGF();
        if(ship->IsWaitingForExpeditionInstructions())
            break;
    }
    BOOST_REQUIRE(ship->IsWaitingForExpeditionInstructions());
    BOOST_REQUIRE_EQUAL(ship->GetCurrentHarbor(), 8u);
    BOOST_REQUIRE_EQUAL(world.CalcDistance(ship->GetPos(), world.GetHarborPoint(8)), 1u);

    // Cancel expedition -> Ship is going back to harbor
    this->CancelExpedition(player.GetShipID(ship));
    BOOST_REQUIRE(ship->IsMoving());
    // Let ship arrive and unload
    for(unsigned gf = 0; gf < gfsToDest + 300; gf++)
    {
        this->em.ExecuteNextGF();
        if(ship->IsIdling())
            break;
    }
    BOOST_REQUIRE(ship->IsIdling());

    // Start again (everything is here)
    this->StartExpedition(hbPos);
    BOOST_REQUIRE(ship->IsOnExpedition());

    // Wait for ship to be "loaded"
    for(unsigned gf = 0; gf < 200; gf++)
    {
        this->em.ExecuteNextGF();
        if(ship->IsWaitingForExpeditionInstructions())
            break;
    }
    BOOST_REQUIRE(ship->IsWaitingForExpeditionInstructions());
    BOOST_REQUIRE_EQUAL(ship->GetCurrentHarbor(), harbor.GetHarborPosID());

    // Try to found colony -> Fail
    this->FoundColony(player.GetShipID(ship));
    BOOST_REQUIRE(ship->IsWaitingForExpeditionInstructions());
    // Go back to free spot
    this->TravelToNextSpot(ShipDirection::NORTH, player.GetShipID(ship));
    BOOST_REQUIRE(!ship->IsWaitingForExpeditionInstructions());
    for(unsigned gf = 0; gf < 2; gf++)
    {
        // Send commands that should be ignored as ship is not expecting them
        this->FoundColony(player.GetShipID(ship));
        this->TravelToNextSpot(ShipDirection::NORTH, player.GetShipID(ship));
        this->CancelExpedition(player.GetShipID(ship));
        this->em.ExecuteNextGF();
    }
    // Go to destination
    postBox->Clear();
    for(unsigned gf = 0; gf < gfsToDest; gf++)
        this->em.ExecuteNextGF();
    BOOST_REQUIRE(ship->IsWaitingForExpeditionInstructions());
    // This should again trigger a message
    msg = dynamic_cast<const ShipPostMsg*>(postBox->GetMsg(0));
    BOOST_REQUIRE(msg);
    BOOST_REQUIRE_EQUAL(msg->GetPos(), ship->GetPos());
    // And now we can found a new colony
    this->FoundColony(player.GetShipID(ship));
    // Ship is free again
    BOOST_REQUIRE(ship->IsIdling());
    const noBuildingSite* newHarbor = world.GetSpecObj<noBuildingSite>(world.GetHarborPoint(8));
    BOOST_REQUIRE(newHarbor);
    BOOST_REQUIRE(world.IsHarborBuildingSiteFromSea(newHarbor));
    // And it should be completed after some time
    for(unsigned gf = 0; gf < 5000; gf++)
    {
        this->em.ExecuteNextGF();
        if(player.GetHarbors().size() > 1)
            break;
    }
    BOOST_REQUIRE_EQUAL(player.GetHarbors().size(), 2u);
}

BOOST_AUTO_TEST_SUITE_END()
