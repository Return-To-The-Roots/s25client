// Copyright (C) 2005 - 2026 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "GamePlayer.h"
#include "RTTR_AssertError.h"
#include "buildings/nobBaseWarehouse.h"
#include "factories/BuildingFactory.h"
#include "figures/nofPassiveSoldier.h"
#include "figures/nofScout_Free.h"
#include "worldFixtures/CreateEmptyWorld.h"
#include "worldFixtures/WorldFixture.h"
#include "worldFixtures/WorldWithGCExecution.h"
#include "gameTypes/GoodTypes.h"
#include "gameTypes/JobTypes.h"
#include "gameData/ShieldConsts.h"
#include <rttr/test/LogAccessor.hpp>
#include <boost/test/unit_test.hpp>
#include <array>
#include <gameData/SettingTypeConv.h>

namespace {
struct AddGoodsFixture : public WorldFixture<CreateEmptyWorld, 1>, public rttr::test::LogAccessor
{
    helpers::EnumArray<unsigned, Job> numPeople, numPeoplePlayer;
    helpers::EnumArray<unsigned, GoodType> numGoods, numGoodsPlayer;
    helpers::EnumArray<unsigned, ArmoredSoldier> numArmoredSoldiers, numArmoredSoldiersPlayer;
    AddGoodsFixture()
    {
        GamePlayer& player = world.GetPlayer(0);
        // Don't keep any reserve
        for(unsigned i = 0; i <= this->ggs.GetMaxMilitaryRank(); ++i)
            player.GetFirstWH()->SetRealReserve(i, 0); //-V522
        numPeople = numPeoplePlayer = player.GetInventory().people;
        numGoods = numGoodsPlayer = player.GetInventory().goods;
        numArmoredSoldiers = numArmoredSoldiersPlayer = player.GetInventory().armoredSoldiers;
    }

    /// Asserts that the expected and actual good count match for the HQ
    void testNumGoodsHQ()
    {
        nobBaseWarehouse& hq = *world.GetSpecObj<nobBaseWarehouse>(world.GetPlayer(0).GetHQPos());
        for(const auto i : helpers::enumRange<Job>())
            BOOST_TEST_CONTEXT("Job: " << rttr::enum_cast(i))
            {
                BOOST_TEST(hq.GetNumVisualFigures(i) == numPeople[i]);
                BOOST_TEST(hq.GetNumRealFigures(i) == numPeople[i]);
            }
        for(const auto i : helpers::enumRange<ArmoredSoldier>())
            BOOST_TEST_CONTEXT("ArmoredSoldier: " << rttr::enum_cast(i))
            {
                BOOST_TEST(hq.GetNumVisualArmoredFigures(i) == numArmoredSoldiers[i]);
                BOOST_TEST(hq.GetNumRealArmoredFigures(i) == numArmoredSoldiers[i]);
            }
        for(const auto i : helpers::enumRange<GoodType>())
            BOOST_TEST_CONTEXT("Good: " << rttr::enum_cast(i))
            {
                BOOST_TEST(hq.GetNumVisualWares(i) == numGoods[i]);
                BOOST_TEST(hq.GetNumRealWares(i) == numGoods[i]);
            }
    }
    /// Asserts that the expected and actual good count match for the player
    void testNumGoodsPlayer()
    {
        GamePlayer& player = world.GetPlayer(0);
        for(const auto i : helpers::enumRange<Job>())
            BOOST_TEST(player.GetInventory()[i] == numPeoplePlayer[i]);
        for(const auto i : helpers::enumRange<ArmoredSoldier>())
            BOOST_TEST(player.GetInventory()[i] == numArmoredSoldiersPlayer[i]);
        for(const auto i : helpers::enumRange<GoodType>())
            BOOST_TEST(player.GetInventory()[i] == numGoodsPlayer[i]);
    }
};

using EmptyWorldFixture1P = WorldFixture<CreateEmptyWorld, 1>;

} // namespace

BOOST_FIXTURE_TEST_CASE(AddGoods, AddGoodsFixture)
{
    LogAccessor logAcc;
    GamePlayer& player = world.GetPlayer(0);
    nobBaseWarehouse& hq = *world.GetSpecObj<nobBaseWarehouse>(player.GetHQPos());
    testNumGoodsHQ();

    // Add nothing -> nothing changed
    Inventory newGoods;
    hq.AddGoods(newGoods, true);
    testNumGoodsHQ();
    testNumGoodsPlayer();

    // Add jobs
    for(const auto i : helpers::enumRange<Job>())
    {
        // Boat carrier gets divided upfront
        if(i == Job::BoatCarrier)
            continue;
        newGoods.Add(i, rttr::enum_cast(i) + 1);
        numPeople[i] += rttr::enum_cast(i) + 1;
    }
    for(const auto i : helpers::enumRange<ArmoredSoldier>())
    {
        newGoods.Add(i, rttr::enum_cast(i) + 1);
        numArmoredSoldiers[i] += rttr::enum_cast(i) + 1;
    }
    numPeoplePlayer = numPeople;
    numArmoredSoldiersPlayer = numArmoredSoldiers;
    hq.AddGoods(newGoods, true);
    testNumGoodsHQ();
    testNumGoodsPlayer();

    // Add only to hq but not to player
    for(const auto i : helpers::enumRange<Job>())
        numPeople[i] += newGoods[i];
    for(const auto i : helpers::enumRange<ArmoredSoldier>())
        numArmoredSoldiers[i] += newGoods[i];

    hq.AddGoods(newGoods, false);
    testNumGoodsHQ();
    testNumGoodsPlayer();

    // Add wares
    newGoods.clear();
    for(const auto i : helpers::enumRange<GoodType>())
    {
        // Only roman shields get added
        if(ConvertShields(i) == GoodType::ShieldRomans && i != GoodType::ShieldRomans)
            continue;
        newGoods.Add(i, rttr::enum_cast(i) + 2);
        numGoods[i] += rttr::enum_cast(i) + 2;
    }
    numGoodsPlayer = numGoods;
    hq.AddGoods(newGoods, true);
    testNumGoodsHQ();
    testNumGoodsPlayer();

    // Add only to hq but not to player
    for(const auto i : helpers::enumRange<GoodType>())
        numGoods[i] += newGoods[i];
    hq.AddGoods(newGoods, false);
    testNumGoodsHQ();
    testNumGoodsPlayer();

#if RTTR_ENABLE_ASSERTS
    newGoods.clear();
    newGoods.Add(Job::BoatCarrier);
    RTTR_REQUIRE_ASSERT(hq.AddGoods(newGoods, false));
    newGoods.clear();
    newGoods.Add(GoodType::ShieldAfricans);
    RTTR_REQUIRE_ASSERT(hq.AddGoods(newGoods, false));
#endif
}

BOOST_FIXTURE_TEST_CASE(OrderJob, EmptyWorldFixture1P)
{
    GamePlayer& player = world.GetPlayer(0);
    auto* hq = world.GetSpecObj<nobBaseWarehouse>(player.GetHQPos());
    auto* wh = static_cast<nobBaseWarehouse*>(BuildingFactory::CreateBuilding(
      world, BuildingType::Storehouse, player.GetHQPos() + MapPoint(4, 0), 0, Nation::Romans));
    world.BuildRoad(0, false, hq->GetFlagPos(), {4, Direction::East});

    // Order all existing builders
    while(hq->GetNumRealFigures(Job::Builder) > 0u)
    {
        const auto numBuilders = hq->GetNumRealFigures(Job::Builder);
        BOOST_TEST(hq->OrderJob(Job::Builder, *wh, false));
        BOOST_TEST_REQUIRE(hq->GetNumRealFigures(Job::Builder) == numBuilders - 1u);
    }
    // Ordering another one fails
    BOOST_TEST_REQUIRE(!hq->OrderJob(Job::Builder, *wh, false));
    BOOST_TEST_REQUIRE(hq->GetNumRealFigures(Job::Builder) == 0u);
    // Recruit all possible builders
    while(hq->GetNumRealWares(GoodType::Hammer) > 0u)
    {
        const auto numHammers = hq->GetNumRealWares(GoodType::Hammer);
        BOOST_TEST(hq->OrderJob(Job::Builder, *wh, true));
        BOOST_TEST_REQUIRE(hq->GetNumRealWares(GoodType::Hammer) == numHammers - 1u);
        BOOST_TEST_REQUIRE(hq->GetNumRealFigures(Job::Builder) == 0u);
    }
    // Ordering another one fails
    BOOST_TEST_REQUIRE(!hq->OrderJob(Job::Builder, *wh, true));
    BOOST_TEST(hq->GetNumRealFigures(Job::Builder) == 0u);
    BOOST_TEST(hq->GetNumRealWares(GoodType::Hammer) == 0u);
}

BOOST_FIXTURE_TEST_CASE(DestroyBuilding, EmptyWorldFixture1P)
{
    GamePlayer& player = world.GetPlayer(0);
    auto* hq = world.GetSpecObj<nobBaseWarehouse>(player.GetHQPos());

    MapPoint whPos = player.GetHQPos() + MapPoint(4, 0);

    auto* wh = static_cast<nobBaseWarehouse*>(
      BuildingFactory::CreateBuilding(world, BuildingType::Storehouse, whPos, 0, Nation::Romans));

    world.BuildRoad(0, false, hq->GetFlagPos(), {4, Direction::East});

    // Reproduce issue #1559, where a dependent figure on the same position would cause a crash
    // upon building destruction.
    auto& scout =
      world.AddFigure(whPos, std::make_unique<nofScout_Free>(whPos, 0, world.GetSpecObj<noRoadNode>(whPos)));
    wh->AddDependentFigure(scout);

    world.DestroyBuilding(whPos, 0);
}

BOOST_FIXTURE_TEST_CASE(CollectGoodsAndFigures, WorldWithGCExecution1P)
{
    GamePlayer& player = world.GetPlayer(0);

    MapPoint wh1Pos = player.GetHQPos() + MapPoint(2, 0);
    MapPoint wh2Pos = wh1Pos + MapPoint(2, 0);

    auto* wh1 = static_cast<nobBaseWarehouse*>(
      BuildingFactory::CreateBuilding(world, BuildingType::Storehouse, wh1Pos, 0, Nation::Romans));
    auto* wh2 = static_cast<nobBaseWarehouse*>(
      BuildingFactory::CreateBuilding(world, BuildingType::Storehouse, wh2Pos, 0, Nation::Romans));

    world.BuildRoad(0, false, wh1->GetFlagPos(), {2, Direction::East});
    MilitarySettings milSettings = MILITARY_SETTINGS_SCALE;
    milSettings[0] = 0; // No recruitment
    this->ChangeMilitary(milSettings);

    Inventory inv;
    for(const auto job : helpers::enumRange<Job>())
    {
        if(job == Job::BoatCarrier)
            continue;
        inv.Add(job);
    }
    for(const auto good : helpers::enumRange<GoodType>())
    {
        if(ConvertShields(good) != good)
            continue;
        inv.Add(good);
    }
    wh1->AddGoods(inv, true);

    for(const auto job : helpers::enumRange<Job>())
    {
        if(job == Job::BoatCarrier)
            continue;
        SetInventorySetting(wh2Pos, job, EInventorySetting::Collect);
    }
    for(const auto good : helpers::enumRange<GoodType>())
    {
        if(ConvertShields(good) != good)
            continue;
        SetInventorySetting(wh2Pos, good, EInventorySetting::Collect);
    }

    RTTR_EXEC_TILL(500, wh2->GetNumVisualFigures(Job::Helper) > 2);
    SetInventorySetting(wh2Pos, Job::Helper, EInventorySetting::Stop);

    RTTR_EXEC_TILL(5000, wh2->GetNumVisualWares(maxEnumValue(GoodType{})) > 0);
    RTTR_EXEC_TILL(5000, wh2->GetNumVisualFigures(maxEnumValue(Job{})) > 0);
    for(const auto job : helpers::enumRange<Job>())
    {
        if(job == Job::BoatCarrier)
            continue;
        BOOST_TEST_INFO_SCOPE("Job: " << rttr::enum_cast(job));
        if(job == Job::Helper) // Helpers get recruited continuously
        {
            BOOST_TEST(wh2->GetNumVisualFigures(job) > 10u);
            BOOST_TEST(wh2->GetNumRealFigures(job) > 10u);
        } else
        {
            BOOST_TEST(wh2->GetNumVisualFigures(job) == 1u);
            BOOST_TEST(wh2->GetNumRealFigures(job) == 1u);
        }
    }
    for(const auto good : helpers::enumRange<GoodType>())
    {
        if(ConvertShields(good) != good)
            continue;
        BOOST_TEST_INFO_SCOPE("Good: " << rttr::enum_cast(good));
        BOOST_TEST(wh2->GetNumVisualWares(good) == 1u);
        BOOST_TEST(wh2->GetNumRealWares(good) == 1u);
    }
}

BOOST_FIXTURE_TEST_CASE(AddSoldierWithArmor, EmptyWorldFixture1P)
{
    GamePlayer& player = world.GetPlayer(0);

    auto* hq = world.GetSpecObj<nobBaseWarehouse>(player.GetHQPos());
    for(unsigned i = 0; i <= this->ggs.GetMaxMilitaryRank(); ++i)
        hq->SetRealReserve(i, 0);

    auto soldierWithArmor =
      std::make_unique<nofPassiveSoldier>(player.GetHQPos(), 0, nullptr, nullptr, getSoldierRank(Job::Sergeant));
    soldierWithArmor->SetArmor(true);

    hq->AddFigure(std::move(soldierWithArmor));
    BOOST_TEST_REQUIRE(hq->GetNumRealArmoredFigures(ArmoredSoldier::Sergeant) == 1);
    BOOST_TEST_REQUIRE(hq->GetNumVisualArmoredFigures(ArmoredSoldier::Sergeant) == 1);
    BOOST_TEST_REQUIRE(hq->GetNumRealWares(GoodType::Armor) == 0);
    BOOST_TEST_REQUIRE(hq->GetNumVisualWares(GoodType::Armor) == 0);
}

BOOST_FIXTURE_TEST_CASE(CheckReserveSoldierWithArmor, EmptyWorldFixture1P)
{
    GamePlayer& player = world.GetPlayer(0);

    auto* hq = world.GetSpecObj<nobBaseWarehouse>(player.GetHQPos());

    auto soldierWithArmor =
      std::make_unique<nofPassiveSoldier>(player.GetHQPos(), 0, nullptr, nullptr, getSoldierRank(Job::Sergeant));
    soldierWithArmor->SetArmor(true);

    hq->AddFigure(std::move(soldierWithArmor));
    BOOST_TEST_REQUIRE(hq->GetNumRealArmoredFigures(ArmoredSoldier::Sergeant) == 0);
    BOOST_TEST_REQUIRE(hq->GetNumVisualArmoredFigures(ArmoredSoldier::Sergeant) == 0);
    BOOST_TEST_REQUIRE(hq->GetNumRealFigures(Job::Sergeant) == 0);
    BOOST_TEST_REQUIRE(hq->GetNumVisualFigures(Job::Sergeant) == 0);
    BOOST_TEST_REQUIRE(*hq->GetReserveAvailablePointer(getSoldierRank(Job::Sergeant)) == 1);
    BOOST_TEST_REQUIRE(*hq->GetReserveArmoredAvailablePointer(getSoldierRank(Job::Sergeant)) == 1);
    BOOST_TEST_REQUIRE(hq->GetNumRealWares(GoodType::Armor) == 0);
    BOOST_TEST_REQUIRE(hq->GetNumVisualWares(GoodType::Armor) == 0);

    // free reserve
    for(unsigned i = 0; i <= this->ggs.GetMaxMilitaryRank(); ++i)
        player.GetFirstWH()->SetRealReserve(i, 0);

    BOOST_TEST_REQUIRE(hq->GetNumRealArmoredFigures(ArmoredSoldier::Sergeant) == 1);
    BOOST_TEST_REQUIRE(hq->GetNumVisualArmoredFigures(ArmoredSoldier::Sergeant) == 1);
    BOOST_TEST_REQUIRE(hq->GetNumRealFigures(Job::Sergeant) == 1);
    BOOST_TEST_REQUIRE(hq->GetNumVisualFigures(Job::Sergeant) == 1);
    BOOST_TEST_REQUIRE(*hq->GetReserveAvailablePointer(getSoldierRank(Job::Sergeant)) == 0);
    BOOST_TEST_REQUIRE(*hq->GetReserveArmoredAvailablePointer(getSoldierRank(Job::Sergeant)) == 0);
}
