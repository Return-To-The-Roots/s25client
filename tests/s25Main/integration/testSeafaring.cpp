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
    const GamePlayer& player = world.getPlayer(curPlayer);
    const BuildingRegister& buildings = player.getBuildingRegister();
    const MapPoint hqPos = player.getHQPos();
    const SeaId seaId(1);
    const HarborId hbId(1);
    const MapPoint hbPos = world.getHarborPoint(hbId);
    BOOST_TEST_REQUIRE(world.calcDistance(hqPos, hbPos) < HQ_RADIUS);

    auto* harbor = dynamic_cast<nobHarborBuilding*>(
      BuildingFactory::createBuilding(world, BuildingType::HarborBuilding, hbPos, curPlayer, Nation::Romans));
    BOOST_TEST_REQUIRE(harbor);
    BOOST_TEST_REQUIRE(world.isHarborAtSea(hbId, seaId));
    BOOST_TEST_REQUIRE(buildings.getHarbors().size() == 1u); //-V807
    BOOST_TEST_REQUIRE(buildings.getHarbors().front() == harbor);
    // A harbor is also a storehouse
    BOOST_TEST_REQUIRE(buildings.getStorehouses().size() == 2u);
    BOOST_TEST_REQUIRE(buildings.getHarbors().back() == harbor);
    BOOST_TEST_REQUIRE(world.getNode(MapPoint(0, 0)).seaId == seaId);
    BOOST_TEST_REQUIRE(world.getSeaId(hbId, Direction::NorthEast) == seaId);

    const std::vector<Direction> road = FindRoadPath(world.getNeighbour(hqPos, Direction::SouthEast),
                                                     world.getNeighbour(hbPos, Direction::SouthEast), world);
    BOOST_TEST_REQUIRE(!road.empty());
}

BOOST_FIXTURE_TEST_CASE(ShipConnections, SmallSeaWorld)
{
    unsigned curPlayer = 0;
    const auto createHarbor = [this, &curPlayer](HarborId hbId) -> nobHarborBuilding& {
        MapPoint hbPos = world.getHarborPoint(hbId);
        auto* harbor = static_cast<nobHarborBuilding*>(
          BuildingFactory::createBuilding(world, BuildingType::HarborBuilding, hbPos, curPlayer, Nation::Romans));
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
    BOOST_TEST(world.isHarborAtSea(HarborId(1), sea1));
    BOOST_TEST(world.isHarborAtSea(HarborId(3), sea1));
    BOOST_TEST(world.isHarborAtSea(HarborId(6), sea1));
    BOOST_TEST(world.isHarborAtSea(HarborId(8), sea1));

    BOOST_TEST(world.isHarborAtSea(HarborId(2), sea2));
    BOOST_TEST(world.isHarborAtSea(HarborId(4), sea2));
    BOOST_TEST(world.isHarborAtSea(HarborId(5), sea2));
    BOOST_TEST(world.isHarborAtSea(HarborId(7), sea2));

    const auto& hb1 = createHarbor(HarborId(1));
    // No other harbor
    BOOST_TEST(hb1.getShipConnections().empty());
    const auto& hb2 = createHarbor(HarborId(2));
    // Different seas
    BOOST_TEST(hb1.getShipConnections().empty());
    BOOST_TEST(hb2.getShipConnections().empty());
    using harbors_t = std::set<const noRoadNode*>;
    // 3 and 1 share sea
    const auto& hb3 = createHarbor(HarborId(3));
    BOOST_TEST(toHarbors(hb1.getShipConnections()) == harbors_t{&hb3});
    BOOST_TEST(hb2.getShipConnections().empty());
    BOOST_TEST(toHarbors(hb3.getShipConnections()) == harbors_t{&hb1});
    // 2,4,5 share sea
    const auto& hb4 = createHarbor(HarborId(4));
    const auto& hb5 = createHarbor(HarborId(5));
    BOOST_TEST(toHarbors(hb1.getShipConnections()) == harbors_t{&hb3});
    BOOST_TEST(toHarbors(hb2.getShipConnections()) == (harbors_t{&hb4, &hb5}));
    BOOST_TEST(toHarbors(hb3.getShipConnections()) == harbors_t{&hb1});
    BOOST_TEST(toHarbors(hb4.getShipConnections()) == (harbors_t{&hb2, &hb5}));
    BOOST_TEST(toHarbors(hb5.getShipConnections()) == (harbors_t{&hb2, &hb4}));
}

BOOST_FIXTURE_TEST_CASE(ShipBuilding, SeaWorldWithGCExecution<>)
{
    addStartResources();
    initGameRNG();

    const GamePlayer& player = world.getPlayer(curPlayer);
    const MapPoint hqPos = player.getHQPos();
    const MapPoint hqFlagPos = world.getNeighbour(hqPos, Direction::SouthEast);
    const HarborId hbId(1);
    const MapPoint hbPos = world.getHarborPoint(hbId);
    const MapPoint shipyardPos(hqPos.x + 3, hqPos.y - 5);

    auto* harbor = dynamic_cast<nobHarborBuilding*>(
      BuildingFactory::createBuilding(world, BuildingType::HarborBuilding, hbPos, curPlayer, Nation::Romans));
    BOOST_TEST_REQUIRE(harbor);
    std::vector<Direction> road = FindRoadPath(hqFlagPos, world.getNeighbour(hbPos, Direction::SouthEast), world);
    BOOST_TEST_REQUIRE(!road.empty());
    this->BuildRoad(hqFlagPos, false, road);
    MapPoint curPt = hqFlagPos;
    for(auto i : road)
    {
        curPt = world.getNeighbour(curPt, i);
        this->SetFlag(curPt);
    }
    BOOST_TEST_REQUIRE(world.GetBQ(shipyardPos, curPlayer) == BuildingQuality::Castle);
    auto* shipYard = dynamic_cast<nobShipYard*>(
      BuildingFactory::createBuilding(world, BuildingType::Shipyard, shipyardPos, curPlayer, Nation::Romans));
    BOOST_TEST_REQUIRE(shipYard);
    road = FindRoadPath(hqFlagPos, world.getNeighbour(shipyardPos, Direction::SouthEast), world);
    BOOST_TEST_REQUIRE(!road.empty());
    this->BuildRoad(hqFlagPos, false, road);
    BOOST_TEST_REQUIRE(shipYard->getMode() == nobShipYard::Mode::Boats); //-V522
    this->SetShipYardMode(shipyardPos, false);
    BOOST_TEST_REQUIRE(shipYard->getMode() == nobShipYard::Mode::Boats);
    this->SetShipYardMode(shipyardPos, true);
    BOOST_TEST_REQUIRE(shipYard->getMode() == nobShipYard::Mode::Ships);
    this->SetShipYardMode(shipyardPos, true);
    BOOST_TEST_REQUIRE(shipYard->getMode() == nobShipYard::Mode::Ships);
    this->SetShipYardMode(shipyardPos, false);
    BOOST_TEST_REQUIRE(shipYard->getMode() == nobShipYard::Mode::Boats);
    this->SetShipYardMode(shipyardPos, true);
    BOOST_TEST_REQUIRE(shipYard->getMode() == nobShipYard::Mode::Ships);

    world.getPostMgr().addPostBox(curPlayer);
    PostBox& postBox = *world.getPostMgr().getPostBox(curPlayer);
    postBox.clear();
    // Ship building takes 10 steps with ~500 GFs each. +600 GF to let wares and people reach the site
    RTTR_EXEC_TILL(5600, postBox.getNumMsgs() > 0);
    // There should be a msg telling the player about the new ship
    BOOST_TEST_REQUIRE(postBox.getNumMsgs() == 1u);
    const auto* msg = dynamic_cast<const ShipPostMsg*>(postBox.getMsg(0));
    BOOST_TEST_REQUIRE(msg);
    BOOST_TEST_REQUIRE(player.getNumShips() == 1u);
    BOOST_TEST_REQUIRE(player.getShips().size() == 1u);
    noShip* ship = player.getShipByID(0);
    BOOST_TEST_REQUIRE(ship);
    BOOST_TEST_REQUIRE(player.getShipID(*ship) == 0u);
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
        GamePlayer& player = world.getPlayer(curPlayer);
        const MapPoint hqPos = player.getHQPos();
        const MapPoint hqFlagPos = world.getNeighbour(hqPos, Direction::SouthEast);
        const MapPoint hbPos = world.getHarborPoint(HarborId(T_hbId));
        world.getPostMgr().addPostBox(curPlayer);
        postBox = world.getPostMgr().getPostBox(curPlayer);

        auto* harbor = dynamic_cast<nobHarborBuilding*>(
          BuildingFactory::createBuilding(world, BuildingType::HarborBuilding, hbPos, curPlayer, Nation::Romans));
        BOOST_TEST_REQUIRE(harbor);
        world.recalcBQAroundPointBig(hbPos);
        std::vector<Direction> road = FindRoadPath(hqFlagPos, world.getNeighbour(hbPos, Direction::SouthEast), world);
        BOOST_TEST_REQUIRE(!road.empty());
        this->BuildRoad(hqFlagPos, false, road);
        MapPoint curPt = hqFlagPos;
        for(auto& i : road)
        {
            curPt = world.getNeighbour(curPt, i);
            this->SetFlag(curPt);
        }

        MapPoint shipPos(hbPos.x, hbPos.y - 3);
        if(!world.isSeaPoint(shipPos))
            shipPos.y += 6;
        BOOST_TEST_REQUIRE(world.isSeaPoint(shipPos));
        auto& ship = world.addFigure(shipPos, std::make_unique<noShip>(shipPos, curPlayer));
        player.registerShip(ship);

        BOOST_TEST_REQUIRE(player.getNumShips() == 1u);
        postBox->clear();
        initGameRNG();
    }

    ShipDirection GetShipDir(const noShip& ship, HarborId targetHbId) const
    {
        for(const auto dir : helpers::enumRange<ShipDirection>())
        {
            if(world.getNextFreeHarborPoint(ship.getPos(), ship.getCurrentHarbor(), dir, ship.getPlayerId())
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
    const GamePlayer& player = world.getPlayer(curPlayer);
    const noShip& ship = ensureNonNull(player.getShipByID(0));
    const nobHarborBuilding& harbor = *player.getBuildingRegister().getHarbors().front();
    const MapPoint hbPos = harbor.getPos();
    const HarborId hbId = world.getHarborPointID(hbPos);
    BOOST_TEST_REQUIRE(ship.isIdling());
    BOOST_TEST_REQUIRE(!harbor.isExplorationExpeditionActive());
    this->StartStopExplorationExpedition(hbPos, true);
    BOOST_TEST_REQUIRE(harbor.isExplorationExpeditionActive());
    // Expedition not ready, ship still idling
    BOOST_TEST_REQUIRE(ship.isIdling());

    // Wait till scouts arrive
    RTTR_EXEC_TILL(380, !ship.isIdling());
    BOOST_TEST_REQUIRE(ship.isGoingToHarbor(harbor));
    BOOST_TEST_REQUIRE(ship.getTargetHarbor() == hbId);
    BOOST_TEST_REQUIRE(player.getShipsToHarbor(harbor) == 1u);

    // No available scouts
    BOOST_TEST_REQUIRE(harbor.getNumRealFigures(Job::Scout) == 0u);
    // Stop it
    this->StartStopExplorationExpedition(hbPos, true);
    BOOST_TEST_REQUIRE(harbor.isExplorationExpeditionActive());
    this->StartStopExplorationExpedition(hbPos, false);
    BOOST_TEST_REQUIRE(!harbor.isExplorationExpeditionActive());
    this->StartStopExplorationExpedition(hbPos, false);
    BOOST_TEST_REQUIRE(!harbor.isExplorationExpeditionActive());
    // Scouts available again
    BOOST_TEST_REQUIRE(harbor.getNumRealFigures(Job::Scout) == 3u);

    // Let ship arrive
    RTTR_EXEC_TILL(180, ship.isIdling());
    BOOST_TEST_REQUIRE(!ship.isGoingToHarbor(harbor));
    BOOST_TEST_REQUIRE(player.getShipsToHarbor(harbor) == 0u);
    BOOST_TEST_REQUIRE(!ship.getTargetHarbor());
    BOOST_TEST_REQUIRE(!ship.getHomeHarbor());

    // We want the ship to only scout unexplored harbors, so set all but one to visible
    world.getNodeWriteable(world.getHarborPoint(HarborId(6))).fow[curPlayer].visibility = Visibility::Visible; //-V807
    // Team visibility, so set one to own team
    world.getPlayer(curPlayer).team = Team::Team1;
    world.getPlayer(1).team = Team::Team1;
    world.getPlayer(curPlayer).makeStartPacts();
    world.getPlayer(1).makeStartPacts();
    world.getNodeWriteable(world.getHarborPoint(HarborId(3))).fow[1].visibility = Visibility::Visible;
    HarborId targetHbId(8);

    // Start again (everything is here)
    this->StartStopExplorationExpedition(hbPos, true);
    // ...so we can start right now
    BOOST_TEST_REQUIRE(ship.isOnExplorationExpedition());
    // Load and start
    RTTR_SKIP_GFS(202);
    BOOST_TEST_REQUIRE(ship.getHomeHarbor() == hbId);
    BOOST_TEST_REQUIRE(ship.getTargetHarbor() == targetHbId);

    unsigned distance = world.calcDistance(hbPos, world.getHarborPoint(targetHbId));
    // Let the ship scout the harbor
    RTTR_EXEC_TILL(distance * 2 * 20, !ship.isMoving());
    BOOST_TEST_REQUIRE(ship.isOnExplorationExpedition());
    BOOST_TEST_REQUIRE(world.calcDistance(world.getHarborPoint(targetHbId), ship.getPos()) <= 2u);
    // Now the ship waits and will select the next harbor. We allow another one:
    HarborId nextHbId(6);
    world.getNodeWriteable(world.getHarborPoint(nextHbId)).fow[curPlayer].visibility = Visibility::FogOfWar;
    RTTR_EXEC_TILL(350, ship.isMoving());
    BOOST_TEST_REQUIRE(ship.getHomeHarbor() == hbId);
    BOOST_TEST_REQUIRE(ship.getTargetHarbor() == nextHbId);
    distance = world.calcDistance(ship.getPos(), world.getHarborPoint(nextHbId));
    // Let the ship scout the harbor
    RTTR_EXEC_TILL(distance * 2 * 20, !ship.isMoving());
    BOOST_TEST_REQUIRE(ship.isOnExplorationExpedition());
    BOOST_TEST_REQUIRE(world.calcDistance(world.getHarborPoint(nextHbId), ship.getPos()) <= 2u);

    // Now disallow the first harbor so ship returns home
    world.getNodeWriteable(world.getHarborPoint(targetHbId)).fow[curPlayer].visibility = Visibility::Visible;

    RTTR_EXEC_TILL(350, ship.isMoving());
    BOOST_TEST_REQUIRE(ship.getHomeHarbor() == hbId);
    BOOST_TEST_REQUIRE(ship.getTargetHarbor() == hbId);

    distance = world.calcDistance(ship.getPos(), world.getHarborPoint(hbId));
    // And at some time it should return home
    RTTR_EXEC_TILL(distance * 2 * 20 + 200, ship.isIdling());
    const SeaId seaId(1);
    BOOST_TEST_REQUIRE(ship.getSeaID() == seaId);
    BOOST_TEST_REQUIRE(ship.getPos() == world.getCoastalPoint(hbId, seaId));

    // Now try to start an expedition but all harbors are explored -> Load, Unload, Idle
    world.getNodeWriteable(world.getHarborPoint(nextHbId)).fow[curPlayer].visibility = Visibility::Visible;
    this->StartStopExplorationExpedition(hbPos, true);
    BOOST_TEST_REQUIRE(ship.isOnExplorationExpedition());
    RTTR_EXEC_TILL(2 * 200 + 5, ship.isIdling());
}

BOOST_FIXTURE_TEST_CASE(DestroyHomeOnExplExp, ShipReadyFixture<2>)
{
    addStartResources();
    curPlayer = 0;
    const GamePlayer& player = world.getPlayer(curPlayer);
    const noShip& ship = ensureNonNull(player.getShipByID(0));
    const nobHarborBuilding& harbor = *player.getBuildingRegister().getHarbors().front();
    const MapPoint hbPos = harbor.getPos();
    const HarborId hbId = world.getHarborPointID(hbPos);
    unsigned numScouts = player.getInventory()[Job::Scout]; //-V807
    BOOST_TEST_REQUIRE(ship.isIdling());

    // We want the ship to only scout unexplored harbors, so set all but one to visible
    world.getPlayer(curPlayer).team = Team::Team1;
    world.getPlayer(1).team = Team::Team1;
    world.getPlayer(curPlayer).makeStartPacts();
    world.getPlayer(1).makeStartPacts();

    world.getNodeWriteable(world.getHarborPoint(HarborId(6))).fow[1].visibility = Visibility::Visible;
    world.getNodeWriteable(world.getHarborPoint(HarborId(3))).fow[1].visibility = Visibility::Visible;
    HarborId targetHbId(8);
    this->StartStopExplorationExpedition(hbPos, true);

    // Start it
    RTTR_EXEC_TILL(600, ship.isOnExplorationExpedition() && ship.isMoving());
    // Incorporate recruitment
    BOOST_TEST_REQUIRE(player.getInventory()[Job::Scout] >= numScouts);
    numScouts = player.getInventory()[Job::Scout];

    BOOST_TEST_REQUIRE(ship.getHomeHarbor() == hbId);
    BOOST_TEST_REQUIRE(ship.getTargetHarbor() == targetHbId);

    // Run till ship is coming back
    RTTR_EXEC_TILL(1000, ship.getTargetHarbor() == hbId);
    // Avoid that it goes back to that point
    world.getNodeWriteable(world.getHarborPoint(targetHbId)).fow[1].visibility = Visibility::Visible;

    // Destroy home harbor
    world.destroyNO(hbPos);
    RTTR_EXEC_TILL(2000, ship.isLost());
    BOOST_TEST_REQUIRE(!ship.isMoving());

    MapPoint newHbPos = world.getHarborPoint(HarborId(6));
    auto* newHarbor = dynamic_cast<nobHarborBuilding*>(
      BuildingFactory::createBuilding(world, BuildingType::HarborBuilding, newHbPos, curPlayer, Nation::Romans));

    BOOST_TEST_REQUIRE(!ship.isLost());
    BOOST_TEST_REQUIRE(ship.isMoving());
    RTTR_EXEC_TILL(1200, ship.isIdling());
    BOOST_TEST(player.getInventory()[Job::Scout] == numScouts);
    BOOST_TEST(newHarbor->getNumRealFigures(Job::Scout) == newHarbor->getNumVisualFigures(Job::Scout)); //-V522
    BOOST_TEST(newHarbor->getNumRealFigures(Job::Scout)
                 + world.getSpecObj<nobBaseWarehouse>(player.getHQPos())->getNumRealFigures(Job::Scout)
               == numScouts);
}

BOOST_FIXTURE_TEST_CASE(Expedition, ShipReadyFixture<>)
{
    addStartResources();
    const GamePlayer& player = world.getPlayer(curPlayer);
    const noShip& ship = ensureNonNull(player.getShipByID(0));
    const nobHarborBuilding& harbor = *player.getBuildingRegister().getHarbors().front();
    const MapPoint hbPos = harbor.getPos();
    BOOST_TEST_REQUIRE(ship.isIdling());
    BOOST_TEST_REQUIRE(!harbor.isExpeditionActive());
    this->StartStopExpedition(hbPos, true);
    BOOST_TEST_REQUIRE(harbor.isExpeditionActive());
    // Expedition not ready, ship still idling
    BOOST_TEST_REQUIRE(ship.isIdling());

    // Wait till wares arrive
    RTTR_EXEC_TILL(3400, !ship.isIdling());
    // Expedition ready, ship ordered
    BOOST_TEST_REQUIRE(ship.isGoingToHarbor(harbor));
    BOOST_TEST_REQUIRE(player.getShipsToHarbor(harbor) == 1u);

    // No available boards
    BOOST_TEST_REQUIRE(harbor.getNumRealWares(GoodType::Boards) == 0u);
    // Stop it
    this->StartStopExpedition(hbPos, true);
    BOOST_TEST_REQUIRE(harbor.isExpeditionActive());
    this->StartStopExpedition(hbPos, false);
    BOOST_TEST_REQUIRE(!harbor.isExpeditionActive());
    this->StartStopExpedition(hbPos, false);
    BOOST_TEST_REQUIRE(!harbor.isExpeditionActive());
    // Boards available again
    BOOST_TEST_REQUIRE(harbor.getNumRealWares(GoodType::Boards) > 0u);

    // Let ship arrive
    RTTR_EXEC_TILL(180, ship.isIdling());
    BOOST_TEST_REQUIRE(!ship.isGoingToHarbor(harbor));
    BOOST_TEST_REQUIRE(player.getShipsToHarbor(harbor) == 0u);

    // Start again (everything is here)
    this->StartStopExpedition(hbPos, true);
    // ...so we can start right now
    BOOST_TEST_REQUIRE(ship.isOnExpedition());

    // Wait for ship to be "loaded"
    RTTR_EXEC_TILL(200, ship.isWaitingForExpeditionInstructions());
    // Ship should be waiting for expedition instructions (where to go) and player should have received a message
    BOOST_TEST_REQUIRE(ship.getCurrentHarbor() == harbor.getHarborPosID());
    BOOST_TEST_REQUIRE(postBox->getNumMsgs() == 1u);
    const auto* msg = dynamic_cast<const ShipPostMsg*>(postBox->getMsg(0));
    BOOST_TEST_REQUIRE(msg);
    BOOST_TEST_REQUIRE(msg->getPos() == ship.getPos()); //-V522

    // Harbor pos taken by other player
    this->travelToNextSpot(ShipDirection::SouthEast, player.getShipID(ship));
    BOOST_TEST_REQUIRE(ship.isWaitingForExpeditionInstructions());

    // Last free one (far south -> North is closer)
    this->travelToNextSpot(ShipDirection::North, player.getShipID(ship));
    BOOST_TEST_REQUIRE(!ship.isWaitingForExpeditionInstructions());
    BOOST_TEST_REQUIRE(ship.isMoving());

    unsigned gfsToDest;
    RTTR_EXEC_TILL_CT_GF(1000, ship.isWaitingForExpeditionInstructions(), gfsToDest);
    HarborId hbId(8);
    BOOST_TEST_REQUIRE(ship.getCurrentHarbor() == hbId);
    BOOST_TEST_REQUIRE(world.calcDistance(ship.getPos(), world.getHarborPoint(hbId)) == 1u);

    // Cancel expedition -> Ship is going back to harbor
    this->CancelExpedition(player.getShipID(ship));
    BOOST_TEST_REQUIRE(ship.isMoving());
    // Let ship arrive and unload
    RTTR_EXEC_TILL(gfsToDest + 300, ship.isIdling());

    // Start again (everything is here)
    this->StartStopExpedition(hbPos, true);
    BOOST_TEST_REQUIRE(ship.isOnExpedition());

    // Wait for ship to be "loaded"
    RTTR_EXEC_TILL(200, ship.isWaitingForExpeditionInstructions());
    BOOST_TEST_REQUIRE(ship.getCurrentHarbor() == harbor.getHarborPosID());

    // Try to found colony -> Fail
    this->foundColony(player.getShipID(ship));
    BOOST_TEST_REQUIRE(ship.isWaitingForExpeditionInstructions());
    // Go back to free spot
    this->travelToNextSpot(ShipDirection::North, player.getShipID(ship));
    BOOST_TEST_REQUIRE(!ship.isWaitingForExpeditionInstructions());
    for(unsigned gf = 0; gf < 2; gf++)
    {
        // Send commands that should be ignored as ship is not expecting them
        this->foundColony(player.getShipID(ship));
        BOOST_TEST_REQUIRE(ship.isMoving());
        this->travelToNextSpot(ShipDirection::South, player.getShipID(ship));
        this->CancelExpedition(player.getShipID(ship));
        BOOST_TEST_REQUIRE(ship.isMoving());
        this->em.executeNextGF();
    }
    // Go to destination
    postBox->clear();
    RTTR_SKIP_GFS(gfsToDest);
    BOOST_TEST_REQUIRE(ship.isWaitingForExpeditionInstructions());
    // This should again trigger a message
    msg = dynamic_cast<const ShipPostMsg*>(postBox->getMsg(0));
    BOOST_TEST_REQUIRE(msg);
    BOOST_TEST_REQUIRE(msg->getPos() == ship.getPos());
    // And now we can found a new colony
    this->foundColony(player.getShipID(ship));
    // Ship is free again
    BOOST_TEST_REQUIRE(ship.isIdling());
    const noBuildingSite* newHarbor = world.getSpecObj<noBuildingSite>(world.getHarborPoint(hbId));
    BOOST_TEST_REQUIRE(newHarbor);
    BOOST_TEST_REQUIRE(world.isHarborBuildingSiteFromSea(newHarbor));
    // And it should be completed after some time
    RTTR_EXEC_TILL(5000, player.getBuildingRegister().getHarbors().size() > 1); //-V807
    BOOST_TEST_REQUIRE(player.getBuildingRegister().getHarbors().size() == 2u);
}

using ShipReadyFixtureBig = ShipReadyFixture<1, 2, 64, 800>;

BOOST_FIXTURE_TEST_CASE(LongDistanceTravel, ShipReadyFixtureBig)
{
    const GamePlayer& player = world.getPlayer(curPlayer);
    const noShip& ship = ensureNonNull(player.getShipByID(0));
    nobHarborBuilding& harbor = *player.getBuildingRegister().getHarbors().front();
    const MapPoint hbPos = harbor.getPos();
    // Go to opposite one
    const HarborId targetHbId(7);
    // Make sure that the other harbor is far away
    BOOST_TEST_REQUIRE(world.getMinHarborDistance(harbor.getHarborPosID(), targetHbId) > 600u);
    // Add some scouts
    harbor.addToInventory(PeopleCounts::make(Job::Scout, 20), true);
    // We want the ship to only scout unexplored harbors, so set all but one to visible
    for(const auto i : helpers::idRange<HarborId>(world.getNumHarborPoints()))
        world.getNodeWriteable(world.getHarborPoint(i)).fow[curPlayer].visibility = Visibility::Visible;
    world.getNodeWriteable(world.getHarborPoint(targetHbId)).fow[curPlayer].visibility = Visibility::Invisible;
    // Start an exploration expedition
    this->StartStopExplorationExpedition(hbPos, true);
    BOOST_TEST_REQUIRE(harbor.isExplorationExpeditionActive());
    // Wait till ship has arrived and starts loading
    RTTR_EXEC_TILL(100, ship.isOnExplorationExpedition());
    // Wait till ship has loaded scouts
    RTTR_EXEC_TILL(200, ship.isMoving());
    BOOST_TEST_REQUIRE(ship.getTargetHarbor() == targetHbId);
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

    GameWorld& getWorld() override { return world; }

    nobHarborBuilding& createHarbor(HarborId hbPosId)
    {
        MapPoint hbPos = world.getHarborPoint(hbPosId);
        auto* harbor = static_cast<nobHarborBuilding*>(
          BuildingFactory::createBuilding(world, BuildingType::HarborBuilding, hbPos, curPlayer, Nation::Romans));
        BOOST_TEST_REQUIRE(harbor);
        GoodsAndPeopleCounts goods;
        goods[GoodType::Wood] = 10;
        goods[Job::Woodcutter] = 10;
        harbor->addToInventory(goods, true);
        return *harbor;
    }

    ShipAndHarborsReadyFixture()
    {
        GamePlayer& player = world.getPlayer(curPlayer);
        createHarbor(HarborId(1));
        createHarbor(HarborId(2));

        MapPoint hbPos = world.getHarborPoint(HarborId(1));
        MapPoint shipPos = world.makeMapPoint(hbPos - Position(2, 0));
        BOOST_TEST_REQUIRE(world.isSeaPoint(shipPos));
        auto& ship = world.addFigure(shipPos, std::make_unique<noShip>(shipPos, curPlayer));
        player.registerShip(ship);

        BOOST_TEST_REQUIRE(player.getNumShips() == 1u);
    }
};

void destroyBldAndFire(GameWorldBase& world, const MapPoint& pos)
{
    world.destroyNO(pos);
    // Remove fire
    world.destroyNO(pos);
    // Remove burned wh if existing
    for(noBase& fig : world.getFigures(pos))
    {
        if(fig.getGOT() == GO_Type::Burnedwarehouse)
        {
            // Remove go-out event (not automatically done as the burned wh is never removed)
            const GameEvent* ev = static_cast<TestEventManager&>(world.getEvMgr()).getObjEvents(fig).front();
            world.getEvMgr().removeEvent(ev);
            world.RemoveFigure(pos, fig)->destroy();
            break;
        }
    }
}
} // namespace

BOOST_FIXTURE_TEST_CASE(harborDestroyed, ShipAndHarborsReadyFixture<1>)
{
    const GamePlayer& player = world.getPlayer(curPlayer);
    const noShip& ship = *player.getShipByID(0);
    constexpr HarborId hb1Id(1);
    constexpr HarborId hb2Id(2);
    constexpr HarborId hb3Id(3);
    const MapPoint hb1Pos = world.getHarborPoint(hb1Id);
    const MapPoint hb2Pos = world.getHarborPoint(hb2Id);
    const MapPoint hb3Pos = world.getHarborPoint(hb3Id);

    // Order goods
    SetInventorySetting(hb2Pos, GoodType::Wood, EInventorySetting::Collect);
    SetInventorySetting(hb2Pos, Job::Woodcutter, EInventorySetting::Collect);

    // Destroy home before load -> Abort after ship reaches harbor
    RTTR_EXEC_TILL(90, ship.isMoving());
    destroyBldAndFire(world, hb1Pos);
    RTTR_EXEC_TILL(10, !ship.isMoving());
    BOOST_TEST_REQUIRE(ship.isIdling());

    // Destroy home during load -> Continue
    createHarbor(hb1Id);
    // Just wait for re-order event and instantly go to loading as ship does not need to move
    RTTR_EXEC_TILL(90, ship.isLoading());
    destroyBldAndFire(world, hb1Pos);
    BOOST_TEST_REQUIRE(ship.isLoading());
    BOOST_TEST_REQUIRE(!ship.getHomeHarbor());
    RTTR_EXEC_TILL(300, ship.isUnloading());
    constexpr SeaId seaId(1);
    BOOST_TEST_REQUIRE(ship.getPos() == world.getCoastalPoint(hb2Id, seaId));
    RTTR_EXEC_TILL(200, ship.isIdling());

    // Destroy destination before load -> Abort after ship reaches harbor
    createHarbor(hb1Id);
    RTTR_EXEC_TILL(90, ship.isMoving());
    destroyBldAndFire(world, hb2Pos);
    RTTR_EXEC_TILL(200, !ship.isMoving());
    BOOST_TEST_REQUIRE(ship.isIdling());

    // Destroy destination during load -> Unload again
    createHarbor(hb2Id);
    // Order goods
    SetInventorySetting(hb2Pos, GoodType::Wood, EInventorySetting::Collect);
    SetInventorySetting(hb2Pos, Job::Woodcutter, EInventorySetting::Collect);
    RTTR_EXEC_TILL(300, ship.isLoading());
    destroyBldAndFire(world, hb2Pos);
    BOOST_TEST_REQUIRE(ship.isUnloading());
    RTTR_EXEC_TILL(200, ship.isIdling());

    // Destroy destination during driving -> Go back and unload
    createHarbor(hb2Id);
    // Order goods
    SetInventorySetting(hb2Pos, GoodType::Wood, EInventorySetting::Collect);
    SetInventorySetting(hb2Pos, Job::Woodcutter, EInventorySetting::Collect);
    RTTR_EXEC_TILL(300, ship.isLoading());
    RTTR_EXEC_TILL(200, ship.isMoving());
    BOOST_TEST_REQUIRE(ship.getHomeHarbor() == hb1Id);
    BOOST_TEST_REQUIRE(ship.getTargetHarbor() == hb2Id);
    destroyBldAndFire(world, hb2Pos);
    RTTR_EXEC_TILL(10, ship.getTargetHarbor() == hb1Id);
    RTTR_EXEC_TILL(20, ship.isUnloading());
    BOOST_TEST_REQUIRE(ship.getPos() == world.getCoastalPoint(hb1Id, seaId));
    RTTR_EXEC_TILL(200, ship.isIdling());

    // Destroy destination during unloading -> Go back and unload
    createHarbor(hb2Id);
    // Order goods
    SetInventorySetting(hb2Pos, GoodType::Wood, EInventorySetting::Collect);
    SetInventorySetting(hb2Pos, Job::Woodcutter, EInventorySetting::Collect);
    RTTR_EXEC_TILL(700, ship.isUnloading());
    BOOST_TEST_REQUIRE(ship.getHomeHarbor() == hb1Id);
    BOOST_TEST_REQUIRE(ship.getTargetHarbor() == hb2Id);
    destroyBldAndFire(world, hb2Pos);
    RTTR_EXEC_TILL(10, ship.getTargetHarbor() == hb1Id);
    RTTR_EXEC_TILL(200, ship.isUnloading());
    BOOST_TEST_REQUIRE(ship.getPos() == world.getCoastalPoint(hb1Id, seaId));
    RTTR_EXEC_TILL(200, ship.isIdling());

    // Destroy both during unloading -> Go to 3rd and unload
    createHarbor(hb2Id);
    createHarbor(hb3Id);
    // Order goods
    SetInventorySetting(hb2Pos, GoodType::Wood, EInventorySetting::Collect);
    SetInventorySetting(hb2Pos, Job::Woodcutter, EInventorySetting::Collect);
    RTTR_EXEC_TILL(700, ship.isUnloading());
    BOOST_TEST_REQUIRE(ship.getHomeHarbor() == hb1Id);
    BOOST_TEST_REQUIRE(ship.getTargetHarbor() == hb2Id);
    destroyBldAndFire(world, hb1Pos);
    destroyBldAndFire(world, hb2Pos);
    RTTR_EXEC_TILL(10, ship.getTargetHarbor() == HarborId(3));
    // Destroy last ->
    destroyBldAndFire(world, hb3Pos);
    RTTR_EXEC_TILL(10, ship.isLost());
    BOOST_TEST_REQUIRE(!ship.getHomeHarbor());
    BOOST_TEST_REQUIRE(!ship.getTargetHarbor());
    createHarbor(hb1Id);
    BOOST_TEST_REQUIRE(ship.isMoving());
    BOOST_TEST_REQUIRE(ship.getHomeHarbor() == hb1Id);
    BOOST_TEST_REQUIRE(ship.getTargetHarbor() == hb1Id);
}

// Dimensions must be large enough so path tolerances get too small
using ShipReadyFixtureLarger = ShipReadyFixture<1, 2, 90, 260>;
BOOST_FIXTURE_TEST_CASE(IncreasedPathLengthWhenHarborDestroyed, ShipReadyFixtureLarger)
{
    // Edge case: Path to harbor longer than any direct connection
    /*
     *     2
     *     W
     *    WLW
     *    WLW
     *   WWLWW
     * 4WLLLLLW5
     *   WWLWW
     *   WWLWW
     *     7
     */
    // Shortest path 4->5 is over south, ship might be between 2 & 4 => still finds path to 5
    constexpr HarborId hbIdTop(2);
    constexpr HarborId hbIdLeft(4);
    constexpr HarborId hbIdRight(5);
    constexpr HarborId hbIdBottom(7);
    const MapPoint hbTopPos = world.getHarborPoint(hbIdTop);
    const MapPoint hbLeftPos = world.getHarborPoint(hbIdLeft);
    const MapPoint hbRightPos = world.getHarborPoint(hbIdRight);
    const MapPoint hbBottomPos = world.getHarborPoint(hbIdBottom);
    // Sanity check expected geometry, ensured by fixture
    BOOST_TEST_REQUIRE(hbLeftPos.x < hbRightPos.x);
    BOOST_TEST_REQUIRE(hbLeftPos.y == hbRightPos.y);
    BOOST_TEST_REQUIRE(hbTopPos.y < hbLeftPos.y);
    BOOST_TEST_REQUIRE(hbLeftPos.y < hbBottomPos.y);
    const unsigned yDiff = hbLeftPos.y - hbTopPos.y;
    BOOST_TEST_REQUIRE(yDiff >= 4u * 3u); // So divisions below yield noticeable difference
    const auto t = GetLandTerrain(world.getDescription(), ETerrain::Buildable);
    for(const auto y : helpers::range(hbLeftPos.y - yDiff * 3u / 4u, hbLeftPos.y + yDiff / 2u))
    {
        auto& node = world.getNodeWriteable(MapPoint(hbTopPos.x, y));
        node.t1 = node.t2 = t;
    }
    constexpr auto horStripHeight = 2u;
    // Horizontal land strip to avoid short cuts
    for(const auto y : helpers::range(hbLeftPos.y - horStripHeight, hbLeftPos.y + horStripHeight))
    {
        for(const auto x : helpers::range(hbLeftPos.x + 4, hbRightPos.x - 4))
        {
            auto& node = world.getNodeWriteable(MapPoint(x, y));
            node.t1 = node.t2 = t;
        }
    }
    MapLoader::initSeasAndHarbors(world);
    // Harbors unchanged
    BOOST_TEST_REQUIRE(world.getHarborPointID(hbTopPos) == hbIdTop);
    BOOST_TEST_REQUIRE(world.getHarborPointID(hbLeftPos) == hbIdLeft);
    BOOST_TEST_REQUIRE(world.getHarborPointID(hbRightPos) == hbIdRight);
    BOOST_TEST_REQUIRE(world.getHarborPointID(hbBottomPos) == hbIdBottom);

    const GamePlayer& player = world.getPlayer(curPlayer);
    const noShip& ship = ensureNonNull(player.getShipByID(0));
    const SeaId seaId = ship.getSeaID();

    // Start shipment to left harbor
    nobHarborBuilding& topHarbor = *player.getBuildingRegister().getHarbors().front();
    BuildingFactory::createBuilding(world, BuildingType::HarborBuilding, hbLeftPos, curPlayer, Nation::Romans);

    topHarbor.addToInventory(GoodCounts::make(GoodType::Wood, 10), true);
    SetInventorySetting(hbLeftPos, GoodType::Wood, EInventorySetting::Collect);

    RTTR_EXEC_TILL(90, ship.isLoading());
    BOOST_TEST_REQUIRE(ship.getPos() == world.getCoastalPoint(hbIdTop, seaId));
    RTTR_EXEC_TILL(400, ship.isMoving());
    BOOST_TEST_REQUIRE(ship.getTargetHarbor() == hbIdLeft);
    const auto leftCoastPt = world.getCoastalPoint(hbIdLeft, seaId);

    // Wait till ship is close to left harbor
    constexpr auto extraDist = 16u; // Choosen to be larger than a magic number used as tolerance
    RTTR_EXEC_TILL(2000, world.calcDistance(ship.getPos(), leftCoastPt) < extraDist * 2);

    // Destroy home so it cannot go back, then target harbor
    destroyBldAndFire(world, hbTopPos);
    destroyBldAndFire(world, hbLeftPos);
    RTTR_EXEC_TILL(10, ship.isLost());
    BOOST_TEST_REQUIRE(!ship.getHomeHarbor());
    BOOST_TEST_REQUIRE(!ship.getTargetHarbor());

    // Building harbor at right makes ship go there even though distance is (too) long
    unsigned len;
    BOOST_TEST_REQUIRE(
      world.findShipPath(ship.getPos(), world.getCoastalPoint(hbIdRight, seaId), unsigned(-1), nullptr, &len));
    const auto hbDistance = world.getMinHarborDistance(hbIdLeft, hbIdRight);
    // Reproducer requires the path to be longer than the direct distance and other connections
    BOOST_TEST_REQUIRE(len > hbDistance + extraDist);
    BOOST_TEST_REQUIRE(hbDistance > world.getMinHarborDistance(hbIdRight, hbIdTop));
    BOOST_TEST_REQUIRE(hbDistance > world.getMinHarborDistance(hbIdRight, hbIdBottom));
    BuildingFactory::createBuilding(world, BuildingType::HarborBuilding, hbRightPos, curPlayer, Nation::Romans);
    BOOST_TEST_REQUIRE(ship.isMoving());
    BOOST_TEST_REQUIRE(ship.getHomeHarbor() == hbIdRight);
    BOOST_TEST_REQUIRE(ship.getTargetHarbor() == hbIdRight);
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
    GamePlayer& player = world.getPlayer(curPlayer);
    const auto& harbors = player.getBuildingRegister().getHarbors();
    BOOST_TEST_REQUIRE(harbors.size() >= 2u);
    nobHarborBuilding& harbor1 = *harbors.front();
    nobHarborBuilding& harbor2 = **(++harbors.begin());
    auto* wh = static_cast<nobBaseWarehouse*>(BuildingFactory::createBuilding(
      world, BuildingType::Storehouse, harbor2.getPos() + MapPoint(2, 0), curPlayer, Nation::Romans));
    BOOST_TEST_REQUIRE(wh);
    buildRoadForBlds(harbor2.getPos(), wh->getPos());
    constexpr auto goodType = GoodType::Wood;

    // Sanity check: Move from harbor1 to wh (over harbor2)
    auto ware = std::make_unique<MockWare>(goodType, wh, &harbor1);
    auto* warePtr = ware.get();
    ware->carry(harbor1);
    auto numVisWaresBefore = harbor1.getNumVisualWares(goodType);
    auto numRealWaresBefore = harbor1.getNumRealWares(goodType);
    harbor1.addWare(std::move(ware));
    BOOST_TEST(harbor1.getNumVisualWares(goodType) == numVisWaresBefore + 1u);
    // Wares that will be moved out are only in visual count
    BOOST_TEST(harbor1.getNumRealWares(goodType) == numRealWaresBefore);
    BOOST_TEST_REQUIRE(!MockWare::destroyed);
    BOOST_TEST(warePtr->getGoal() == wh);
    BOOST_TEST(warePtr->getNextDir() == RoadPathDirection::Ship);

    // Add ware with unreachable goal -> Added to harbor
    // First fully disconnect 2nd harbor -> Only reachable via ship
    for(const auto d : helpers::enumRange<Direction>())
    {
        if(d != Direction::NorthWest)
            DestroyRoad(harbor2.getFlagPos(), d);
    }
    ware = std::make_unique<MockWare>(goodType, wh, &harbor1);
    ware->carry(harbor1);
    numVisWaresBefore = harbor1.getNumVisualWares(goodType);
    numRealWaresBefore = harbor1.getNumRealWares(goodType);
    harbor1.addWare(std::move(ware));
    BOOST_TEST(harbor1.getNumVisualWares(goodType) == numVisWaresBefore + 1u);
    BOOST_TEST(harbor1.getNumRealWares(goodType) == numRealWaresBefore + 1u);
    BOOST_TEST(MockWare::destroyed);

    // Same but disallow harbor as goal -> Find another storehouse
    SetInventorySetting(harbor1.getPos(), goodType, EInventorySetting::Stop);
    ware = std::make_unique<MockWare>(goodType, wh, &harbor1);
    ware->carry(harbor1);
    warePtr = ware.get();
    numVisWaresBefore = harbor1.getNumVisualWares(goodType);
    numRealWaresBefore = harbor1.getNumRealWares(goodType);
    harbor1.addWare(std::move(ware));
    BOOST_TEST(harbor1.getNumVisualWares(goodType) == numVisWaresBefore + 1u);
    BOOST_TEST(harbor1.getNumRealWares(goodType) == numRealWaresBefore);
    BOOST_TEST_REQUIRE(!MockWare::destroyed);
    BOOST_TEST(warePtr->getGoal() == &harbor2);

    // If no other storehouse is available take it anyway
    SetInventorySetting(harbor2.getPos(), goodType, EInventorySetting::Stop);
    ware = std::make_unique<MockWare>(goodType, wh, &harbor1);
    ware->carry(harbor1);
    numVisWaresBefore = harbor1.getNumVisualWares(goodType);
    numRealWaresBefore = harbor1.getNumRealWares(goodType);
    harbor1.addWare(std::move(ware));
    BOOST_TEST(harbor1.getNumVisualWares(goodType) == numVisWaresBefore + 1u);
    BOOST_TEST(harbor1.getNumRealWares(goodType) == numRealWaresBefore + 1u);
    BOOST_TEST(MockWare::destroyed);
}

BOOST_FIXTURE_TEST_CASE(GoToNeighborHarbor, ShipReadyFixture<1>)
{
    const GamePlayer& player = world.getPlayer(curPlayer);
    const noShip& ship = ensureNonNull(player.getShipByID(0));
    nobHarborBuilding& homeHarbor = *player.getBuildingRegister().getHarbors().front();
    const MapPoint homeHbPos = homeHarbor.getPos();
    GoodsAndPeopleCounts expWares;
    expWares[Job::Builder] = 1;
    expWares[GoodType::Boards] = 20;
    expWares[GoodType::Stones] = 20;
    homeHarbor.addToInventory(expWares, true);

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
    const MapPoint nbHbPos = world.getNeighbour(harborPos, Direction::SouthEast); // (2)
    const MapPoint otherHbPos = world.makeMapPoint(homeHbPos - Point(3, 0));      // Unrelated harbor
    const std::vector<MapPoint> harborPoints = {harborPos, nbHbPos, otherHbPos};
    for(const MapPoint p : harborPoints)
        world.getNodeWriteable(p).bq = BuildingQuality::Harbor;
    // These nodes and 3 left of it get land terrain (below)
    const std::array nodes{
      world.getNeighbour(world.getNeighbour(harborPos, Direction::NorthEast), Direction::NorthEast),
      world.getNeighbour(harborPos, Direction::NorthWest), harborPos, nbHbPos};
    for(auto p : nodes)
    {
        for([[maybe_unused]] auto i : helpers::range(2))
        {
            auto& node = world.getNodeWriteable(p);
            node.t1 = node.t2 = GetLandTerrain(world.getDescription());
            p = world.getNeighbour(p, Direction::West);
        }
    }
    const auto coastPoint = world.getNeighbour(harborPos, Direction::East);
    // And the coastal point
    {
        auto& node = world.getNodeWriteable(world.getNeighbour(coastPoint, Direction::NorthWest));
        node.t1 = GetLandTerrain(world.getDescription());
    }
    MapLoader::initSeasAndHarbors(world, harborPoints);

    const auto harborId = world.getHarborPointID(harborPos);
    const auto nbHbId = world.getHarborPointID(nbHbPos);
    BOOST_TEST_REQUIRE(harborId.isValid());
    BOOST_TEST_REQUIRE(nbHbId.isValid());
    const auto seaId = world.getSeaId(harborId, Direction::East);
    BOOST_TEST_REQUIRE(seaId.isValid());
    BOOST_TEST_REQUIRE(world.getCoastalPoint(harborId, seaId) == coastPoint);
    const auto nbSeaId = world.getSeaId(nbHbId, Direction::NorthEast);
    BOOST_TEST_REQUIRE(nbSeaId == seaId);
    BOOST_TEST_REQUIRE(world.getCoastalPoint(nbHbId, seaId) == coastPoint);

    this->StartStopExpedition(homeHbPos, true);
    BOOST_TEST_REQUIRE(homeHarbor.isExpeditionActive());
    // Wait till ship has loaded wares and is ready
    RTTR_EXEC_TILL(500, ship.isWaitingForExpeditionInstructions());

    this->travelToNextSpot(this->GetShipDir(ship, harborId), player.getShipID(ship));
    BOOST_TEST_REQUIRE(!ship.isWaitingForExpeditionInstructions());
    BOOST_TEST_REQUIRE(ship.isMoving());
    BOOST_TEST_REQUIRE(ship.getTargetHarbor() == harborId);

    RTTR_EXEC_TILL(500, ship.isWaitingForExpeditionInstructions());
    BOOST_TEST_REQUIRE(ship.getPos() == coastPoint);

    this->travelToNextSpot(this->GetShipDir(ship, nbHbId), player.getShipID(ship));
    BOOST_TEST_REQUIRE(ship.getTargetHarbor() == nbHbId);
    // Already there
    BOOST_TEST(ship.isWaitingForExpeditionInstructions());
    BOOST_TEST(ship.getPos() == coastPoint);

    // Can move
    const auto otherHarborId = world.getHarborPointID(otherHbPos);
    BOOST_TEST_REQUIRE(otherHarborId.isValid());
    // Precondition that any ship dir will be found
    BOOST_TEST_REQUIRE(world.isHarborAtSea(otherHarborId, seaId));
    this->travelToNextSpot(this->GetShipDir(ship, otherHarborId), player.getShipID(ship));
    BOOST_TEST_REQUIRE(ship.getTargetHarbor() == otherHarborId);
    // Moving
    BOOST_TEST_REQUIRE(ship.isMoving());
}

BOOST_AUTO_TEST_SUITE_END()
