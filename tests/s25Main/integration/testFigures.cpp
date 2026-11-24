// Copyright (C) 2005 - 2026 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "GamePlayer.h"
#include "GlobalGameSettings.h"
#include "PointOutput.h"
#include "RttrForeachPt.h"
#include "addons/const_addons.h"
#include "buildings/nobBaseWarehouse.h"
#include "factories/BuildingFactory.h"
#include "figures/noFigure.h"
#include "figures/nofCarrier.h"
#include "figures/nofGeologist.h"
#include "figures/nofPassiveWorker.h"
#include "figures/nofScout_Free.h"

#include "helpers/Range.h"
#include "notifications/ResourceNote.h"
#include "worldFixtures/CreateSeaWorld.h"
#include "worldFixtures/WorldWithGCExecution.h"
#include "nodeObjs/noFlag.h"
#include "nodeObjs/noSign.h"
#include "gameTypes/GameTypesOutput.h"
#include <rttr/test/random.hpp>
#include <rttr/test/testHelpers.hpp>
#include <boost/test/unit_test.hpp>
#include <vector>

// LCOV_EXCL_START
namespace boost::test_tools::tt_detail {
template<>
struct print_log_value<CarrierState>
{
    void operator()(std::ostream& os, const CarrierState type) { os << static_cast<unsigned>(rttr::enumCast(type)); }
};
} // namespace boost::test_tools::tt_detail
// LCOV_EXCL_STOP

BOOST_AUTO_TEST_SUITE(FigureTests)

BOOST_AUTO_TEST_CASE(StrandedSoldierReturnSearchDefaultKeepsNormalTryingsAndRadius)
{
    GlobalGameSettings ggs;
    ggs.setSelection(AddonId::STRANDED_SOLDIER_RETURN_SEARCH, 0);

    BOOST_TEST(getStrandedSoldierReturnSearchTryings(ggs) == 6u);
    BOOST_TEST(getStrandedSoldierReturnSearchRadius(ggs, 6) == 15u);
    BOOST_TEST(getStrandedSoldierReturnSearchRadius(ggs, 1) == 15u);
}

BOOST_AUTO_TEST_CASE(StrandedSoldierReturnSearchReducedKeepsNormalTryingsAndFixedHalfRadius)
{
    GlobalGameSettings ggs;
    ggs.setSelection(AddonId::STRANDED_SOLDIER_RETURN_SEARCH, 1);

    BOOST_TEST(getStrandedSoldierReturnSearchTryings(ggs) == 6u);
    BOOST_TEST(getStrandedSoldierReturnSearchRadius(ggs, 12) == 7u);
    BOOST_TEST(getStrandedSoldierReturnSearchRadius(ggs, 6) == 7u);
    BOOST_TEST(getStrandedSoldierReturnSearchRadius(ggs, 1) == 7u);
}

BOOST_AUTO_TEST_CASE(StrandedSoldierReturnSearchExtendedEscalatesAfterNormalTryings)
{
    GlobalGameSettings ggs;
    ggs.setSelection(AddonId::STRANDED_SOLDIER_RETURN_SEARCH, 2);

    BOOST_TEST(getStrandedSoldierReturnSearchTryings(ggs) == 12u);
    BOOST_TEST(getStrandedSoldierReturnSearchRadius(ggs, 12) == 15u);
    BOOST_TEST(getStrandedSoldierReturnSearchRadius(ggs, 7) == 15u);
    BOOST_TEST(getStrandedSoldierReturnSearchRadius(ggs, 6) == 30u);
    BOOST_TEST(getStrandedSoldierReturnSearchRadius(ggs, 1) == 30u);
}

BOOST_AUTO_TEST_CASE(StrandedSoldierReturnSearchVeryLargeEscalatesThroughThreeStages)
{
    GlobalGameSettings ggs;
    ggs.setSelection(AddonId::STRANDED_SOLDIER_RETURN_SEARCH, 3);

    BOOST_TEST(getStrandedSoldierReturnSearchTryings(ggs) == 18u);
    BOOST_TEST(getStrandedSoldierReturnSearchRadius(ggs, 18) == 15u);
    BOOST_TEST(getStrandedSoldierReturnSearchRadius(ggs, 13) == 15u);
    BOOST_TEST(getStrandedSoldierReturnSearchRadius(ggs, 12) == 30u);
    BOOST_TEST(getStrandedSoldierReturnSearchRadius(ggs, 7) == 30u);
    BOOST_TEST(getStrandedSoldierReturnSearchRadius(ggs, 6) == 60u);
    BOOST_TEST(getStrandedSoldierReturnSearchRadius(ggs, 1) == 60u);
}

BOOST_FIXTURE_TEST_CASE(StartingToWanderAbrogatesGoal, WorldWithGCExecution1P)
{
    auto* wh = world.getPlayer(curPlayer).getFirstWH();
    BOOST_TEST_REQUIRE(wh);
    auto* flag = wh->getFlag();
    BOOST_TEST_REQUIRE(flag);

    auto& worker =
      world.addFigure(flag->getPos(),
                      std::make_unique<nofPassiveWorker>(Job::Helper, flag->getPos(), curPlayer, flag));
    BOOST_TEST_REQUIRE(worker.getGoal() == flag);

    worker.startWandering();

    BOOST_TEST(worker.isWandering());
    BOOST_TEST(worker.getGoal() == nullptr);
    // Must be safe after startWandering, i.e. no dangling goal/dependent figure is left behind
    worker.destroy();
}

BOOST_FIXTURE_TEST_CASE(DestroyWHWithFigure, WorldWithGCExecution2P)
{
    world.getPlayer(curPlayer).getFirstWH()->addToInventory(PeopleCounts::make(Job::Helper, 10), true);
    MapPoint flagPos = world.getNeighbour(hqPos, Direction::SouthEast);
    MapPoint whPos(flagPos.x + 5, flagPos.y);
    auto* wh = static_cast<nobBaseWarehouse*>(
      BuildingFactory::createBuilding(world, BuildingType::Storehouse, whPos, curPlayer, Nation::Romans));
    wh->addToInventory(PeopleCounts::make(Job::Helper, 1), true);
    const unsigned numHelpers = world.getPlayer(curPlayer).getInventory().people[Job::Helper]; //-V807
    MapPoint whFlagPos = world.getNeighbour(whPos, Direction::SouthEast);
    // Build a road -> Requests a worker
    this->BuildRoad(whFlagPos, false, std::vector<Direction>(2, Direction::West));
    BOOST_TEST_REQUIRE(wh->getNumRealFigures(Job::Helper) == 0u);
    BOOST_TEST_REQUIRE(wh->getLeavingFigures().size() == 1u);
    // Destroy Road
    this->DestroyFlag(whFlagPos - MapPoint(2, 0));
    BOOST_TEST_REQUIRE(wh->getNumRealFigures(Job::Helper) == 1u);
    BOOST_TEST_REQUIRE(wh->getLeavingFigures().size() == 0u);

    this->BuildRoad(whFlagPos, false, std::vector<Direction>(2, Direction::West));
    const noFigure* fig = &wh->getLeavingFigures().front();
    // Destroy wh -> Worker released
    this->DestroyFlag(whFlagPos);
    BOOST_TEST_REQUIRE(world.getPlayer(curPlayer).getInventory().people[Job::Helper] == numHelpers);
    BOOST_TEST_REQUIRE(fig->getPos() == whPos);
    BOOST_TEST_REQUIRE(fig->isWandering());

    // Same for HQ
    // Build a road -> Requests a worker
    this->BuildRoad(flagPos, false, std::vector<Direction>(2, Direction::West));
    wh = world.getSpecObj<nobBaseWarehouse>(hqPos);
    BOOST_TEST_REQUIRE(wh->getLeavingFigures().size() == 1u);
    fig = &wh->getLeavingFigures().front();
    // Destroy wh -> Worker released
    this->DestroyFlag(flagPos);
    BOOST_TEST_REQUIRE(world.getPlayer(curPlayer).getInventory().people[Job::Helper] == numHelpers);
    BOOST_TEST_REQUIRE(fig->getPos() == hqPos);
    BOOST_TEST_REQUIRE(fig->isWandering());
}

BOOST_FIXTURE_TEST_CASE(DestroyWHWithWare, WorldWithGCExecution2P)
{
    addStartResources();
    MapPoint flagPos = world.getNeighbour(hqPos, Direction::SouthEast);
    MapPoint whFlagPos(flagPos.x + 5, flagPos.y);
    MapPoint whPos = world.getNeighbour(whFlagPos, Direction::NorthWest);
    BuildingFactory::createBuilding(world, BuildingType::HarborBuilding, whPos, curPlayer, Nation::Romans);
    // Build a road
    this->BuildRoad(whFlagPos, false, std::vector<Direction>(5, Direction::West));
    // Request people and wares
    this->SetInventorySetting(whPos, GoodType::Wood, EInventorySetting::Collect);
    this->SetInventorySetting(whPos, Job::Woodcutter, EInventorySetting::Collect);
    auto* flag = world.getSpecObj<noFlag>(flagPos);
    RTTR_EXEC_TILL(200, flag->getNumWares() > 0);
    // Destroy wh -> Cancel wares and figures
    this->DestroyFlag(whFlagPos);
}

using EmptyWorldFixture1PBig = WorldFixture<CreateEmptyWorld, 1, 22, 20>;

BOOST_FIXTURE_TEST_CASE(ScoutScouts, EmptyWorldFixture1PBig)
{
    const MapPoint hqFlagPos = world.getNeighbour(world.getPlayer(0).getHQPos(), Direction::SouthEast);
    auto& scout = world.addFigure(
      hqFlagPos, std::make_unique<nofScout_Free>(hqFlagPos, 0, world.getSpecObj<noRoadNode>(hqFlagPos)));
    scout.actAtFirst();
    const auto countVisibleNodes = [&]() {
        unsigned numVisibleNodes = 0;
        RTTR_FOREACH_PT(MapPoint, world.getSize())
        {
            if(world.calcVisiblityWithAllies(pt, 0) != Visibility::Invisible)
                numVisibleNodes++;
        }
        return numVisibleNodes;
    };
    BOOST_TEST_REQUIRE(countVisibleNodes() < prodOfComponents(world.getSize())); // Must have scoutable nodes
    RTTR_SKIP_GFS(600);
    // All scouted
    BOOST_TEST(countVisibleNodes() == prodOfComponents(world.getSize()));
}

using EmptyWorldFixture1P = WorldFixture<CreateEmptyWorld, 1>;

BOOST_FIXTURE_TEST_CASE(GeologistPlacesSigns, EmptyWorldFixture1P)
{
    const MapPoint hqFlagPos = world.getNeighbour(world.getPlayer(0).getHQPos(), Direction::SouthEast);
    const MapPoint waterPos = world.getNeighbour(hqFlagPos, Direction::NorthEast);
    const MapPoint coalPos = world.getNeighbour(hqFlagPos, Direction::East);
    const MapPoint ironPos = world.getNeighbour(hqFlagPos, Direction::SouthEast);
    const MapPoint goldPos = world.getNeighbour(hqFlagPos, Direction::SouthWest);
    const MapPoint granitePos = world.getNeighbour2(hqFlagPos, 6); // A bit further
    world.getNodeWriteable(waterPos).resources = Resource(ResourceType::Water, rttr::test::randomValue(1u, 15u));
    world.getNodeWriteable(coalPos).resources = Resource(ResourceType::Coal, rttr::test::randomValue(1u, 15u));
    world.getNodeWriteable(ironPos).resources = Resource(ResourceType::Iron, rttr::test::randomValue(1u, 15u));
    world.getNodeWriteable(goldPos).resources = Resource(ResourceType::Gold, rttr::test::randomValue(1u, 15u));
    world.getNodeWriteable(granitePos).resources = Resource(ResourceType::Granite, rttr::test::randomValue(1u, 15u));
    // Add some geologists
    for(unsigned i = 0; i < 10; i++)
    {
        world
          .addFigure(hqFlagPos, std::make_unique<nofGeologist>(hqFlagPos, 0, world.getSpecObj<noRoadNode>(hqFlagPos)))
          .actAtFirst();
    }

    std::vector<ResourceNote> notes;
    const auto sub =
      world.getNotifications().subscribe<ResourceNote>([&notes](const ResourceNote& note) { notes.push_back(note); });

    RTTR_SKIP_GFS(800);
    // All discovered
    BOOST_TEST(notes.size() >= 5u);
    // All from this player
    BOOST_TEST(!helpers::containsIf(notes, [](const ResourceNote& note) { return note.player != 0; }));
    BOOST_TEST(helpers::containsIf(notes, [&](const ResourceNote& note) { return note.pos == waterPos; }));
    BOOST_TEST(helpers::containsIf(notes, [&](const ResourceNote& note) { return note.pos == coalPos; }));
    BOOST_TEST(helpers::containsIf(notes, [&](const ResourceNote& note) { return note.pos == ironPos; }));
    BOOST_TEST(helpers::containsIf(notes, [&](const ResourceNote& note) { return note.pos == goldPos; }));
    BOOST_TEST(helpers::containsIf(notes, [&](const ResourceNote& note) { return note.pos == granitePos; }));
    for(const ResourceNote& note : notes)
    {
        BOOST_TEST(note.res == world.getNode(note.pos).resources);
        const auto* sign = world.getSpecObj<noSign>(note.pos);
        BOOST_TEST_REQUIRE(sign);
        BOOST_TEST(sign->getResource() == note.res);
    }
}

using BoatCarrierLostWorkWorld = WorldFixture<CreateWaterWorld, 1, 40, 20>;
BOOST_FIXTURE_TEST_CASE(BoatCarrierLostWork_ShoreWanders_OpenWaterDrowns, BoatCarrierLostWorkWorld)
{
    auto& player = world.getPlayer(0);
    // A boat carrier is a helper plus a boat, so the player owns both for each one
    player.increaseInventoryJob(Job::Helper, 2);
    player.increaseInventoryWare(GoodType::Boat, 2);

    // Close enough to shore: Paddle to shore and walk to HQ
    const MapPoint hqFlagPos = player.getFirstWH()->getFlagPos();
    MapPoint shorePt = hqFlagPos;
    // Find shore right of flag
    for(const auto x : helpers::range<MapCoord>(shorePt.x + 1, world.getSize().x))
    {
        shorePt.x = x;
        if(world.getSeaFromCoastalPoint(shorePt))
            break;
    }
    BOOST_TEST_REQUIRE(world.getSeaFromCoastalPoint(shorePt));
    // Place a road far in the water
    const auto halfRoadLen = 10;
    for(const auto x : helpers::range<MapCoord>(1, halfRoadLen))
        BOOST_TEST_REQUIRE(world.isWaterPoint(world.makeMapPoint(shorePt + Offset(x, 0))));
    std::vector<Direction> route(halfRoadLen, Direction::East);
    route.insert(route.end(), 2, Direction::SouthWest);
    route.insert(route.end(), halfRoadLen, Direction::West);
    MapPoint endPt = shorePt;
    for(const Direction dir : route)
    {
        for(const auto pt : world.getPointsInRadiusWithCenter(endPt, 1))
            world.getNodeWriteable(pt).owner = 1;
        endPt = world.getNeighbour(endPt, dir);
    }
    BOOST_TEST_REQUIRE(world.SetFlag(shorePt, 0));
    auto& shoreFlag = ensureNonNull(world.getSpecObj<noFlag>(shorePt));
    enum SubTest
    {
        AtFlag,
        CloseToShore,
        OpenWater
    };
    for(const auto subTest : {AtFlag, CloseToShore, OpenWater})
        BOOST_TEST_CONTEXT("subTest=" << subTest)
        {
            BOOST_TEST_REQUIRE(world.BuildRoad(0, true, shorePt, route).isValid());
            auto& carrier =
              world.addFigure(shorePt, std::make_unique<nofCarrier>(CarrierType::Boat, shorePt, 0,
                                                                    shoreFlag.getRoute(Direction::East), &shoreFlag));
            shoreFlag.getRoute(Direction::East)->setCarrier(0, &carrier);
            carrier.actAtFirst();
            BOOST_TEST_REQUIRE(carrier.getCarrierState() == CarrierState::GotoMiddleOfRoad);
            BOOST_TEST_REQUIRE(carrier.isMoving());
            MapPoint carrierPos;
            if(subTest == AtFlag)
                carrierPos = shorePt;
            else
                carrierPos = world.makeMapPoint(shorePt + Offset(subTest == CloseToShore ? 3 : halfRoadLen, 0));
            // Put such that it arrives at this pos on the next walk event
            const auto carrierFromPos = world.getNeighbour(carrierPos, carrier.getCurMoveDir() + 3u);
            world.addFigure(carrierFromPos, world.removeFigure(carrier.getPos(), carrier));
            carrier.setPos(carrierFromPos);

            rescheduleWalkEvent(carrier, 1);
            shoreFlag.DestroyRoad(Direction::East);
            RTTR_SKIP_GFS(1); // Walking/death decisions happen when arrived at next node

            if(subTest == OpenWater)
            {
                // It drowns: removed from the world
                BOOST_TEST_REQUIRE(!world.hasFigureAt(carrierPos, carrier));
            } else
            {
                // Still alive
                BOOST_TEST_REQUIRE(world.hasFigureAt(carrierPos, carrier));
                if(subTest == CloseToShore)
                    BOOST_TEST(carrier.getCarrierState() == CarrierState::BoatcarrierWanderOnWater);
                else
                    BOOST_TEST(carrier.isWandering());
            }
        }
}

BOOST_AUTO_TEST_SUITE_END()
