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

#include "rttrDefines.h" // IWYU pragma: keep
#include "GamePlayer.h"
#include "PointOutput.h"
#include "SeaWorldWithGCExecution.h"
#include "buildings/noBuildingSite.h"
#include "buildings/nobHarborBuilding.h"
#include "buildings/nobShipYard.h"
#include "factories/BuildingFactory.h"
#include "pathfinding/FindPathForRoad.h"
#include "postSystem/PostBox.h"
#include "postSystem/ShipPostMsg.h"
#include "nodeObjs/noShip.h"
#include "test/initTestHelpers.h"
#include <boost/foreach.hpp>
#include <boost/test/unit_test.hpp>
#include <iostream>

namespace {
std::vector<Direction> FindRoadPath(const MapPoint fromPt, const MapPoint toPt, const GameWorldBase& world)
{
    return FindPathForRoad(world, fromPt, toPt, false);
}
} // namespace

BOOST_AUTO_TEST_SUITE(SeafaringTestSuite)

BOOST_FIXTURE_TEST_CASE(HarborPlacing, SeaWorldWithGCExecution<>)
{
    const GamePlayer& player = world.GetPlayer(curPlayer);
    const BuildingRegister& buildings = player.GetBuildingRegister();
    const MapPoint hqPos = player.GetHQPos();
    const unsigned seaId = 1;
    const unsigned hbId = 1;
    const MapPoint hbPos = world.GetHarborPoint(hbId);
    BOOST_REQUIRE_LT(world.CalcDistance(hqPos, hbPos), HQ_RADIUS);

    nobHarborBuilding* harbor =
      dynamic_cast<nobHarborBuilding*>(BuildingFactory::CreateBuilding(world, BLD_HARBORBUILDING, hbPos, curPlayer, NAT_ROMANS));
    BOOST_REQUIRE(harbor);
    BOOST_REQUIRE_EQUAL(buildings.GetHarbors().size(), 1u); //-V807
    BOOST_REQUIRE_EQUAL(buildings.GetHarbors().front(), harbor);
    // A harbor is also a storehouse
    BOOST_REQUIRE_EQUAL(buildings.GetStorehouses().size(), 2u);
    BOOST_REQUIRE_EQUAL(buildings.GetHarbors().back(), harbor);
    std::vector<nobHarborBuilding*> harbors;
    BOOST_REQUIRE_EQUAL(world.GetNode(MapPoint(0, 0)).seaId, seaId);
    BOOST_REQUIRE_EQUAL(world.GetSeaId(hbId, Direction::NORTHEAST), seaId);
    player.GetHarborsAtSea(harbors, seaId);
    BOOST_REQUIRE_EQUAL(harbors.size(), 1u);
    BOOST_REQUIRE_EQUAL(harbors.front(), harbor);

    const std::vector<Direction> road =
      FindRoadPath(world.GetNeighbour(hqPos, Direction::SOUTHEAST), world.GetNeighbour(hbPos, Direction::SOUTHEAST), world);
    BOOST_REQUIRE(!road.empty());
}

BOOST_FIXTURE_TEST_CASE(ShipBuilding, SeaWorldWithGCExecution<>)
{
    initGameRNG();

    const GamePlayer& player = world.GetPlayer(curPlayer);
    const MapPoint hqPos = player.GetHQPos();
    const MapPoint hqFlagPos = world.GetNeighbour(hqPos, Direction::SOUTHEAST);
    const unsigned hbId = 1;
    const MapPoint hbPos = world.GetHarborPoint(hbId);
    const MapPoint shipyardPos(hqPos.x + 3, hqPos.y - 5);

    nobHarborBuilding* harbor =
      dynamic_cast<nobHarborBuilding*>(BuildingFactory::CreateBuilding(world, BLD_HARBORBUILDING, hbPos, curPlayer, NAT_ROMANS));
    BOOST_REQUIRE(harbor);
    std::vector<Direction> road = FindRoadPath(hqFlagPos, world.GetNeighbour(hbPos, Direction::SOUTHEAST), world);
    BOOST_REQUIRE(!road.empty());
    this->BuildRoad(hqFlagPos, false, road);
    MapPoint curPt = hqFlagPos;
    for(unsigned i = 0; i < road.size(); i++)
    {
        curPt = world.GetNeighbour(curPt, road[i]);
        this->SetFlag(curPt);
    }
    BOOST_REQUIRE_EQUAL(world.GetBQ(shipyardPos, curPlayer), BQ_CASTLE);
    nobShipYard* shipYard =
      dynamic_cast<nobShipYard*>(BuildingFactory::CreateBuilding(world, BLD_SHIPYARD, shipyardPos, curPlayer, NAT_ROMANS));
    BOOST_REQUIRE(shipYard);
    road = FindRoadPath(hqFlagPos, world.GetNeighbour(shipyardPos, Direction::SOUTHEAST), world);
    BOOST_REQUIRE(!road.empty());
    this->BuildRoad(hqFlagPos, false, road);
    BOOST_REQUIRE_EQUAL(shipYard->GetMode(), nobShipYard::BOATS); //-V522
    this->SetShipYardMode(shipyardPos, false);
    BOOST_REQUIRE_EQUAL(shipYard->GetMode(), nobShipYard::BOATS);
    this->SetShipYardMode(shipyardPos, true);
    BOOST_REQUIRE_EQUAL(shipYard->GetMode(), nobShipYard::SHIPS);
    this->SetShipYardMode(shipyardPos, true);
    BOOST_REQUIRE_EQUAL(shipYard->GetMode(), nobShipYard::SHIPS);
    this->SetShipYardMode(shipyardPos, false);
    BOOST_REQUIRE_EQUAL(shipYard->GetMode(), nobShipYard::BOATS);
    this->SetShipYardMode(shipyardPos, true);
    BOOST_REQUIRE_EQUAL(shipYard->GetMode(), nobShipYard::SHIPS);

    world.GetPostMgr().AddPostBox(curPlayer);
    PostBox& postBox = *world.GetPostMgr().GetPostBox(curPlayer);
    postBox.Clear();
    // Ship building takes 10 steps with ~500 GFs each. +600 GF to let wares and people reach the site
    RTTR_EXEC_TILL(5600, postBox.GetNumMsgs() > 0);
    // There should be a msg telling the player about the new ship
    BOOST_REQUIRE_EQUAL(postBox.GetNumMsgs(), 1u);
    const ShipPostMsg* msg = dynamic_cast<const ShipPostMsg*>(postBox.GetMsg(0));
    BOOST_REQUIRE(msg);
    BOOST_REQUIRE_EQUAL(player.GetNumShips(), 1u);
    BOOST_REQUIRE_EQUAL(player.GetShips().size(), 1u);
    noShip* ship = player.GetShipByID(0);
    BOOST_REQUIRE(ship);
    BOOST_REQUIRE_EQUAL(player.GetShipID(ship), 0u);
}

template<unsigned T_numPlayers = 3, unsigned T_hbId = 1, unsigned T_width = SeaWorldDefault::width,
         unsigned T_height = SeaWorldDefault::height>
struct ShipReadyFixture : public SeaWorldWithGCExecution<T_numPlayers, T_width, T_height>
{
    typedef SeaWorldWithGCExecution<T_numPlayers, T_width, T_height> Parent;
    using Parent::curPlayer;
    using Parent::world;

    PostBox* postBox;
    ShipReadyFixture()
    {
        GamePlayer& player = world.GetPlayer(curPlayer);
        const MapPoint hqPos = player.GetHQPos();
        const MapPoint hqFlagPos = world.GetNeighbour(hqPos, Direction::SOUTHEAST);
        const MapPoint hbPos = world.GetHarborPoint(T_hbId);
        world.GetPostMgr().AddPostBox(curPlayer);
        postBox = world.GetPostMgr().GetPostBox(curPlayer);

        nobHarborBuilding* harbor =
          dynamic_cast<nobHarborBuilding*>(BuildingFactory::CreateBuilding(world, BLD_HARBORBUILDING, hbPos, curPlayer, NAT_ROMANS));
        BOOST_REQUIRE(harbor);
        world.RecalcBQAroundPointBig(hbPos);
        std::vector<Direction> road = FindRoadPath(hqFlagPos, world.GetNeighbour(hbPos, Direction::SOUTHEAST), world);
        BOOST_REQUIRE(!road.empty());
        this->BuildRoad(hqFlagPos, false, road);
        MapPoint curPt = hqFlagPos;
        for(unsigned i = 0; i < road.size(); i++)
        {
            curPt = world.GetNeighbour(curPt, road[i]);
            this->SetFlag(curPt);
        }

        MapPoint shipPos(hbPos.x, hbPos.y - 3);
        if(!world.IsSeaPoint(shipPos))
            shipPos.y += 6;
        BOOST_REQUIRE(world.IsSeaPoint(shipPos));
        noShip* ship = new noShip(shipPos, curPlayer);
        world.AddFigure(ship->GetPos(), ship);
        player.RegisterShip(ship);

        BOOST_REQUIRE_EQUAL(player.GetNumShips(), 1u);
        postBox->Clear();
    }
};

BOOST_FIXTURE_TEST_CASE(ExplorationExpedition, ShipReadyFixture<>)
{
    initGameRNG();

    curPlayer = 0;
    const GamePlayer& player = world.GetPlayer(curPlayer);
    const noShip* ship = player.GetShipByID(0);
    const nobHarborBuilding& harbor = *player.GetBuildingRegister().GetHarbors().front();
    const MapPoint hbPos = harbor.GetPos();
    const unsigned hbId = world.GetHarborPointID(hbPos);
    BOOST_REQUIRE(ship);
    BOOST_REQUIRE(ship->IsIdling());
    BOOST_REQUIRE(!harbor.IsExplorationExpeditionActive());
    this->StartStopExplorationExpedition(hbPos, true);
    BOOST_REQUIRE(harbor.IsExplorationExpeditionActive());
    // Expedition not ready, ship still idling
    BOOST_REQUIRE(ship->IsIdling());

    // Wait till scouts arrive
    RTTR_EXEC_TILL(380, !ship->IsIdling());
    BOOST_REQUIRE(ship->IsGoingToHarbor(harbor));
    BOOST_REQUIRE_EQUAL(ship->GetTargetHarbor(), hbId);
    BOOST_REQUIRE_EQUAL(player.GetShipsToHarbor(harbor), 1u);

    // No available scouts
    BOOST_REQUIRE_EQUAL(harbor.GetNumRealFigures(JOB_SCOUT), 0u);
    // Stop it
    this->StartStopExplorationExpedition(hbPos, true);
    BOOST_REQUIRE(harbor.IsExplorationExpeditionActive());
    this->StartStopExplorationExpedition(hbPos, false);
    BOOST_REQUIRE(!harbor.IsExplorationExpeditionActive());
    this->StartStopExplorationExpedition(hbPos, false);
    BOOST_REQUIRE(!harbor.IsExplorationExpeditionActive());
    // Scouts available again
    BOOST_REQUIRE_EQUAL(harbor.GetNumRealFigures(JOB_SCOUT), 3u);

    // Let ship arrive
    RTTR_EXEC_TILL(180, ship->IsIdling());
    BOOST_REQUIRE(!ship->IsGoingToHarbor(harbor));
    BOOST_REQUIRE_EQUAL(player.GetShipsToHarbor(harbor), 0u);
    BOOST_REQUIRE_EQUAL(ship->GetTargetHarbor(), 0u);
    BOOST_REQUIRE_EQUAL(ship->GetHomeHarbor(), 0u);

    // We want the ship to only scout unexplored harbors, so set all but one to visible
    world.GetNodeWriteable(world.GetHarborPoint(6)).fow[curPlayer].visibility = VIS_VISIBLE; //-V807
    // Team visibility, so set one to own team
    world.GetPlayer(curPlayer).team = TM_TEAM1;
    world.GetPlayer(1).team = TM_TEAM1;
    world.GetPlayer(curPlayer).MakeStartPacts();
    world.GetPlayer(1).MakeStartPacts();
    world.GetNodeWriteable(world.GetHarborPoint(3)).fow[1].visibility = VIS_VISIBLE;
    unsigned targetHbId = 8u;

    // Start again (everything is here)
    this->StartStopExplorationExpedition(hbPos, true);
    // ...so we can start right now
    BOOST_REQUIRE(ship->IsOnExplorationExpedition());
    // Load and start
    RTTR_SKIP_GFS(202);
    BOOST_REQUIRE_EQUAL(ship->GetHomeHarbor(), hbId);
    BOOST_REQUIRE_EQUAL(ship->GetTargetHarbor(), targetHbId);

    unsigned distance = world.CalcDistance(hbPos, world.GetHarborPoint(targetHbId));
    // Let the ship scout the harbor
    RTTR_EXEC_TILL(distance * 2 * 20, !ship->IsMoving());
    BOOST_REQUIRE(ship->IsOnExplorationExpedition());
    BOOST_REQUIRE_LE(world.CalcDistance(world.GetHarborPoint(targetHbId), ship->GetPos()), 2u);
    // Now the ship waits and will select the next harbor. We allow another one:
    world.GetNodeWriteable(world.GetHarborPoint(6)).fow[curPlayer].visibility = VIS_FOW;
    targetHbId = 6u;
    RTTR_EXEC_TILL(350, ship->IsMoving());
    BOOST_REQUIRE_EQUAL(ship->GetHomeHarbor(), hbId);
    BOOST_REQUIRE_EQUAL(ship->GetTargetHarbor(), targetHbId);
    distance = world.CalcDistance(ship->GetPos(), world.GetHarborPoint(targetHbId));
    // Let the ship scout the harbor
    RTTR_EXEC_TILL(distance * 2 * 20, !ship->IsMoving());
    BOOST_REQUIRE(ship->IsOnExplorationExpedition());
    BOOST_REQUIRE_LE(world.CalcDistance(world.GetHarborPoint(targetHbId), ship->GetPos()), 2u);

    // Now disallow the first harbor so ship returns home
    world.GetNodeWriteable(world.GetHarborPoint(8)).fow[curPlayer].visibility = VIS_VISIBLE;

    RTTR_EXEC_TILL(350, ship->IsMoving());
    BOOST_REQUIRE_EQUAL(ship->GetHomeHarbor(), hbId);
    BOOST_REQUIRE_EQUAL(ship->GetTargetHarbor(), hbId);

    distance = world.CalcDistance(ship->GetPos(), world.GetHarborPoint(hbId));
    // And at some time it should return home
    RTTR_EXEC_TILL(distance * 2 * 20 + 200, ship->IsIdling());
    BOOST_REQUIRE_EQUAL(ship->GetSeaID(), 1u);
    BOOST_REQUIRE_EQUAL(ship->GetPos(), world.GetCoastalPoint(hbId, 1));

    // Now try to start an expedition but all harbors are explored -> Load, Unload, Idle
    world.GetNodeWriteable(world.GetHarborPoint(6)).fow[curPlayer].visibility = VIS_VISIBLE;
    this->StartStopExplorationExpedition(hbPos, true);
    BOOST_REQUIRE(ship->IsOnExplorationExpedition());
    RTTR_EXEC_TILL(2 * 200 + 5, ship->IsIdling());
}

BOOST_FIXTURE_TEST_CASE(DestroyHomeOnExplExp, ShipReadyFixture<2>)
{
    initGameRNG();

    curPlayer = 0;
    const GamePlayer& player = world.GetPlayer(curPlayer);
    const noShip* ship = player.GetShipByID(0);
    const nobHarborBuilding& harbor = *player.GetBuildingRegister().GetHarbors().front();
    const MapPoint hbPos = harbor.GetPos();
    const unsigned hbId = world.GetHarborPointID(hbPos);
    unsigned numScouts = player.GetInventory().people[JOB_SCOUT]; //-V807
    BOOST_REQUIRE(ship->IsIdling());

    // We want the ship to only scout unexplored harbors, so set all but one to visible
    world.GetPlayer(curPlayer).team = TM_TEAM1;
    world.GetPlayer(1).team = TM_TEAM1;
    world.GetPlayer(curPlayer).MakeStartPacts();
    world.GetPlayer(1).MakeStartPacts();

    world.GetNodeWriteable(world.GetHarborPoint(6)).fow[1].visibility = VIS_VISIBLE;
    world.GetNodeWriteable(world.GetHarborPoint(3)).fow[1].visibility = VIS_VISIBLE;
    unsigned targetHbId = 8u;
    this->StartStopExplorationExpedition(hbPos, true);

    // Start it
    RTTR_EXEC_TILL(600, ship->IsOnExplorationExpedition() && ship->IsMoving());
    // Incorporate recruitment
    BOOST_REQUIRE_GE(player.GetInventory().people[JOB_SCOUT], numScouts);
    numScouts = player.GetInventory().people[JOB_SCOUT];

    BOOST_REQUIRE_EQUAL(ship->GetHomeHarbor(), hbId);
    BOOST_REQUIRE_EQUAL(ship->GetTargetHarbor(), targetHbId);

    // Run till ship is coming back
    RTTR_EXEC_TILL(1000, ship->GetTargetHarbor() == hbId);
    // Avoid that it goes back to that point
    world.GetNodeWriteable(world.GetHarborPoint(targetHbId)).fow[1].visibility = VIS_VISIBLE;

    // Destroy home harbor
    world.DestroyNO(hbPos);
    RTTR_EXEC_TILL(2000, ship->IsLost());
    BOOST_REQUIRE(!ship->IsMoving());

    MapPoint newHbPos = world.GetHarborPoint(6);
    nobHarborBuilding* newHarbor =
      dynamic_cast<nobHarborBuilding*>(BuildingFactory::CreateBuilding(world, BLD_HARBORBUILDING, newHbPos, curPlayer, NAT_ROMANS));

    BOOST_REQUIRE(!ship->IsLost());
    BOOST_REQUIRE(ship->IsMoving());
    RTTR_EXEC_TILL(1200, ship->IsIdling());
    BOOST_REQUIRE_EQUAL(player.GetInventory().people[JOB_SCOUT], numScouts);
    BOOST_REQUIRE_EQUAL(newHarbor->GetNumRealFigures(JOB_SCOUT), newHarbor->GetNumVisualFigures(JOB_SCOUT)); //-V522
    BOOST_REQUIRE_EQUAL(newHarbor->GetNumRealFigures(JOB_SCOUT)
                          + world.GetSpecObj<nobBaseWarehouse>(player.GetHQPos())->GetNumRealFigures(JOB_SCOUT),
                        numScouts);
}

BOOST_FIXTURE_TEST_CASE(Expedition, ShipReadyFixture<>)
{
    initGameRNG();

    const GamePlayer& player = world.GetPlayer(curPlayer);
    const noShip* ship = player.GetShipByID(0);
    const nobHarborBuilding& harbor = *player.GetBuildingRegister().GetHarbors().front();
    const MapPoint hbPos = harbor.GetPos();
    BOOST_REQUIRE(ship);
    BOOST_REQUIRE(ship->IsIdling());
    BOOST_REQUIRE(!harbor.IsExpeditionActive());
    this->StartStopExpedition(hbPos, true);
    BOOST_REQUIRE(harbor.IsExpeditionActive());
    // Expedition not ready, ship still idling
    BOOST_REQUIRE(ship->IsIdling());

    // Wait till wares arrive
    RTTR_EXEC_TILL(3400, !ship->IsIdling());
    // Expedition ready, ship ordered
    BOOST_REQUIRE(ship->IsGoingToHarbor(harbor));
    BOOST_REQUIRE_EQUAL(player.GetShipsToHarbor(harbor), 1u);

    // No available boards
    BOOST_REQUIRE_EQUAL(harbor.GetNumRealWares(GD_BOARDS), 0u);
    // Stop it
    this->StartStopExpedition(hbPos, true);
    BOOST_REQUIRE(harbor.IsExpeditionActive());
    this->StartStopExpedition(hbPos, false);
    BOOST_REQUIRE(!harbor.IsExpeditionActive());
    this->StartStopExpedition(hbPos, false);
    BOOST_REQUIRE(!harbor.IsExpeditionActive());
    // Boards available again
    BOOST_REQUIRE_GT(harbor.GetNumRealWares(GD_BOARDS), 0u);

    // Let ship arrive
    RTTR_EXEC_TILL(180, ship->IsIdling());
    BOOST_REQUIRE(!ship->IsGoingToHarbor(harbor));
    BOOST_REQUIRE_EQUAL(player.GetShipsToHarbor(harbor), 0u);

    // Start again (everything is here)
    this->StartStopExpedition(hbPos, true);
    // ...so we can start right now
    BOOST_REQUIRE(ship->IsOnExpedition());

    // Wait for ship to be "loaded"
    RTTR_EXEC_TILL(200, ship->IsWaitingForExpeditionInstructions());
    // Ship should be waiting for expedition instructions (where to go) and player should have received a message
    BOOST_REQUIRE_EQUAL(ship->GetCurrentHarbor(), harbor.GetHarborPosID());
    BOOST_REQUIRE_EQUAL(postBox->GetNumMsgs(), 1u);
    const ShipPostMsg* msg = dynamic_cast<const ShipPostMsg*>(postBox->GetMsg(0));
    BOOST_REQUIRE(msg);
    BOOST_REQUIRE_EQUAL(msg->GetPos(), ship->GetPos()); //-V522

    // Harbor pos taken by other player
    this->TravelToNextSpot(ShipDirection::SOUTHEAST, player.GetShipID(ship));
    BOOST_REQUIRE(ship->IsWaitingForExpeditionInstructions());

    // Last free one (far south -> North is closer)
    this->TravelToNextSpot(ShipDirection::NORTH, player.GetShipID(ship));
    BOOST_REQUIRE(!ship->IsWaitingForExpeditionInstructions());
    BOOST_REQUIRE(ship->IsMoving());

    unsigned gfsToDest;
    RTTR_EXEC_TILL_CT_GF(1000, ship->IsWaitingForExpeditionInstructions(), gfsToDest);
    BOOST_REQUIRE_EQUAL(ship->GetCurrentHarbor(), 8u);
    BOOST_REQUIRE_EQUAL(world.CalcDistance(ship->GetPos(), world.GetHarborPoint(8)), 1u);

    // Cancel expedition -> Ship is going back to harbor
    this->CancelExpedition(player.GetShipID(ship));
    BOOST_REQUIRE(ship->IsMoving());
    // Let ship arrive and unload
    RTTR_EXEC_TILL(gfsToDest + 300, ship->IsIdling());

    // Start again (everything is here)
    this->StartStopExpedition(hbPos, true);
    BOOST_REQUIRE(ship->IsOnExpedition());

    // Wait for ship to be "loaded"
    RTTR_EXEC_TILL(200, ship->IsWaitingForExpeditionInstructions());
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
        BOOST_REQUIRE(ship->IsMoving());
        this->TravelToNextSpot(ShipDirection::SOUTH, player.GetShipID(ship));
        this->CancelExpedition(player.GetShipID(ship));
        BOOST_REQUIRE(ship->IsMoving());
        this->em.ExecuteNextGF();
    }
    // Go to destination
    postBox->Clear();
    RTTR_SKIP_GFS(gfsToDest);
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
    RTTR_EXEC_TILL(5000, player.GetBuildingRegister().GetHarbors().size() > 1); //-V807
    BOOST_REQUIRE_EQUAL(player.GetBuildingRegister().GetHarbors().size(), 2u);
}

typedef ShipReadyFixture<1, 2, 64, 800> ShipReadyFixtureBig;

BOOST_FIXTURE_TEST_CASE(LongDistanceTravel, ShipReadyFixtureBig)
{
    initGameRNG();
    const GamePlayer& player = world.GetPlayer(curPlayer);
    const noShip* ship = player.GetShipByID(0);
    nobHarborBuilding& harbor = *player.GetBuildingRegister().GetHarbors().front();
    const MapPoint hbPos = harbor.GetPos();
    BOOST_REQUIRE(ship);
    // Go to opposite one
    const unsigned targetHbId = 7;
    // Make sure that the other harbor is far away
    BOOST_REQUIRE_GT(world.CalcHarborDistance(2, targetHbId), 600u);
    // Add some scouts
    Inventory newScouts;
    newScouts.people[JOB_SCOUT] = 20;
    harbor.AddGoods(newScouts, true);
    // We want the ship to only scout unexplored harbors, so set all but one to visible
    for(unsigned i = 1; i <= 8; i++)
        world.GetNodeWriteable(world.GetHarborPoint(i)).fow[curPlayer].visibility = VIS_VISIBLE;
    world.GetNodeWriteable(world.GetHarborPoint(targetHbId)).fow[curPlayer].visibility = VIS_INVISIBLE;
    // Start an exploration expedition
    this->StartStopExplorationExpedition(hbPos, true);
    BOOST_REQUIRE(harbor.IsExplorationExpeditionActive());
    // Wait till ship has arrived and starts loading
    RTTR_EXEC_TILL(100, ship->IsOnExplorationExpedition());
    // Wait till ship has loaded scouts
    RTTR_EXEC_TILL(200, ship->IsMoving());
    BOOST_REQUIRE_EQUAL(ship->GetTargetHarbor(), targetHbId);
}

template<unsigned T_numPlayers = 2, unsigned T_width = SmallSeaWorldDefault<T_numPlayers>::width,
         unsigned T_height = SmallSeaWorldDefault<T_numPlayers>::height>
struct ShipAndHarborsReadyFixture : public WorldFixture<CreateWaterWorld, T_numPlayers, T_width, T_height>, public GCExecutor
{
public:
    typedef WorldFixture<CreateWaterWorld, T_numPlayers, T_width, T_height> Parent;
    using Parent::world;

    virtual GameWorldGame& GetWorld() override { return world; }

    nobHarborBuilding& createHarbor(unsigned hbPosId)
    {
        MapPoint hbPos = world.GetHarborPoint(hbPosId);
        nobHarborBuilding* harbor =
          static_cast<nobHarborBuilding*>(BuildingFactory::CreateBuilding(world, BLD_HARBORBUILDING, hbPos, curPlayer, NAT_ROMANS));
        BOOST_REQUIRE(harbor);
        Inventory inv;
        inv.Add(GD_WOOD, 10);
        inv.Add(JOB_WOODCUTTER, 10);
        harbor->AddGoods(inv, true);
        return *harbor;
    }

    ShipAndHarborsReadyFixture()
    {
        GamePlayer& player = world.GetPlayer(curPlayer);
        createHarbor(1);
        createHarbor(2);

        MapPoint hbPos = world.GetHarborPoint(1);
        MapPoint shipPos = world.MakeMapPoint(hbPos - Position(2, 0));
        BOOST_REQUIRE(world.IsSeaPoint(shipPos));
        noShip* ship = new noShip(shipPos, curPlayer);
        world.AddFigure(ship->GetPos(), ship);
        player.RegisterShip(ship);

        BOOST_REQUIRE_EQUAL(player.GetNumShips(), 1u);
    }
};

void destroyBldAndFire(GameWorldBase& world, const MapPoint& pos)
{
    world.DestroyNO(pos);
    // Remove fire
    world.DestroyNO(pos);
    // Remove burned wh if existing
    BOOST_FOREACH(noBase* fig, world.GetFigures(pos))
    {
        if(fig->GetGOT() == GOT_BURNEDWAREHOUSE)
        {
            // Remove go-out event (not automatically done as the burned wh is never removed)
            const GameEvent* ev = static_cast<TestEventManager&>(world.GetEvMgr()).GetObjEvents(*fig).front();
            world.GetEvMgr().RemoveEvent(ev);
            destroyAndDelete(fig);
            break;
        }
    }
}

BOOST_FIXTURE_TEST_CASE(HarborDestroyed, ShipAndHarborsReadyFixture<1>)
{
    const GamePlayer& player = world.GetPlayer(curPlayer);
    const noShip& ship = *player.GetShipByID(0);
    const MapPoint hb1Pos = world.GetHarborPoint(1);
    const MapPoint hb2Pos = world.GetHarborPoint(2);
    const MapPoint hb3Pos = world.GetHarborPoint(3);

    // Order goods
    SetInventorySetting(hb2Pos, GD_WOOD, EInventorySetting::COLLECT);
    SetInventorySetting(hb2Pos, JOB_WOODCUTTER, EInventorySetting::COLLECT);

    // Destroy home before load -> Abort after ship reaches harbor
    RTTR_EXEC_TILL(90, ship.IsMoving());
    destroyBldAndFire(world, hb1Pos);
    RTTR_EXEC_TILL(10, !ship.IsMoving());
    BOOST_REQUIRE(ship.IsIdling());

    // Destroy home during load -> Continue
    createHarbor(1);
    // Just wait for re-order event and instantly go to loading as ship does not need to move
    RTTR_EXEC_TILL(90, ship.IsLoading());
    destroyBldAndFire(world, hb1Pos);
    BOOST_REQUIRE(ship.IsLoading());
    BOOST_REQUIRE_EQUAL(ship.GetHomeHarbor(), 0u);
    RTTR_EXEC_TILL(300, ship.IsUnloading());
    BOOST_REQUIRE_EQUAL(ship.GetPos(), world.GetCoastalPoint(2, 1));
    RTTR_EXEC_TILL(200, ship.IsIdling());

    // Destroy destination before load -> Abort after ship reaches harbor
    createHarbor(1);
    RTTR_EXEC_TILL(90, ship.IsMoving());
    destroyBldAndFire(world, hb2Pos);
    RTTR_EXEC_TILL(200, !ship.IsMoving());
    BOOST_REQUIRE(ship.IsIdling());

    // Destroy destination during load -> Unload again
    createHarbor(2);
    // Order goods
    SetInventorySetting(hb2Pos, GD_WOOD, EInventorySetting::COLLECT);
    SetInventorySetting(hb2Pos, JOB_WOODCUTTER, EInventorySetting::COLLECT);
    RTTR_EXEC_TILL(300, ship.IsLoading());
    destroyBldAndFire(world, hb2Pos);
    BOOST_REQUIRE(ship.IsUnloading());
    RTTR_EXEC_TILL(200, ship.IsIdling());

    // Destroy destination during driving -> Go back and unload
    createHarbor(2);
    // Order goods
    SetInventorySetting(hb2Pos, GD_WOOD, EInventorySetting::COLLECT);
    SetInventorySetting(hb2Pos, JOB_WOODCUTTER, EInventorySetting::COLLECT);
    RTTR_EXEC_TILL(300, ship.IsLoading());
    RTTR_EXEC_TILL(200, ship.IsMoving());
    BOOST_REQUIRE_EQUAL(ship.GetHomeHarbor(), 1u);
    BOOST_REQUIRE_EQUAL(ship.GetTargetHarbor(), 2u);
    destroyBldAndFire(world, hb2Pos);
    RTTR_EXEC_TILL(10, ship.GetTargetHarbor() == 1u);
    RTTR_EXEC_TILL(20, ship.IsUnloading());
    BOOST_REQUIRE_EQUAL(ship.GetPos(), world.GetCoastalPoint(1, 1));
    RTTR_EXEC_TILL(200, ship.IsIdling());

    // Destroy destination during unloading -> Go back and unload
    createHarbor(2);
    // Order goods
    SetInventorySetting(hb2Pos, GD_WOOD, EInventorySetting::COLLECT);
    SetInventorySetting(hb2Pos, JOB_WOODCUTTER, EInventorySetting::COLLECT);
    RTTR_EXEC_TILL(700, ship.IsUnloading());
    BOOST_REQUIRE_EQUAL(ship.GetHomeHarbor(), 1u);
    BOOST_REQUIRE_EQUAL(ship.GetTargetHarbor(), 2u);
    destroyBldAndFire(world, hb2Pos);
    RTTR_EXEC_TILL(10, ship.GetTargetHarbor() == 1u);
    RTTR_EXEC_TILL(200, ship.IsUnloading());
    BOOST_REQUIRE_EQUAL(ship.GetPos(), world.GetCoastalPoint(1, 1));
    RTTR_EXEC_TILL(200, ship.IsIdling());

    // Destroy both during unloading -> Go to 3rd and unload
    createHarbor(2);
    createHarbor(3);
    // Order goods
    SetInventorySetting(hb2Pos, GD_WOOD, EInventorySetting::COLLECT);
    SetInventorySetting(hb2Pos, JOB_WOODCUTTER, EInventorySetting::COLLECT);
    RTTR_EXEC_TILL(700, ship.IsUnloading());
    BOOST_REQUIRE_EQUAL(ship.GetHomeHarbor(), 1u);
    BOOST_REQUIRE_EQUAL(ship.GetTargetHarbor(), 2u);
    destroyBldAndFire(world, hb1Pos);
    destroyBldAndFire(world, hb2Pos);
    RTTR_EXEC_TILL(10, ship.GetTargetHarbor() == 3u);
    // Destroy last ->
    destroyBldAndFire(world, hb3Pos);
    RTTR_EXEC_TILL(10, ship.IsLost());
    BOOST_REQUIRE_EQUAL(ship.GetHomeHarbor(), 0u);
    BOOST_REQUIRE_EQUAL(ship.GetTargetHarbor(), 0u);
    createHarbor(1);
    BOOST_REQUIRE(ship.IsMoving());
    BOOST_REQUIRE_EQUAL(ship.GetHomeHarbor(), 1u);
    BOOST_REQUIRE_EQUAL(ship.GetTargetHarbor(), 1u);
}

BOOST_AUTO_TEST_SUITE_END()
