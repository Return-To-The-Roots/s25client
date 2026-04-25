// Copyright (C) 2005 - 2026 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "GamePlayer.h"
#include "PointOutput.h"
#include "Ware.h"
#include "buildings/noBuildingSite.h"
#include "buildings/nobHarborBuilding.h"
#include "buildings/nobShipYard.h"
#include "factories/BuildingFactory.h"
#include "helpers/IdRange.h"
#include "helpers/Range.h"
#include "pathfinding/FindPathForRoad.h"
#include "postSystem/PostBox.h"
#include "postSystem/ShipPostMsg.h"
#include "worldFixtures/SeaWorldWithGCExecution.h"
#include "worldFixtures/initGameRNG.hpp"
#include "worldFixtures/terrainHelpers.h"
#include "world/MapLoader.h"
#include "nodeObjs/noShip.h"
#include "gameTypes/GameTypesOutput.h"
#include "rttr/test/testHelpers.hpp"
#include <boost/test/unit_test.hpp>
#include <iterator>

// LCOV_EXCL_START
#define RTTR_ENUM_OUTPUT(EnumName)                                                 \
    static std::ostream& operator<<(std::ostream& out, const EnumName e)           \
    {                                                                              \
        return out << #EnumName "::" << static_cast<unsigned>(rttr::enum_cast(e)); \
    }

RTTR_ENUM_OUTPUT(nobShipYard::Mode)
// LCOV_EXCL_STOP

namespace {

using SmallSeaWorld = WorldFixture<CreateSeaWorld, 2, SmallSeaWorldDefault<2>::width, SmallSeaWorldDefault<2>::height>;

std::vector<Direction> FindRoadPath(const MapPoint fromPt, const MapPoint toPt, const GameWorldBase& world)
{
    return FindPathForRoad(world, fromPt, toPt, false);
}
/// Helper to extract only the destinations(harbors) from the connections list
std::set<const noRoadNode*> toHarbors(const std::vector<nobHarborBuilding::ShipConnection>& connections)
{
    std::set<const noRoadNode*> res;
    std::transform(connections.begin(), connections.end(), std::inserter(res, res.begin()),
                   [](const auto& con) { return con.dest; });
    return res;
}
} // namespace

BOOST_AUTO_TEST_SUITE(SeafaringTestSuite)

BOOST_FIXTURE_TEST_CASE(HarborPlacing, SeaWorldWithGCExecution<>)
{
    const GamePlayer& player = world.GetPlayer(curPlayer);
    const BuildingRegister& buildings = player.GetBuildingRegister();
    const MapPoint hqPos = player.GetHQPos();
    const SeaId seaId(1);
    const HarborId hbId(1);
    const MapPoint hbPos = world.GetHarborPoint(hbId);
    BOOST_TEST_REQUIRE(world.CalcDistance(hqPos, hbPos) < HQ_RADIUS);

    auto* harbor = dynamic_cast<nobHarborBuilding*>(
      BuildingFactory::CreateBuilding(world, BuildingType::HarborBuilding, hbPos, curPlayer, Nation::Romans));
    BOOST_TEST_REQUIRE(harbor);
    BOOST_TEST_REQUIRE(world.IsHarborAtSea(hbId, seaId));
    BOOST_TEST_REQUIRE(buildings.GetHarbors().size() == 1u); //-V807
    BOOST_TEST_REQUIRE(buildings.GetHarbors().front() == harbor);
    // A harbor is also a storehouse
    BOOST_TEST_REQUIRE(buildings.GetStorehouses().size() == 2u);
    BOOST_TEST_REQUIRE(buildings.GetHarbors().back() == harbor);
    BOOST_TEST_REQUIRE(world.GetNode(MapPoint(0, 0)).seaId == seaId);
    BOOST_TEST_REQUIRE(world.GetSeaId(hbId, Direction::NorthEast) == seaId);

    const std::vector<Direction> road = FindRoadPath(world.GetNeighbour(hqPos, Direction::SouthEast),
                                                     world.GetNeighbour(hbPos, Direction::SouthEast), world);
    BOOST_TEST_REQUIRE(!road.empty());
}

BOOST_FIXTURE_TEST_CASE(ShipConnections, SmallSeaWorld)
{
    unsigned curPlayer = 0;
    const auto createHarbor = [this, &curPlayer](HarborId hbId) -> nobHarborBuilding& {
        MapPoint hbPos = world.GetHarborPoint(hbId);
        auto* harbor = static_cast<nobHarborBuilding*>(
          BuildingFactory::CreateBuilding(world, BuildingType::HarborBuilding, hbPos, curPlayer, Nation::Romans));
        BOOST_TEST_REQUIRE(harbor);
        return *harbor;
    };
    /* Layout of harbors (1 inner + 1 outer sea)
     *       1
     *       2
     * 3  4      5  6
     *       7
     *       8
     */
    // I.e.:
    constexpr SeaId sea1(1);
    constexpr SeaId sea2(2);
    BOOST_TEST(world.IsHarborAtSea(HarborId(1), sea1));
    BOOST_TEST(world.IsHarborAtSea(HarborId(3), sea1));
    BOOST_TEST(world.IsHarborAtSea(HarborId(6), sea1));
    BOOST_TEST(world.IsHarborAtSea(HarborId(8), sea1));

    BOOST_TEST(world.IsHarborAtSea(HarborId(2), sea2));
    BOOST_TEST(world.IsHarborAtSea(HarborId(4), sea2));
    BOOST_TEST(world.IsHarborAtSea(HarborId(5), sea2));
    BOOST_TEST(world.IsHarborAtSea(HarborId(7), sea2));

    const auto& hb1 = createHarbor(HarborId(1));
    // No other harbor
    BOOST_TEST(hb1.GetShipConnections().empty());
    const auto& hb2 = createHarbor(HarborId(2));
    // Different seas
    BOOST_TEST(hb1.GetShipConnections().empty());
    BOOST_TEST(hb2.GetShipConnections().empty());
    using harbors_t = std::set<const noRoadNode*>;
    // 3 and 1 share sea
    const auto& hb3 = createHarbor(HarborId(3));
    BOOST_TEST(toHarbors(hb1.GetShipConnections()) == harbors_t{&hb3});
    BOOST_TEST(hb2.GetShipConnections().empty());
    BOOST_TEST(toHarbors(hb3.GetShipConnections()) == harbors_t{&hb1});
    // 2,4,5 share sea
    const auto& hb4 = createHarbor(HarborId(4));
    const auto& hb5 = createHarbor(HarborId(5));
    BOOST_TEST(toHarbors(hb1.GetShipConnections()) == harbors_t{&hb3});
    BOOST_TEST(toHarbors(hb2.GetShipConnections()) == (harbors_t{&hb4, &hb5}));
    BOOST_TEST(toHarbors(hb3.GetShipConnections()) == harbors_t{&hb1});
    BOOST_TEST(toHarbors(hb4.GetShipConnections()) == (harbors_t{&hb2, &hb5}));
    BOOST_TEST(toHarbors(hb5.GetShipConnections()) == (harbors_t{&hb2, &hb4}));
}

BOOST_FIXTURE_TEST_CASE(ShipBuilding, SeaWorldWithGCExecution<>)
{
    addStartResources();
    initGameRNG();

    const GamePlayer& player = world.GetPlayer(curPlayer);
    const MapPoint hqPos = player.GetHQPos();
    const MapPoint hqFlagPos = world.GetNeighbour(hqPos, Direction::SouthEast);
    const HarborId hbId(1);
    const MapPoint hbPos = world.GetHarborPoint(hbId);
    const MapPoint shipyardPos(hqPos.x + 3, hqPos.y - 5);

    auto* harbor = dynamic_cast<nobHarborBuilding*>(
      BuildingFactory::CreateBuilding(world, BuildingType::HarborBuilding, hbPos, curPlayer, Nation::Romans));
    BOOST_TEST_REQUIRE(harbor);
    std::vector<Direction> road = FindRoadPath(hqFlagPos, world.GetNeighbour(hbPos, Direction::SouthEast), world);
    BOOST_TEST_REQUIRE(!road.empty());
    this->BuildRoad(hqFlagPos, false, road);
    MapPoint curPt = hqFlagPos;
    for(auto i : road)
    {
        curPt = world.GetNeighbour(curPt, i);
        this->SetFlag(curPt);
    }
    BOOST_TEST_REQUIRE(world.GetBQ(shipyardPos, curPlayer) == BuildingQuality::Castle);
    auto* shipYard = dynamic_cast<nobShipYard*>(
      BuildingFactory::CreateBuilding(world, BuildingType::Shipyard, shipyardPos, curPlayer, Nation::Romans));
    BOOST_TEST_REQUIRE(shipYard);
    road = FindRoadPath(hqFlagPos, world.GetNeighbour(shipyardPos, Direction::SouthEast), world);
    BOOST_TEST_REQUIRE(!road.empty());
    this->BuildRoad(hqFlagPos, false, road);
    BOOST_TEST_REQUIRE(shipYard->GetMode() == nobShipYard::Mode::Boats); //-V522
    this->SetShipYardMode(shipyardPos, false);
    BOOST_TEST_REQUIRE(shipYard->GetMode() == nobShipYard::Mode::Boats);
    this->SetShipYardMode(shipyardPos, true);
    BOOST_TEST_REQUIRE(shipYard->GetMode() == nobShipYard::Mode::Ships);
    this->SetShipYardMode(shipyardPos, true);
    BOOST_TEST_REQUIRE(shipYard->GetMode() == nobShipYard::Mode::Ships);
    this->SetShipYardMode(shipyardPos, false);
    BOOST_TEST_REQUIRE(shipYard->GetMode() == nobShipYard::Mode::Boats);
    this->SetShipYardMode(shipyardPos, true);
    BOOST_TEST_REQUIRE(shipYard->GetMode() == nobShipYard::Mode::Ships);

    world.GetPostMgr().AddPostBox(curPlayer);
    PostBox& postBox = *world.GetPostMgr().GetPostBox(curPlayer);
    postBox.Clear();
    // Ship building takes 10 steps with ~500 GFs each. +600 GF to let wares and people reach the site
    RTTR_EXEC_TILL(5600, postBox.GetNumMsgs() > 0);
    // There should be a msg telling the player about the new ship
    BOOST_TEST_REQUIRE(postBox.GetNumMsgs() == 1u);
    const auto* msg = dynamic_cast<const ShipPostMsg*>(postBox.GetMsg(0));
    BOOST_TEST_REQUIRE(msg);
    BOOST_TEST_REQUIRE(player.GetNumShips() == 1u);
    BOOST_TEST_REQUIRE(player.GetShips().size() == 1u);
    noShip* ship = player.GetShipByID(0);
    BOOST_TEST_REQUIRE(ship);
    BOOST_TEST_REQUIRE(player.GetShipID(*ship) == 0u);
}

template<unsigned T_numPlayers = 3, unsigned T_hbId = 1, unsigned T_width = SeaWorldDefault::width,
         unsigned T_height = SeaWorldDefault::height>
struct ShipReadyFixture : public SeaWorldWithGCExecution<T_numPlayers, T_width, T_height>
{
    using Parent = SeaWorldWithGCExecution<T_numPlayers, T_width, T_height>;
    using Parent::curPlayer;
    using Parent::world;

    PostBox* postBox;
    ShipReadyFixture()
    {
        GamePlayer& player = world.GetPlayer(curPlayer);
        const MapPoint hqPos = player.GetHQPos();
        const MapPoint hqFlagPos = world.GetNeighbour(hqPos, Direction::SouthEast);
        const MapPoint hbPos = world.GetHarborPoint(HarborId(T_hbId));
        world.GetPostMgr().AddPostBox(curPlayer);
        postBox = world.GetPostMgr().GetPostBox(curPlayer);

        auto* harbor = dynamic_cast<nobHarborBuilding*>(
          BuildingFactory::CreateBuilding(world, BuildingType::HarborBuilding, hbPos, curPlayer, Nation::Romans));
        BOOST_TEST_REQUIRE(harbor);
        world.RecalcBQAroundPointBig(hbPos);
        std::vector<Direction> road = FindRoadPath(hqFlagPos, world.GetNeighbour(hbPos, Direction::SouthEast), world);
        BOOST_TEST_REQUIRE(!road.empty());
        this->BuildRoad(hqFlagPos, false, road);
        MapPoint curPt = hqFlagPos;
        for(auto& i : road)
        {
            curPt = world.GetNeighbour(curPt, i);
            this->SetFlag(curPt);
        }

        MapPoint shipPos(hbPos.x, hbPos.y - 3);
        if(!world.IsSeaPoint(shipPos))
            shipPos.y += 6;
        BOOST_TEST_REQUIRE(world.IsSeaPoint(shipPos));
        auto& ship = world.AddFigure(shipPos, std::make_unique<noShip>(shipPos, curPlayer));
        player.RegisterShip(ship);

        BOOST_TEST_REQUIRE(player.GetNumShips() == 1u);
        postBox->Clear();
        initGameRNG();
    }

    ShipDirection GetShipDir(const noShip& ship, HarborId targetHbId) const
    {
        for(const auto dir : helpers::enumRange<ShipDirection>())
        {
            if(world.GetNextFreeHarborPoint(ship.GetPos(), ship.GetCurrentHarbor(), dir, ship.GetPlayerId())
               == targetHbId)
                return dir;
        }
        BOOST_TEST_FAIL("No dir found");                    // LCOV_EXCL_LINE
        BOOST_UNREACHABLE_RETURN(ShipDirection::NorthWest); // LCOV_EXCL_LINE
    }
};

BOOST_FIXTURE_TEST_CASE(ExplorationExpedition, ShipReadyFixture<>)
{
    curPlayer = 0;
    addStartResources();
    const GamePlayer& player = world.GetPlayer(curPlayer);
    const noShip& ship = ensureNonNull(player.GetShipByID(0));
    const nobHarborBuilding& harbor = *player.GetBuildingRegister().GetHarbors().front();
    const MapPoint hbPos = harbor.GetPos();
    const HarborId hbId = world.GetHarborPointID(hbPos);
    BOOST_TEST_REQUIRE(ship.IsIdling());
    BOOST_TEST_REQUIRE(!harbor.IsExplorationExpeditionActive());
    this->StartStopExplorationExpedition(hbPos, true);
    BOOST_TEST_REQUIRE(harbor.IsExplorationExpeditionActive());
    // Expedition not ready, ship still idling
    BOOST_TEST_REQUIRE(ship.IsIdling());

    // Wait till scouts arrive
    RTTR_EXEC_TILL(380, !ship.IsIdling());
    BOOST_TEST_REQUIRE(ship.IsGoingToHarbor(harbor));
    BOOST_TEST_REQUIRE(ship.GetTargetHarbor() == hbId);
    BOOST_TEST_REQUIRE(player.GetShipsToHarbor(harbor) == 1u);

    // No available scouts
    BOOST_TEST_REQUIRE(harbor.GetNumRealFigures(Job::Scout) == 0u);
    // Stop it
    this->StartStopExplorationExpedition(hbPos, true);
    BOOST_TEST_REQUIRE(harbor.IsExplorationExpeditionActive());
    this->StartStopExplorationExpedition(hbPos, false);
    BOOST_TEST_REQUIRE(!harbor.IsExplorationExpeditionActive());
    this->StartStopExplorationExpedition(hbPos, false);
    BOOST_TEST_REQUIRE(!harbor.IsExplorationExpeditionActive());
    // Scouts available again
    BOOST_TEST_REQUIRE(harbor.GetNumRealFigures(Job::Scout) == 3u);

    // Let ship arrive
    RTTR_EXEC_TILL(180, ship.IsIdling());
    BOOST_TEST_REQUIRE(!ship.IsGoingToHarbor(harbor));
    BOOST_TEST_REQUIRE(player.GetShipsToHarbor(harbor) == 0u);
    BOOST_TEST_REQUIRE(!ship.GetTargetHarbor());
    BOOST_TEST_REQUIRE(!ship.GetHomeHarbor());

    // We want the ship to only scout unexplored harbors, so set all but one to visible
    world.GetNodeWriteable(world.GetHarborPoint(HarborId(6))).fow[curPlayer].visibility = Visibility::Visible; //-V807
    // Team visibility, so set one to own team
    world.GetPlayer(curPlayer).team = Team::Team1;
    world.GetPlayer(1).team = Team::Team1;
    world.GetPlayer(curPlayer).MakeStartPacts();
    world.GetPlayer(1).MakeStartPacts();
    world.GetNodeWriteable(world.GetHarborPoint(HarborId(3))).fow[1].visibility = Visibility::Visible;
    HarborId targetHbId(8);

    // Start again (everything is here)
    this->StartStopExplorationExpedition(hbPos, true);
    // ...so we can start right now
    BOOST_TEST_REQUIRE(ship.IsOnExplorationExpedition());
    // Load and start
    RTTR_SKIP_GFS(202);
    BOOST_TEST_REQUIRE(ship.GetHomeHarbor() == hbId);
    BOOST_TEST_REQUIRE(ship.GetTargetHarbor() == targetHbId);

    unsigned distance = world.CalcDistance(hbPos, world.GetHarborPoint(targetHbId));
    // Let the ship scout the harbor
    RTTR_EXEC_TILL(distance * 2 * 20, !ship.IsMoving());
    BOOST_TEST_REQUIRE(ship.IsOnExplorationExpedition());
    BOOST_TEST_REQUIRE(world.CalcDistance(world.GetHarborPoint(targetHbId), ship.GetPos()) <= 2u);
    // Now the ship waits and will select the next harbor. We allow another one:
    HarborId nextHbId(6);
    world.GetNodeWriteable(world.GetHarborPoint(nextHbId)).fow[curPlayer].visibility = Visibility::FogOfWar;
    RTTR_EXEC_TILL(350, ship.IsMoving());
    BOOST_TEST_REQUIRE(ship.GetHomeHarbor() == hbId);
    BOOST_TEST_REQUIRE(ship.GetTargetHarbor() == nextHbId);
    distance = world.CalcDistance(ship.GetPos(), world.GetHarborPoint(nextHbId));
    // Let the ship scout the harbor
    RTTR_EXEC_TILL(distance * 2 * 20, !ship.IsMoving());
    BOOST_TEST_REQUIRE(ship.IsOnExplorationExpedition());
    BOOST_TEST_REQUIRE(world.CalcDistance(world.GetHarborPoint(nextHbId), ship.GetPos()) <= 2u);

    // Now disallow the first harbor so ship returns home
    world.GetNodeWriteable(world.GetHarborPoint(targetHbId)).fow[curPlayer].visibility = Visibility::Visible;

    RTTR_EXEC_TILL(350, ship.IsMoving());
    BOOST_TEST_REQUIRE(ship.GetHomeHarbor() == hbId);
    BOOST_TEST_REQUIRE(ship.GetTargetHarbor() == hbId);

    distance = world.CalcDistance(ship.GetPos(), world.GetHarborPoint(hbId));
    // And at some time it should return home
    RTTR_EXEC_TILL(distance * 2 * 20 + 200, ship.IsIdling());
    const SeaId seaId(1);
    BOOST_TEST_REQUIRE(ship.GetSeaID() == seaId);
    BOOST_TEST_REQUIRE(ship.GetPos() == world.GetCoastalPoint(hbId, seaId));

    // Now try to start an expedition but all harbors are explored -> Load, Unload, Idle
    world.GetNodeWriteable(world.GetHarborPoint(nextHbId)).fow[curPlayer].visibility = Visibility::Visible;
    this->StartStopExplorationExpedition(hbPos, true);
    BOOST_TEST_REQUIRE(ship.IsOnExplorationExpedition());
    RTTR_EXEC_TILL(2 * 200 + 5, ship.IsIdling());
}

BOOST_FIXTURE_TEST_CASE(DestroyHomeOnExplExp, ShipReadyFixture<2>)
{
    addStartResources();
    curPlayer = 0;
    const GamePlayer& player = world.GetPlayer(curPlayer);
    const noShip& ship = ensureNonNull(player.GetShipByID(0));
    const nobHarborBuilding& harbor = *player.GetBuildingRegister().GetHarbors().front();
    const MapPoint hbPos = harbor.GetPos();
    const HarborId hbId = world.GetHarborPointID(hbPos);
    unsigned numScouts = player.GetInventory()[Job::Scout]; //-V807
    BOOST_TEST_REQUIRE(ship.IsIdling());

    // We want the ship to only scout unexplored harbors, so set all but one to visible
    world.GetPlayer(curPlayer).team = Team::Team1;
    world.GetPlayer(1).team = Team::Team1;
    world.GetPlayer(curPlayer).MakeStartPacts();
    world.GetPlayer(1).MakeStartPacts();

    world.GetNodeWriteable(world.GetHarborPoint(HarborId(6))).fow[1].visibility = Visibility::Visible;
    world.GetNodeWriteable(world.GetHarborPoint(HarborId(3))).fow[1].visibility = Visibility::Visible;
    HarborId targetHbId(8);
    this->StartStopExplorationExpedition(hbPos, true);

    // Start it
    RTTR_EXEC_TILL(600, ship.IsOnExplorationExpedition() && ship.IsMoving());
    // Incorporate recruitment
    BOOST_TEST_REQUIRE(player.GetInventory()[Job::Scout] >= numScouts);
    numScouts = player.GetInventory()[Job::Scout];

    BOOST_TEST_REQUIRE(ship.GetHomeHarbor() == hbId);
    BOOST_TEST_REQUIRE(ship.GetTargetHarbor() == targetHbId);

    // Run till ship is coming back
    RTTR_EXEC_TILL(1000, ship.GetTargetHarbor() == hbId);
    // Avoid that it goes back to that point
    world.GetNodeWriteable(world.GetHarborPoint(targetHbId)).fow[1].visibility = Visibility::Visible;

    // Destroy home harbor
    world.DestroyNO(hbPos);
    RTTR_EXEC_TILL(2000, ship.IsLost());
    BOOST_TEST_REQUIRE(!ship.IsMoving());

    MapPoint newHbPos = world.GetHarborPoint(HarborId(6));
    auto* newHarbor = dynamic_cast<nobHarborBuilding*>(
      BuildingFactory::CreateBuilding(world, BuildingType::HarborBuilding, newHbPos, curPlayer, Nation::Romans));

    BOOST_TEST_REQUIRE(!ship.IsLost());
    BOOST_TEST_REQUIRE(ship.IsMoving());
    RTTR_EXEC_TILL(1200, ship.IsIdling());
    BOOST_TEST(player.GetInventory()[Job::Scout] == numScouts);
    BOOST_TEST(newHarbor->GetNumRealFigures(Job::Scout) == newHarbor->GetNumVisualFigures(Job::Scout)); //-V522
    BOOST_TEST(newHarbor->GetNumRealFigures(Job::Scout)
                 + world.GetSpecObj<nobBaseWarehouse>(player.GetHQPos())->GetNumRealFigures(Job::Scout)
               == numScouts);
}

BOOST_FIXTURE_TEST_CASE(Expedition, ShipReadyFixture<>)
{
    addStartResources();
    const GamePlayer& player = world.GetPlayer(curPlayer);
    const noShip& ship = ensureNonNull(player.GetShipByID(0));
    const nobHarborBuilding& harbor = *player.GetBuildingRegister().GetHarbors().front();
    const MapPoint hbPos = harbor.GetPos();
    BOOST_TEST_REQUIRE(ship.IsIdling());
    BOOST_TEST_REQUIRE(!harbor.IsExpeditionActive());
    this->StartStopExpedition(hbPos, true);
    BOOST_TEST_REQUIRE(harbor.IsExpeditionActive());
    // Expedition not ready, ship still idling
    BOOST_TEST_REQUIRE(ship.IsIdling());

    // Wait till wares arrive
    RTTR_EXEC_TILL(3400, !ship.IsIdling());
    // Expedition ready, ship ordered
    BOOST_TEST_REQUIRE(ship.IsGoingToHarbor(harbor));
    BOOST_TEST_REQUIRE(player.GetShipsToHarbor(harbor) == 1u);

    // No available boards
    BOOST_TEST_REQUIRE(harbor.GetNumRealWares(GoodType::Boards) == 0u);
    // Stop it
    this->StartStopExpedition(hbPos, true);
    BOOST_TEST_REQUIRE(harbor.IsExpeditionActive());
    this->StartStopExpedition(hbPos, false);
    BOOST_TEST_REQUIRE(!harbor.IsExpeditionActive());
    this->StartStopExpedition(hbPos, false);
    BOOST_TEST_REQUIRE(!harbor.IsExpeditionActive());
    // Boards available again
    BOOST_TEST_REQUIRE(harbor.GetNumRealWares(GoodType::Boards) > 0u);

    // Let ship arrive
    RTTR_EXEC_TILL(180, ship.IsIdling());
    BOOST_TEST_REQUIRE(!ship.IsGoingToHarbor(harbor));
    BOOST_TEST_REQUIRE(player.GetShipsToHarbor(harbor) == 0u);

    // Start again (everything is here)
    this->StartStopExpedition(hbPos, true);
    // ...so we can start right now
    BOOST_TEST_REQUIRE(ship.IsOnExpedition());

    // Wait for ship to be "loaded"
    RTTR_EXEC_TILL(200, ship.IsWaitingForExpeditionInstructions());
    // Ship should be waiting for expedition instructions (where to go) and player should have received a message
    BOOST_TEST_REQUIRE(ship.GetCurrentHarbor() == harbor.GetHarborPosID());
    BOOST_TEST_REQUIRE(postBox->GetNumMsgs() == 1u);
    const auto* msg = dynamic_cast<const ShipPostMsg*>(postBox->GetMsg(0));
    BOOST_TEST_REQUIRE(msg);
    BOOST_TEST_REQUIRE(msg->GetPos() == ship.GetPos()); //-V522

    // Harbor pos taken by other player
    this->TravelToNextSpot(ShipDirection::SouthEast, player.GetShipID(ship));
    BOOST_TEST_REQUIRE(ship.IsWaitingForExpeditionInstructions());

    // Last free one (far south -> North is closer)
    this->TravelToNextSpot(ShipDirection::North, player.GetShipID(ship));
    BOOST_TEST_REQUIRE(!ship.IsWaitingForExpeditionInstructions());
    BOOST_TEST_REQUIRE(ship.IsMoving());

    unsigned gfsToDest;
    RTTR_EXEC_TILL_CT_GF(1000, ship.IsWaitingForExpeditionInstructions(), gfsToDest);
    HarborId hbId(8);
    BOOST_TEST_REQUIRE(ship.GetCurrentHarbor() == hbId);
    BOOST_TEST_REQUIRE(world.CalcDistance(ship.GetPos(), world.GetHarborPoint(hbId)) == 1u);

    // Cancel expedition -> Ship is going back to harbor
    this->CancelExpedition(player.GetShipID(ship));
    BOOST_TEST_REQUIRE(ship.IsMoving());
    // Let ship arrive and unload
    RTTR_EXEC_TILL(gfsToDest + 300, ship.IsIdling());

    // Start again (everything is here)
    this->StartStopExpedition(hbPos, true);
    BOOST_TEST_REQUIRE(ship.IsOnExpedition());

    // Wait for ship to be "loaded"
    RTTR_EXEC_TILL(200, ship.IsWaitingForExpeditionInstructions());
    BOOST_TEST_REQUIRE(ship.GetCurrentHarbor() == harbor.GetHarborPosID());

    // Try to found colony -> Fail
    this->FoundColony(player.GetShipID(ship));
    BOOST_TEST_REQUIRE(ship.IsWaitingForExpeditionInstructions());
    // Go back to free spot
    this->TravelToNextSpot(ShipDirection::North, player.GetShipID(ship));
    BOOST_TEST_REQUIRE(!ship.IsWaitingForExpeditionInstructions());
    for(unsigned gf = 0; gf < 2; gf++)
    {
        // Send commands that should be ignored as ship is not expecting them
        this->FoundColony(player.GetShipID(ship));
        BOOST_TEST_REQUIRE(ship.IsMoving());
        this->TravelToNextSpot(ShipDirection::South, player.GetShipID(ship));
        this->CancelExpedition(player.GetShipID(ship));
        BOOST_TEST_REQUIRE(ship.IsMoving());
        this->em.ExecuteNextGF();
    }
    // Go to destination
    postBox->Clear();
    RTTR_SKIP_GFS(gfsToDest);
    BOOST_TEST_REQUIRE(ship.IsWaitingForExpeditionInstructions());
    // This should again trigger a message
    msg = dynamic_cast<const ShipPostMsg*>(postBox->GetMsg(0));
    BOOST_TEST_REQUIRE(msg);
    BOOST_TEST_REQUIRE(msg->GetPos() == ship.GetPos());
    // And now we can found a new colony
    this->FoundColony(player.GetShipID(ship));
    // Ship is free again
    BOOST_TEST_REQUIRE(ship.IsIdling());
    const noBuildingSite* newHarbor = world.GetSpecObj<noBuildingSite>(world.GetHarborPoint(hbId));
    BOOST_TEST_REQUIRE(newHarbor);
    BOOST_TEST_REQUIRE(world.IsHarborBuildingSiteFromSea(newHarbor));
    // And it should be completed after some time
    RTTR_EXEC_TILL(5000, player.GetBuildingRegister().GetHarbors().size() > 1); //-V807
    BOOST_TEST_REQUIRE(player.GetBuildingRegister().GetHarbors().size() == 2u);
}

using ShipReadyFixtureBig = ShipReadyFixture<1, 2, 64, 800>;

BOOST_FIXTURE_TEST_CASE(LongDistanceTravel, ShipReadyFixtureBig)
{
    const GamePlayer& player = world.GetPlayer(curPlayer);
    const noShip& ship = ensureNonNull(player.GetShipByID(0));
    nobHarborBuilding& harbor = *player.GetBuildingRegister().GetHarbors().front();
    const MapPoint hbPos = harbor.GetPos();
    // Go to opposite one
    const HarborId targetHbId(7);
    // Make sure that the other harbor is far away
    BOOST_TEST_REQUIRE(world.CalcHarborDistance(HarborId(2), targetHbId) > 600u);
    // Add some scouts
    harbor.AddToInventory(PeopleCounts::make(Job::Scout, 20), true);
    // We want the ship to only scout unexplored harbors, so set all but one to visible
    for(const auto i : helpers::idRange<HarborId>(world.GetNumHarborPoints()))
        world.GetNodeWriteable(world.GetHarborPoint(i)).fow[curPlayer].visibility = Visibility::Visible;
    world.GetNodeWriteable(world.GetHarborPoint(targetHbId)).fow[curPlayer].visibility = Visibility::Invisible;
    // Start an exploration expedition
    this->StartStopExplorationExpedition(hbPos, true);
    BOOST_TEST_REQUIRE(harbor.IsExplorationExpeditionActive());
    // Wait till ship has arrived and starts loading
    RTTR_EXEC_TILL(100, ship.IsOnExplorationExpedition());
    // Wait till ship has loaded scouts
    RTTR_EXEC_TILL(200, ship.IsMoving());
    BOOST_TEST_REQUIRE(ship.GetTargetHarbor() == targetHbId);
}

namespace {
template<unsigned T_numPlayers = 2, unsigned T_width = SmallSeaWorldDefault<T_numPlayers>::width,
         unsigned T_height = SmallSeaWorldDefault<T_numPlayers>::height>
struct ShipAndHarborsReadyFixture :
    public WorldFixture<CreateWaterWorld, T_numPlayers, T_width, T_height>,
    public GCExecutor
{
public:
    using Parent = WorldFixture<CreateWaterWorld, T_numPlayers, T_width, T_height>;
    using Parent::world;

    GameWorld& GetWorld() override { return world; }

    nobHarborBuilding& createHarbor(HarborId hbPosId)
    {
        MapPoint hbPos = world.GetHarborPoint(hbPosId);
        auto* harbor = static_cast<nobHarborBuilding*>(
          BuildingFactory::CreateBuilding(world, BuildingType::HarborBuilding, hbPos, curPlayer, Nation::Romans));
        BOOST_TEST_REQUIRE(harbor);
        GoodsAndPeopleCounts goods;
        goods[GoodType::Wood] = 10;
        goods[Job::Woodcutter] = 10;
        harbor->AddToInventory(goods, true);
        return *harbor;
    }

    ShipAndHarborsReadyFixture()
    {
        GamePlayer& player = world.GetPlayer(curPlayer);
        createHarbor(HarborId(1));
        createHarbor(HarborId(2));

        MapPoint hbPos = world.GetHarborPoint(HarborId(1));
        MapPoint shipPos = world.MakeMapPoint(hbPos - Position(2, 0));
        BOOST_TEST_REQUIRE(world.IsSeaPoint(shipPos));
        auto& ship = world.AddFigure(shipPos, std::make_unique<noShip>(shipPos, curPlayer));
        player.RegisterShip(ship);

        BOOST_TEST_REQUIRE(player.GetNumShips() == 1u);
    }
};

void destroyBldAndFire(GameWorldBase& world, const MapPoint& pos)
{
    world.DestroyNO(pos);
    // Remove fire
    world.DestroyNO(pos);
    // Remove burned wh if existing
    for(noBase& fig : world.GetFigures(pos))
    {
        if(fig.GetGOT() == GO_Type::Burnedwarehouse)
        {
            // Remove go-out event (not automatically done as the burned wh is never removed)
            const GameEvent* ev = static_cast<TestEventManager&>(world.GetEvMgr()).GetObjEvents(fig).front();
            world.GetEvMgr().RemoveEvent(ev);
            world.RemoveFigure(pos, fig)->Destroy();
            break;
        }
    }
}
} // namespace

BOOST_FIXTURE_TEST_CASE(HarborDestroyed, ShipAndHarborsReadyFixture<1>)
{
    const GamePlayer& player = world.GetPlayer(curPlayer);
    const noShip& ship = *player.GetShipByID(0);
    constexpr HarborId hb1Id(1);
    constexpr HarborId hb2Id(2);
    constexpr HarborId hb3Id(3);
    const MapPoint hb1Pos = world.GetHarborPoint(hb1Id);
    const MapPoint hb2Pos = world.GetHarborPoint(hb2Id);
    const MapPoint hb3Pos = world.GetHarborPoint(hb3Id);

    // Order goods
    SetInventorySetting(hb2Pos, GoodType::Wood, EInventorySetting::Collect);
    SetInventorySetting(hb2Pos, Job::Woodcutter, EInventorySetting::Collect);

    // Destroy home before load -> Abort after ship reaches harbor
    RTTR_EXEC_TILL(90, ship.IsMoving());
    destroyBldAndFire(world, hb1Pos);
    RTTR_EXEC_TILL(10, !ship.IsMoving());
    BOOST_TEST_REQUIRE(ship.IsIdling());

    // Destroy home during load -> Continue
    createHarbor(hb1Id);
    // Just wait for re-order event and instantly go to loading as ship does not need to move
    RTTR_EXEC_TILL(90, ship.IsLoading());
    destroyBldAndFire(world, hb1Pos);
    BOOST_TEST_REQUIRE(ship.IsLoading());
    BOOST_TEST_REQUIRE(!ship.GetHomeHarbor());
    RTTR_EXEC_TILL(300, ship.IsUnloading());
    constexpr SeaId seaId(1);
    BOOST_TEST_REQUIRE(ship.GetPos() == world.GetCoastalPoint(hb2Id, seaId));
    RTTR_EXEC_TILL(200, ship.IsIdling());

    // Destroy destination before load -> Abort after ship reaches harbor
    createHarbor(hb1Id);
    RTTR_EXEC_TILL(90, ship.IsMoving());
    destroyBldAndFire(world, hb2Pos);
    RTTR_EXEC_TILL(200, !ship.IsMoving());
    BOOST_TEST_REQUIRE(ship.IsIdling());

    // Destroy destination during load -> Unload again
    createHarbor(hb2Id);
    // Order goods
    SetInventorySetting(hb2Pos, GoodType::Wood, EInventorySetting::Collect);
    SetInventorySetting(hb2Pos, Job::Woodcutter, EInventorySetting::Collect);
    RTTR_EXEC_TILL(300, ship.IsLoading());
    destroyBldAndFire(world, hb2Pos);
    BOOST_TEST_REQUIRE(ship.IsUnloading());
    RTTR_EXEC_TILL(200, ship.IsIdling());

    // Destroy destination during driving -> Go back and unload
    createHarbor(hb2Id);
    // Order goods
    SetInventorySetting(hb2Pos, GoodType::Wood, EInventorySetting::Collect);
    SetInventorySetting(hb2Pos, Job::Woodcutter, EInventorySetting::Collect);
    RTTR_EXEC_TILL(300, ship.IsLoading());
    RTTR_EXEC_TILL(200, ship.IsMoving());
    BOOST_TEST_REQUIRE(ship.GetHomeHarbor() == hb1Id);
    BOOST_TEST_REQUIRE(ship.GetTargetHarbor() == hb2Id);
    destroyBldAndFire(world, hb2Pos);
    RTTR_EXEC_TILL(10, ship.GetTargetHarbor() == hb1Id);
    RTTR_EXEC_TILL(20, ship.IsUnloading());
    BOOST_TEST_REQUIRE(ship.GetPos() == world.GetCoastalPoint(hb1Id, seaId));
    RTTR_EXEC_TILL(200, ship.IsIdling());

    // Destroy destination during unloading -> Go back and unload
    createHarbor(hb2Id);
    // Order goods
    SetInventorySetting(hb2Pos, GoodType::Wood, EInventorySetting::Collect);
    SetInventorySetting(hb2Pos, Job::Woodcutter, EInventorySetting::Collect);
    RTTR_EXEC_TILL(700, ship.IsUnloading());
    BOOST_TEST_REQUIRE(ship.GetHomeHarbor() == hb1Id);
    BOOST_TEST_REQUIRE(ship.GetTargetHarbor() == hb2Id);
    destroyBldAndFire(world, hb2Pos);
    RTTR_EXEC_TILL(10, ship.GetTargetHarbor() == hb1Id);
    RTTR_EXEC_TILL(200, ship.IsUnloading());
    BOOST_TEST_REQUIRE(ship.GetPos() == world.GetCoastalPoint(hb1Id, seaId));
    RTTR_EXEC_TILL(200, ship.IsIdling());

    // Destroy both during unloading -> Go to 3rd and unload
    createHarbor(hb2Id);
    createHarbor(hb3Id);
    // Order goods
    SetInventorySetting(hb2Pos, GoodType::Wood, EInventorySetting::Collect);
    SetInventorySetting(hb2Pos, Job::Woodcutter, EInventorySetting::Collect);
    RTTR_EXEC_TILL(700, ship.IsUnloading());
    BOOST_TEST_REQUIRE(ship.GetHomeHarbor() == hb1Id);
    BOOST_TEST_REQUIRE(ship.GetTargetHarbor() == hb2Id);
    destroyBldAndFire(world, hb1Pos);
    destroyBldAndFire(world, hb2Pos);
    RTTR_EXEC_TILL(10, ship.GetTargetHarbor() == HarborId(3));
    // Destroy last ->
    destroyBldAndFire(world, hb3Pos);
    RTTR_EXEC_TILL(10, ship.IsLost());
    BOOST_TEST_REQUIRE(!ship.GetHomeHarbor());
    BOOST_TEST_REQUIRE(!ship.GetTargetHarbor());
    createHarbor(hb1Id);
    BOOST_TEST_REQUIRE(ship.IsMoving());
    BOOST_TEST_REQUIRE(ship.GetHomeHarbor() == hb1Id);
    BOOST_TEST_REQUIRE(ship.GetTargetHarbor() == hb1Id);
}

struct MockWare : Ware
{
    static bool destroyed;
    MockWare(GoodType type, noBaseBuilding* goal, noRoadNode* location) : Ware(type, goal, location)
    {
        destroyed = false;
    }
    ~MockWare() { destroyed = true; }
};
bool MockWare::destroyed = false;

BOOST_FIXTURE_TEST_CASE(AddWareWithUnreachableGoalToHarbor, ShipAndHarborsReadyFixture<1>)
{
    // Recreate issue sensitive to timing:
    // 1. Ware almost at harbor flag and should be shipped -> will be carried into building
    // 2. Goal becomes unreachable (e.g. road removed)
    // 3. Ware reaches harbor and needs to be carried into a storehouse
    // 4. Harbor itself does not accept the ware so another storehouse must be found
    GamePlayer& player = world.GetPlayer(curPlayer);
    const auto& harbors = player.GetBuildingRegister().GetHarbors();
    BOOST_TEST_REQUIRE(harbors.size() >= 2u);
    nobHarborBuilding& harbor1 = *harbors.front();
    nobHarborBuilding& harbor2 = **(++harbors.begin());
    auto* wh = static_cast<nobBaseWarehouse*>(BuildingFactory::CreateBuilding(
      world, BuildingType::Storehouse, harbor2.GetPos() + MapPoint(2, 0), curPlayer, Nation::Romans));
    BOOST_TEST_REQUIRE(wh);
    BuildRoadForBlds(harbor2.GetPos(), wh->GetPos());
    constexpr auto goodType = GoodType::Wood;

    // Sanity check: Move from harbor1 to wh (over harbor2)
    auto ware = std::make_unique<MockWare>(goodType, wh, &harbor1);
    auto* warePtr = ware.get();
    ware->Carry(&harbor1);
    auto numVisWaresBefore = harbor1.GetNumVisualWares(goodType);
    auto numRealWaresBefore = harbor1.GetNumRealWares(goodType);
    harbor1.AddWare(std::move(ware));
    BOOST_TEST(harbor1.GetNumVisualWares(goodType) == numVisWaresBefore + 1u);
    // Wares that will be moved out are only in visual count
    BOOST_TEST(harbor1.GetNumRealWares(goodType) == numRealWaresBefore);
    BOOST_TEST_REQUIRE(!MockWare::destroyed);
    BOOST_TEST(warePtr->GetGoal() == wh);
    BOOST_TEST(warePtr->GetNextDir() == RoadPathDirection::Ship);

    // Add ware with unreachable goal -> Added to harbor
    // First fully disconnect 2nd harbor -> Only reachable via ship
    for(const auto d : helpers::enumRange<Direction>())
    {
        if(d != Direction::NorthWest)
            DestroyRoad(harbor2.GetFlagPos(), d);
    }
    ware = std::make_unique<MockWare>(goodType, wh, &harbor1);
    ware->Carry(&harbor1);
    numVisWaresBefore = harbor1.GetNumVisualWares(goodType);
    numRealWaresBefore = harbor1.GetNumRealWares(goodType);
    harbor1.AddWare(std::move(ware));
    BOOST_TEST(harbor1.GetNumVisualWares(goodType) == numVisWaresBefore + 1u);
    BOOST_TEST(harbor1.GetNumRealWares(goodType) == numRealWaresBefore + 1u);
    BOOST_TEST(MockWare::destroyed);

    // Same but disallow harbor as goal -> Find another storehouse
    SetInventorySetting(harbor1.GetPos(), goodType, EInventorySetting::Stop);
    ware = std::make_unique<MockWare>(goodType, wh, &harbor1);
    ware->Carry(&harbor1);
    warePtr = ware.get();
    numVisWaresBefore = harbor1.GetNumVisualWares(goodType);
    numRealWaresBefore = harbor1.GetNumRealWares(goodType);
    harbor1.AddWare(std::move(ware));
    BOOST_TEST(harbor1.GetNumVisualWares(goodType) == numVisWaresBefore + 1u);
    BOOST_TEST(harbor1.GetNumRealWares(goodType) == numRealWaresBefore);
    BOOST_TEST_REQUIRE(!MockWare::destroyed);
    BOOST_TEST(warePtr->GetGoal() == &harbor2);

    // If no other storehouse is available take it anyway
    SetInventorySetting(harbor2.GetPos(), goodType, EInventorySetting::Stop);
    ware = std::make_unique<MockWare>(goodType, wh, &harbor1);
    ware->Carry(&harbor1);
    numVisWaresBefore = harbor1.GetNumVisualWares(goodType);
    numRealWaresBefore = harbor1.GetNumRealWares(goodType);
    harbor1.AddWare(std::move(ware));
    BOOST_TEST(harbor1.GetNumVisualWares(goodType) == numVisWaresBefore + 1u);
    BOOST_TEST(harbor1.GetNumRealWares(goodType) == numRealWaresBefore + 1u);
    BOOST_TEST(MockWare::destroyed);
}

BOOST_FIXTURE_TEST_CASE(GoToNeighborHarbor, ShipReadyFixture<1>)
{
    const GamePlayer& player = world.GetPlayer(curPlayer);
    const noShip& ship = ensureNonNull(player.GetShipByID(0));
    nobHarborBuilding& homeHarbor = *player.GetBuildingRegister().GetHarbors().front();
    const MapPoint homeHbPos = homeHarbor.GetPos();
    GoodsAndPeopleCounts expWares;
    expWares[Job::Builder] = 1;
    expWares[GoodType::Boards] = 20;
    expWares[GoodType::Stones] = 20;
    homeHarbor.AddToInventory(expWares, true);

    /* A ship might get stuck when targeting a harbor (2) SE of the current harbor (1): Issue #1784
       Harbors are at NE coast of island, (1) directly at sea, (2) one node inland:
      ________
      _____H1 C
      ______H2__
      ____________
     Coastal point of (1) is E, coastal point of (2) is NE -> Same point
    So going to that harbor involves no movement which breaks assumptions.
    */

    // Put the harbors up into the sea so we can control the coastal point
    const MapPoint harborPos = homeHbPos + MapPoint(7, -3);                       // (1)
    const MapPoint nbHbPos = world.GetNeighbour(harborPos, Direction::SouthEast); // (2)
    const MapPoint otherHbPos = world.MakeMapPoint(homeHbPos - Point(3, 0));      // Unrelated harbor
    const std::vector<MapPoint> harborPoints = {harborPos, nbHbPos, otherHbPos};
    for(const MapPoint p : harborPoints)
        world.GetNodeWriteable(p).bq = BuildingQuality::Harbor;
    // These nodes and 3 left of it get land terrain (below)
    const std::array nodes{
      world.GetNeighbour(world.GetNeighbour(harborPos, Direction::NorthEast), Direction::NorthEast),
      world.GetNeighbour(harborPos, Direction::NorthWest), harborPos, nbHbPos};
    for(auto p : nodes)
    {
        for([[maybe_unused]] auto i : helpers::range(2))
        {
            auto& node = world.GetNodeWriteable(p);
            node.t1 = node.t2 = GetLandTerrain(world.GetDescription());
            p = world.GetNeighbour(p, Direction::West);
        }
    }
    const auto coastPoint = world.GetNeighbour(harborPos, Direction::East);
    // And the coastal point
    {
        auto& node = world.GetNodeWriteable(world.GetNeighbour(coastPoint, Direction::NorthWest));
        node.t1 = GetLandTerrain(world.GetDescription());
    }
    MapLoader::InitSeasAndHarbors(world, harborPoints);

    const auto harborId = world.GetHarborPointID(harborPos);
    const auto nbHbId = world.GetHarborPointID(nbHbPos);
    BOOST_TEST_REQUIRE(harborId.isValid());
    BOOST_TEST_REQUIRE(nbHbId.isValid());
    const auto seaId = world.GetSeaId(harborId, Direction::East);
    BOOST_TEST_REQUIRE(seaId.isValid());
    BOOST_TEST_REQUIRE(world.GetCoastalPoint(harborId, seaId) == coastPoint);
    const auto nbSeaId = world.GetSeaId(nbHbId, Direction::NorthEast);
    BOOST_TEST_REQUIRE(nbSeaId == seaId);
    BOOST_TEST_REQUIRE(world.GetCoastalPoint(nbHbId, seaId) == coastPoint);

    this->StartStopExpedition(homeHbPos, true);
    BOOST_TEST_REQUIRE(homeHarbor.IsExpeditionActive());
    // Wait till ship has loaded wares and is ready
    RTTR_EXEC_TILL(500, ship.IsWaitingForExpeditionInstructions());

    this->TravelToNextSpot(this->GetShipDir(ship, harborId), player.GetShipID(ship));
    BOOST_TEST_REQUIRE(!ship.IsWaitingForExpeditionInstructions());
    BOOST_TEST_REQUIRE(ship.IsMoving());
    BOOST_TEST_REQUIRE(ship.GetTargetHarbor() == harborId);

    RTTR_EXEC_TILL(500, ship.IsWaitingForExpeditionInstructions());
    BOOST_TEST_REQUIRE(ship.GetPos() == coastPoint);

    this->TravelToNextSpot(this->GetShipDir(ship, nbHbId), player.GetShipID(ship));
    BOOST_TEST_REQUIRE(ship.GetTargetHarbor() == nbHbId);
    // Already there
    BOOST_TEST(ship.IsWaitingForExpeditionInstructions());
    BOOST_TEST(ship.GetPos() == coastPoint);

    // Can move
    const auto otherHarborId = world.GetHarborPointID(otherHbPos);
    BOOST_TEST_REQUIRE(otherHarborId.isValid());
    // Precondition that any ship dir will be found
    BOOST_TEST_REQUIRE(world.IsHarborAtSea(otherHarborId, seaId));
    this->TravelToNextSpot(this->GetShipDir(ship, otherHarborId), player.GetShipID(ship));
    BOOST_TEST_REQUIRE(ship.GetTargetHarbor() == otherHarborId);
    // Moving
    BOOST_TEST_REQUIRE(ship.IsMoving());
}

BOOST_AUTO_TEST_SUITE_END()
