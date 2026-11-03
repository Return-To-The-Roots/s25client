// Copyright (C) 2005 - 2026 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "GameCommand.h"
#include "GamePlayer.h"
#include "PointOutput.h"
#include "RttrForeachPt.h"
#include "SerializedGameData.h"
#include "addons/const_addons.h"
#include "ai/AIPlayer.h"
#include "buildings/nobBaseWarehouse.h"
#include "buildings/nobMilitary.h"
#include "factories/AIFactory.h"
#include "factories/BuildingFactory.h"
#include "figures/nofCarrier.h"
#include "figures/nofPassiveSoldier.h"
#include "helpers/EnumRange.h"
#include "helpers/pointerContainerUtils.h"
#include "worldFixtures/MockLocalGameState.h"
#include "worldFixtures/WorldWithGCExecution.h"
#include "worldFixtures/terrainHelpers.h"
#include "world/GameWorld.h"
#include "world/MapLoader.h"
#include "nodeObjs/noFlag.h"
#include "gameTypes/BuildingCount.h"
#include "gameTypes/GameTypesOutput.h"
#include "gameData/MilitaryConsts.h"
#include "gameData/SettingTypeConv.h"
#include "gameData/TerrainDesc.h"
#include "gameData/WorldDescription.h"
#include <boost/test/unit_test.hpp>
#include <vector>

// LCOV_EXCL_START
BOOST_TEST_DONT_PRINT_LOG_VALUE(CarrierState)
BOOST_TEST_DONT_PRINT_LOG_VALUE(CarrierType)
BOOST_TEST_DONT_PRINT_LOG_VALUE(RoadType)
// LCOV_EXCL_STOP

namespace {
/// World with the HQ on land in the west and an island in the east, which can only be reached over a waterway:
/// HQ flag -(road)- western flag -(waterway)- eastern flag -(island road)- island flag
/// Everything else is water, so figures can't walk to the island
struct BoatTransportFixture : public WorldWithGCExecution<1, 40, 16>
{
    MapPoint hqFlagPos, westFlagPos, eastFlagPos, islandFlagPos;
    /// Direction of the island road starting at the eastern flag
    const Direction islandRoadDir;
    /// Middle point of the waterway
    MapPoint waterwayMiddlePos;

    explicit BoatTransportFixture(const unsigned waterwayLength = 4,
                                  const Direction islandRoadDir = Direction::NorthEast)
        : islandRoadDir(islandRoadDir)
    {
        ggs.setSelection(AddonId::BOATS_TRANSPORT_FIGURES, 1);
        hqFlagPos = world.GetNeighbour(hqPos, Direction::SouthEast);
        westFlagPos = world.MakeMapPoint(Position(hqFlagPos) + Position(2, 0));
        eastFlagPos = world.MakeMapPoint(Position(westFlagPos) + Position(waterwayLength, 0));
        islandFlagPos = world.GetNeighbour(world.GetNeighbour(eastFlagPos, islandRoadDir), islandRoadDir);
        waterwayMiddlePos = world.MakeMapPoint(Position(westFlagPos) + Position(waterwayLength / 2, 0));

        const auto tWater = GetWaterTerrain(world.GetDescription());
        RTTR_FOREACH_PT(MapPoint, world.GetSize())
        {
            // Land from a bit west of the HQ to the western flag and a small island east of the eastern flag
            const bool isLand = (pt.x + 7 >= westFlagPos.x && pt.x < westFlagPos.x)
                                || (pt.x > eastFlagPos.x && pt.x <= eastFlagPos.x + 4);
            if(!isLand)
            {
                MapNode& node = world.GetNodeWriteable(pt);
                node.t1 = node.t2 = tWater;
            }
        }
        // Required for boat carriers to find the shore
        BOOST_TEST_REQUIRE(MapLoader::InitSeasAndHarbors(world));
        world.InitAfterLoad();
    }

    /// Use shallow water around the middle point of the waterway: Flags can be placed there, but ships can't pass
    void makeWaterwayMiddleShallow()
    {
        const auto tShallowWater = world.GetDescription().terrain.find(
          [](const TerrainDesc& t) { return t.kind == TerrainKind::Water && t.Is(ETerrain::Buildable); });
        BOOST_TEST_REQUIRE(tShallowWater);
        for(const Direction dir : helpers::EnumRange<Direction>{})
            setRightTerrain(world, waterwayMiddlePos, dir, tShallowWater);
        world.InitAfterLoad();
    }

    nobBaseWarehouse& getHQ() { return *world.GetSpecObj<nobBaseWarehouse>(hqPos); }
    const noFlag& getFlag(const MapPoint pt) const { return *world.GetSpecObj<noFlag>(pt); }
    RoadSegment* getWaterway() const { return getFlag(westFlagPos).GetRoute(Direction::East); }
    RoadSegment* getIslandRoad() const { return getFlag(eastFlagPos).GetRoute(islandRoadDir); }
    bool isWaitingAtWestFlag(const noFigure& figure) const
    {
        return helpers::containsPtr(getFlag(westFlagPos).GetFiguresForBoats(), &figure);
    }

    void addToHQ(const unsigned numHelpers, const unsigned numBoats)
    {
        getHQ().AddToInventory(PeopleCounts::make(Job::Helper, numHelpers), true);
        getHQ().AddToInventory(GoodCounts::make(GoodType::Boat, numBoats), true);
    }

    /// Build the roads to the island. The waterway is either built from west to east or the other way round
    void buildRoads(const bool waterwayFromEast = false)
    {
        this->BuildRoad(hqFlagPos, false, std::vector<Direction>(2, Direction::East));
        const auto waterwayLength = static_cast<unsigned>(world.CalcDistance(westFlagPos, eastFlagPos));
        if(waterwayFromEast)
        {
            this->SetFlag(eastFlagPos);
            this->BuildRoad(eastFlagPos, true, std::vector<Direction>(waterwayLength, Direction::West));
        } else
            this->BuildRoad(westFlagPos, true, std::vector<Direction>(waterwayLength, Direction::East));
        this->BuildRoad(eastFlagPos, false, std::vector<Direction>(2, islandRoadDir));
        BOOST_TEST_REQUIRE(getWaterway());
        BOOST_TEST_REQUIRE(getWaterway()->GetRoadType() == RoadType::Water);
        BOOST_TEST_REQUIRE(getWaterway()->GetLength() == waterwayLength);
        BOOST_TEST_REQUIRE(getIslandRoad());
        BOOST_TEST_REQUIRE(getIslandRoad()->GetRoadType() == RoadType::Normal);
    }

    /// Execute GFs till the given figure (going to the HQ) arrived there
    void waitTillArrivedAtHQ(const noFigure& figure, const unsigned maxGFs)
    {
        const nobBaseWarehouse& hq = getHQ();
        BOOST_TEST_REQUIRE(figure.GetGoal() == &hq);
        BOOST_TEST_REQUIRE(hq.IsDependentFigure(figure));
        // Figure gets destroyed on arrival, so only use its address
        const noFigure* figurePtr = &figure;
        RTTR_EXEC_TILL(maxGFs, !hq.IsDependentFigure(*figurePtr));
    }

    /// Save the game, load it again and check that saving the loaded game results in the same data
    void saveAndLoad()
    {
        SerializedGameData sgd;
        sgd.MakeSnapshot(*game);
        MockLocalGameState lgs;
        em.Clear();
        world.Unload();
        sgd.ReadSnapshot(*game, lgs);

        SerializedGameData sgd2;
        sgd2.MakeSnapshot(*game);
        BOOST_CHECK_EQUAL_COLLECTIONS(sgd.GetData(), sgd.GetData() + sgd.GetLength(), sgd2.GetData(),
                                      sgd2.GetData() + sgd2.GetLength());
    }
};

/// Like BoatTransportFixture but with a long waterway leading out of the territory of the HQ.
/// The island and the eastern part of the waterway only belong to us because of a barracks on the island.
struct BarracksOnIslandFixture : public BoatTransportFixture
{
    MapPoint barracksPos;

    BarracksOnIslandFixture() : BoatTransportFixture(9, Direction::East)
    {
        // Island road ends at the flag of the barracks
        barracksPos = world.GetNeighbour(islandFlagPos, Direction::NorthWest);
        auto* barracks = static_cast<nobMilitary*>(
          BuildingFactory::CreateBuilding(world, BuildingType::Barracks, barracksPos, curPlayer, Nation::Romans));
        BOOST_TEST_REQUIRE(barracks->GetFlagPos() == islandFlagPos);
        // Occupy it
        auto& soldier = world.AddFigure(
          barracksPos, std::make_unique<nofPassiveSoldier>(barracksPos, curPlayer, barracks, barracks, 0));
        world.GetPlayer(curPlayer).IncreaseInventoryJob(soldier.GetJobType(), 1);
        barracks->GotWorker(soldier.GetJobType(), soldier);
        soldier.WalkToGoal();
        BOOST_TEST_REQUIRE(barracks->GetNumTroops() == 1u);
        // Fully occupy all military buildings, so the barracks wants another soldier
        this->ChangeMilitary(MILITARY_SETTINGS_SCALE);
        BOOST_TEST_REQUIRE(world.IsPlayerTerritory(eastFlagPos, curPlayer + 1));
        // Waterway leads out of the territory of the HQ
        BOOST_TEST_REQUIRE(world.CalcDistance(hqPos, eastFlagPos) > HQ_RADIUS);
    }
};
} // namespace

BOOST_AUTO_TEST_SUITE(BoatTransportSuite)

BOOST_FIXTURE_TEST_CASE(BoatCarriesFigureOverWaterway, BoatTransportFixture)
{
    addToHQ(3, 1);
    buildRoads();
    // Both roads get a carrier: The island road can be reached via the waterway
    BOOST_TEST_REQUIRE(getWaterway()->hasCarrier(0));
    BOOST_TEST_REQUIRE(getIslandRoad()->hasCarrier(0));
    const nofCarrier& boatCarrier = *getWaterway()->getCarrier(0);
    const nofCarrier& islandCarrier = *getIslandRoad()->getCarrier(0);
    BOOST_TEST(boatCarrier.GetCarrierType() == CarrierType::Boat);
    BOOST_TEST(islandCarrier.GetCarrierType() == CarrierType::Normal);

    bool waitedAtFlag = false;
    bool wasCarried = false;
    const auto islandCarrierArrived = [&]() {
        if(isWaitingAtWestFlag(islandCarrier))
        {
            waitedAtFlag = true;
            BOOST_TEST_REQUIRE(islandCarrier.IsWaitingForBoat());
            BOOST_TEST_REQUIRE(!world.HasFigureAt(westFlagPos, islandCarrier));
        }
        if(boatCarrier.GetCarriedFigure() == &islandCarrier)
        {
            wasCarried = true;
            BOOST_TEST_REQUIRE(!islandCarrier.IsWaitingForBoat());
        }
        return islandCarrier.GetCarrierState() != CarrierState::FigureWork;
    };
    RTTR_EXEC_TILL(2000, islandCarrierArrived());
    BOOST_TEST(waitedAtFlag);
    BOOST_TEST(wasCarried);
    BOOST_TEST(getFlag(westFlagPos).GetFiguresForBoats().empty());
    BOOST_TEST(!boatCarrier.GetCarriedFigure());
    BOOST_TEST(islandCarrier.GetCurrentRoad() == getIslandRoad());
}

BOOST_FIXTURE_TEST_CASE(NoTransportWithoutAddon, BoatTransportFixture)
{
    ggs.setSelection(AddonId::BOATS_TRANSPORT_FIGURES, 0);
    addToHQ(3, 1);
    buildRoads();
    // Waterways are still used for wares
    BOOST_TEST(getWaterway()->hasCarrier(0));
    // But nobody can get to the island road
    BOOST_TEST(!getIslandRoad()->hasCarrier(0));
    RTTR_SKIP_GFS(500);
    BOOST_TEST(!getIslandRoad()->hasCarrier(0));
    BOOST_TEST(getFlag(westFlagPos).GetFiguresForBoats().empty());
}

BOOST_FIXTURE_TEST_CASE(WaitForBoatCarrier, BoatTransportFixture)
{
    // No boat -> No boat carrier
    addToHQ(2, 0);
    buildRoads();
    BOOST_TEST_REQUIRE(!getWaterway()->hasCarrier(0));
    // The island road is only reachable via the waterway, so it is used although there is no boat carrier (yet)
    BOOST_TEST_REQUIRE(getIslandRoad()->hasCarrier(0));
    const nofCarrier& islandCarrier = *getIslandRoad()->getCarrier(0);
    RTTR_EXEC_TILL(200, isWaitingAtWestFlag(islandCarrier));
    BOOST_TEST(islandCarrier.IsWaitingForBoat());
    BOOST_TEST(islandCarrier.GetCurrentRoad() == getWaterway());
    // Nothing happens until a boat carrier arrives
    RTTR_SKIP_GFS(200);
    BOOST_TEST_REQUIRE(isWaitingAtWestFlag(islandCarrier));

    // Now we get a boat and hence a boat carrier which takes the waiting carrier to the island
    getHQ().AddToInventory(GoodCounts::make(GoodType::Boat, 1), true);
    world.GetPlayer(curPlayer).FindCarrierForAllRoads();
    BOOST_TEST_REQUIRE(getWaterway()->hasCarrier(0));
    RTTR_EXEC_TILL(1000, islandCarrier.GetCarrierState() != CarrierState::FigureWork);
    BOOST_TEST(getFlag(westFlagPos).GetFiguresForBoats().empty());
}

BOOST_FIXTURE_TEST_CASE(WaterwayDestroyedWhileWaiting, BoatTransportFixture)
{
    addToHQ(2, 0);
    buildRoads();
    const nofCarrier& islandCarrier = *getIslandRoad()->getCarrier(0);
    RTTR_EXEC_TILL(200, isWaitingAtWestFlag(islandCarrier));

    // Destroy the waterway -> Figure gets back into the world and wanders like figures on destroyed roads
    this->DestroyRoad(westFlagPos, Direction::East);
    BOOST_TEST_REQUIRE(!getWaterway());
    BOOST_TEST(getFlag(westFlagPos).GetFiguresForBoats().empty());
    BOOST_TEST(!islandCarrier.IsWaitingForBoat());
    BOOST_TEST(world.HasFigureAt(westFlagPos, islandCarrier));
    BOOST_TEST(islandCarrier.IsWandering());
    // Nobody can get to the island road anymore
    BOOST_TEST(!getIslandRoad()->hasCarrier(0));

    // The helper finds its way home
    RTTR_EXEC_TILL(2000, islandCarrier.GetGoal() == &getHQ());
    waitTillArrivedAtHQ(islandCarrier, 500);
}

BOOST_FIXTURE_TEST_CASE(FlagDestroyedWhileWaiting, BoatTransportFixture)
{
    addToHQ(2, 0);
    buildRoads();
    const nofCarrier& islandCarrier = *getIslandRoad()->getCarrier(0);
    RTTR_EXEC_TILL(200, isWaitingAtWestFlag(islandCarrier));

    // Destroying the flag destroys all its roads
    this->DestroyFlag(westFlagPos);
    BOOST_TEST_REQUIRE(!world.GetSpecObj<noFlag>(westFlagPos));
    BOOST_TEST(!islandCarrier.IsWaitingForBoat());
    BOOST_TEST(world.HasFigureAt(westFlagPos, islandCarrier));
    BOOST_TEST(islandCarrier.IsWandering());

    // Can still get home
    RTTR_EXEC_TILL(2000, islandCarrier.GetGoal() == &getHQ());
    waitTillArrivedAtHQ(islandCarrier, 500);
}

BOOST_FIXTURE_TEST_CASE(GoHomeWhileWaiting, BoatTransportFixture)
{
    addToHQ(2, 0);
    buildRoads();
    const nofCarrier& islandCarrier = *getIslandRoad()->getCarrier(0);
    RTTR_EXEC_TILL(200, isWaitingAtWestFlag(islandCarrier));

    // Destroy the island road -> The carrier isn't needed anymore and goes home without waiting for the boat
    this->DestroyRoad(eastFlagPos, islandRoadDir);
    BOOST_TEST(getFlag(westFlagPos).GetFiguresForBoats().empty());
    BOOST_TEST(!islandCarrier.IsWaitingForBoat());
    BOOST_TEST(world.HasFigureAt(westFlagPos, islandCarrier));
    BOOST_TEST(!islandCarrier.IsWandering());
    waitTillArrivedAtHQ(islandCarrier, 200);
}

BOOST_FIXTURE_TEST_CASE(GoalDestroyedWhileCarried, BoatTransportFixture)
{
    addToHQ(3, 1);
    buildRoads();
    const nofCarrier& boatCarrier = *getWaterway()->getCarrier(0);
    const nofCarrier& islandCarrier = *getIslandRoad()->getCarrier(0);
    RTTR_EXEC_TILL(1000, boatCarrier.GetCarriedFigure() == &islandCarrier);

    // Destroy the island road -> The carrier goes home once he reaches the island
    this->DestroyRoad(eastFlagPos, islandRoadDir);
    BOOST_TEST(boatCarrier.GetCarriedFigure() == &islandCarrier);
    // So he gets carried back
    waitTillArrivedAtHQ(islandCarrier, 1000);
    BOOST_TEST(!boatCarrier.GetCarriedFigure());
}

BOOST_FIXTURE_TEST_CASE(WaterwayDestroyedWhileCarried, BoatTransportFixture)
{
    addToHQ(3, 1);
    buildRoads();
    const nofCarrier& boatCarrier = *getWaterway()->getCarrier(0);
    const nofCarrier& islandCarrier = *getIslandRoad()->getCarrier(0);
    RTTR_EXEC_TILL(1000, boatCarrier.GetCarriedFigure() == &islandCarrier);
    // Wait till the boat is on the water
    RTTR_EXEC_TILL(100, boatCarrier.GetPos() != westFlagPos);

    // Destroy the waterway -> Boat carrier paddles to the shore with the figure which then wanders around
    this->DestroyRoad(westFlagPos, Direction::East);
    BOOST_TEST_REQUIRE(boatCarrier.GetCarriedFigure() == &islandCarrier);
    BOOST_TEST(islandCarrier.IsWandering());
    RTTR_EXEC_TILL(100, !boatCarrier.GetCarriedFigure());
    BOOST_TEST(islandCarrier.IsWandering());
    BOOST_TEST(world.HasFigureAt(islandCarrier.GetPos(), islandCarrier));
    BOOST_TEST(!world.IsWaterPoint(islandCarrier.GetPos()));
    // They might die or find a way home, but this must not fail
    RTTR_SKIP_GFS(2000);
}

BOOST_FIXTURE_TEST_CASE(WaterwayDestroyedWhileCarriedAndShoreUnreachable, BoatTransportFixture)
{
    makeWaterwayMiddleShallow();
    addToHQ(3, 1);
    buildRoads();
    const nofCarrier& boatCarrier = *getWaterway()->getCarrier(0);
    const nofCarrier& islandCarrier = *getIslandRoad()->getCarrier(0);
    RTTR_EXEC_TILL(1000, boatCarrier.GetCarriedFigure() == &islandCarrier);
    // Wait till the boat moves to the shallow water from where ships can't reach the shore
    RTTR_EXEC_TILL(100, world.GetNeighbour(boatCarrier.GetPos(), boatCarrier.GetCurMoveDir()) == waterwayMiddlePos);

    // Destroy the waterway -> The figure gets out of the boat immediately
    this->DestroyRoad(westFlagPos, Direction::East);
    BOOST_TEST(!boatCarrier.GetCarriedFigure());
    // They will die, but this must not fail
    RTTR_SKIP_GFS(2000);
}

BOOST_FIXTURE_TEST_CASE(SplitWaterwayWithWaitingFigure, BoatTransportFixture)
{
    makeWaterwayMiddleShallow();
    addToHQ(4, 0);
    // Build the waterway from the island, so the western flag is at its end
    buildRoads(true);
    BOOST_TEST_REQUIRE(getWaterway()->GetF2() == world.GetSpecObj<noFlag>(westFlagPos));
    const nofCarrier& islandCarrier = *getIslandRoad()->getCarrier(0);
    RTTR_EXEC_TILL(200, isWaitingAtWestFlag(islandCarrier));

    // Split the waterway -> The figure now waits for the new part starting at the western flag
    this->SetFlag(waterwayMiddlePos);
    BOOST_TEST_REQUIRE(world.GetSpecObj<noFlag>(waterwayMiddlePos));
    BOOST_TEST_REQUIRE(getWaterway()->GetRoadType() == RoadType::Water);
    BOOST_TEST_REQUIRE(getWaterway()->GetLength() == 2u);
    BOOST_TEST_REQUIRE(isWaitingAtWestFlag(islandCarrier));
    BOOST_TEST(islandCarrier.GetCurrentRoad() == getWaterway());

    // Get boats: Figure gets carried over both parts
    getHQ().AddToInventory(GoodCounts::make(GoodType::Boat, 2), true);
    world.GetPlayer(curPlayer).FindCarrierForAllRoads();
    BOOST_TEST_REQUIRE(getWaterway()->hasCarrier(0));
    BOOST_TEST_REQUIRE(getFlag(waterwayMiddlePos).GetRoute(Direction::East)->hasCarrier(0));
    RTTR_EXEC_TILL(1000, islandCarrier.GetCarrierState() != CarrierState::FigureWork);
    BOOST_TEST(getFlag(westFlagPos).GetFiguresForBoats().empty());
    BOOST_TEST(getFlag(waterwayMiddlePos).GetFiguresForBoats().empty());
}

BOOST_FIXTURE_TEST_CASE(SaveAndLoadFiguresForBoats, BoatTransportFixture)
{
    addToHQ(3, 1);
    buildRoads();
    RTTR_EXEC_TILL(1000, !getFlag(westFlagPos).GetFiguresForBoats().empty());
    // Figure waiting at the flag
    saveAndLoad();
    BOOST_TEST_REQUIRE(getFlag(westFlagPos).GetFiguresForBoats().size() == 1u);
    const noFigure& waitingFigure = *getFlag(westFlagPos).GetFiguresForBoats().front();
    BOOST_TEST(waitingFigure.IsWaitingForBoat());
    BOOST_TEST(waitingFigure.GetCurrentRoad() == getWaterway());

    // Figure in the boat
    RTTR_EXEC_TILL(1000, getWaterway()->getCarrier(0)->GetCarriedFigure());
    saveAndLoad();
    const nofCarrier& boatCarrier = *getWaterway()->getCarrier(0);
    BOOST_TEST_REQUIRE(boatCarrier.GetCarriedFigure());
    BOOST_TEST(boatCarrier.GetCarriedFigure() == getIslandRoad()->getCarrier(0));

    // And it still arrives
    const nofCarrier& islandCarrier = *getIslandRoad()->getCarrier(0);
    RTTR_EXEC_TILL(1000, islandCarrier.GetCarrierState() != CarrierState::FigureWork);
}

BOOST_FIXTURE_TEST_CASE(SoldiersGetCarriedToIsland, BarracksOnIslandFixture)
{
    addToHQ(3, 1);
    getHQ().AddToInventory(PeopleCounts::make(Job::Private, 2), true);
    buildRoads();
    const auto isSoldier = [](const noFigure* figure) { return figure && figure->IsSoldier(); };
    const auto isSoldierWaiting = [&]() {
        return helpers::contains_if(getFlag(westFlagPos).GetFiguresForBoats(),
                                    [&](const auto& figure) { return isSoldier(figure.get()); });
    };
    // Barracks can be reached now, so it orders the missing soldier who waits for the boat...
    RTTR_EXEC_TILL(3000, isSoldierWaiting());
    saveAndLoad();
    BOOST_TEST_REQUIRE(isSoldierWaiting());
    // ...then gets carried...
    RTTR_EXEC_TILL(1000, isSoldier(getWaterway()->getCarrier(0)->GetCarriedFigure()));
    saveAndLoad();
    BOOST_TEST_REQUIRE(isSoldier(getWaterway()->getCarrier(0)->GetCarriedFigure()));
    // ...and finally reaches the barracks
    const auto& barracks = *world.GetSpecObj<nobMilitary>(barracksPos);
    RTTR_EXEC_TILL(1000, barracks.GetNumTroops() == 2u);
}

BOOST_FIXTURE_TEST_CASE(TerritoryLostWhileCarried, BarracksOnIslandFixture)
{
    addToHQ(3, 1);
    getHQ().AddToInventory(PeopleCounts::make(Job::Private, 2), true);
    buildRoads();
    const nofCarrier& boatCarrier = *getWaterway()->getCarrier(0);
    // Someone in the boat and someone waiting for it
    RTTR_EXEC_TILL(3000, boatCarrier.GetCarriedFigure() && !getFlag(westFlagPos).GetFiguresForBoats().empty());

    // Demolish the barracks -> The island and hence the waterway is lost
    this->DestroyBuilding(barracksPos);
    BOOST_TEST_REQUIRE(!world.GetSpecObj<nobMilitary>(barracksPos));
    BOOST_TEST_REQUIRE(world.GetNode(eastFlagPos).owner == 0u);
    BOOST_TEST_REQUIRE(!world.GetSpecObj<noFlag>(eastFlagPos));
    BOOST_TEST_REQUIRE(!getWaterway());
    BOOST_TEST(getFlag(westFlagPos).GetFiguresForBoats().empty());
    // The boat carrier paddles to the shore where they get out of the boat
    RTTR_EXEC_TILL(100, !boatCarrier.GetCarriedFigure());
    // Everyone finds a way home or dies, but this must not fail
    RTTR_SKIP_GFS(3000);
    BOOST_TEST(getFlag(westFlagPos).GetFiguresForBoats().empty());
}

BOOST_FIXTURE_TEST_CASE(AIPlaysWithWaterway, BoatTransportFixture)
{
    addStartResources();
    buildRoads();
    // The AI itself doesn't build waterways, but it must cope with them and the figures carried over them
    auto ai = AIFactory::Create(AI::Info(AI::Type::Default, AI::Level::Hard), curPlayer, world);
    for(unsigned gf = 0; gf < 5000;)
    {
        std::vector<gc::GameCommandPtr> aiGcs = ai->FetchGameCommands();
        for(unsigned i = 0; i < 5; i++, gf++)
        {
            em.ExecuteNextGF();
            ai->RunGF(em.GetCurrentGF(), i == 0);
        }
        for(gc::GameCommandPtr& gc : aiGcs)
            gc->Execute(world, curPlayer);
    }
    const GamePlayer& player = world.GetPlayer(curPlayer);
    BOOST_TEST(!player.IsDefeated());
    // The AI did something
    const BuildingCount bldCounts = player.GetBuildingRegister().GetBuildingNums();
    unsigned numBlds = 0;
    for(const auto bldType : helpers::enumRange<BuildingType>())
        numBlds += bldCounts.buildings[bldType] + bldCounts.buildingSites[bldType];
    BOOST_TEST(numBlds > 1u);
    saveAndLoad();
}

BOOST_FIXTURE_TEST_CASE(HQDestroyedWhileCarried, BoatTransportFixture)
{
    addToHQ(4, 1);
    buildRoads();
    const nofCarrier& boatCarrier = *getWaterway()->getCarrier(0);
    RTTR_EXEC_TILL(1000, boatCarrier.GetCarriedFigure());

    // Destroy the HQ -> All land is lost
    world.DestroyNO(hqPos);
    BOOST_TEST_REQUIRE(!world.GetSpecObj<noFlag>(westFlagPos));
    BOOST_TEST_REQUIRE(!world.GetSpecObj<noFlag>(eastFlagPos));
    BOOST_TEST(world.GetPlayer(curPlayer).IsDefeated());
    // Everyone will die, but this must not fail
    RTTR_SKIP_GFS(3000);
}

BOOST_AUTO_TEST_SUITE_END()
