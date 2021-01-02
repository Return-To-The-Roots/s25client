// Copyright (c) 2016 - 2018 Settlers Freaks (sf-team at siedler25.org)
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

#include "GamePlayer.h"
#include "RTTR_AssertError.h"
#include "buildings/nobBaseWarehouse.h"
#include "factories/BuildingFactory.h"
#include "worldFixtures/CreateEmptyWorld.h"
#include "worldFixtures/WorldFixture.h"
#include "gameTypes/GoodTypes.h"
#include "gameTypes/JobTypes.h"
#include "gameData/ShieldConsts.h"
#include <rttr/test/LogAccessor.hpp>
#include <boost/test/unit_test.hpp>
#include <array>

namespace {
struct AddGoodsFixture : public WorldFixture<CreateEmptyWorld, 1>, public rttr::test::LogAccessor
{
    helpers::EnumArray<unsigned, Job> numPeople, numPeoplePlayer;
    helpers::EnumArray<unsigned, GoodType> numGoods, numGoodsPlayer;
    AddGoodsFixture()
    {
        GamePlayer& player = world.GetPlayer(0);
        // Don't keep any reserve
        for(unsigned i = 0; i <= this->ggs.GetMaxMilitaryRank(); ++i)
            player.GetFirstWH()->SetRealReserve(i, 0); //-V522
        numPeople = numPeoplePlayer = player.GetInventory().people;
        numGoods = numGoodsPlayer = player.GetInventory().goods;
    }

    /// Asserts that the expected and actual good count match for the HQ
    void testNumGoodsHQ()
    {
        nobBaseWarehouse& hq = *world.GetSpecObj<nobBaseWarehouse>(world.GetPlayer(0).GetHQPos());
        for(const auto i : helpers::enumRange<Job>())
        {
            BOOST_REQUIRE_EQUAL(hq.GetNumVisualFigures(i), numPeople[i]);
            BOOST_REQUIRE_EQUAL(hq.GetNumRealFigures(i), numPeople[i]);
        }
        for(const auto i : helpers::enumRange<GoodType>())
        {
            BOOST_REQUIRE_EQUAL(hq.GetNumVisualWares(i), numGoods[i]);
            BOOST_REQUIRE_EQUAL(hq.GetNumRealWares(i), numGoods[i]);
        }
    }
    /// Asserts that the expected and actual good count match for the player
    void testNumGoodsPlayer()
    {
        GamePlayer& player = world.GetPlayer(0);
        for(const auto i : helpers::enumRange<Job>())
            BOOST_REQUIRE_EQUAL(player.GetInventory()[i], numPeoplePlayer[i]);
        for(const auto i : helpers::enumRange<GoodType>())
            BOOST_REQUIRE_EQUAL(player.GetInventory()[i], numGoodsPlayer[i]);
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
    numPeoplePlayer = numPeople;
    hq.AddGoods(newGoods, true);
    testNumGoodsHQ();
    testNumGoodsPlayer();

    // Add only to hq but not to player
    for(const auto i : helpers::enumRange<Job>())
        numPeople[i] += newGoods[i];
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
    world.BuildRoad(0, false, hq->GetFlagPos(), {4, Direction::EAST});

    // Order all existing builders
    while(hq->GetNumRealFigures(Job::Builder) > 0u)
    {
        const auto numBuilders = hq->GetNumRealFigures(Job::Builder);
        BOOST_TEST(hq->OrderJob(Job::Builder, wh, false));
        BOOST_TEST_REQUIRE(hq->GetNumRealFigures(Job::Builder) == numBuilders - 1u);
    }
    // Ordering another one fails
    BOOST_TEST_REQUIRE(!hq->OrderJob(Job::Builder, wh, false));
    BOOST_TEST_REQUIRE(hq->GetNumRealFigures(Job::Builder) == 0u);
    // Recruit all possible builders
    while(hq->GetNumRealWares(GoodType::Hammer) > 0u)
    {
        const auto numHammers = hq->GetNumRealWares(GoodType::Hammer);
        BOOST_TEST(hq->OrderJob(Job::Builder, wh, true));
        BOOST_TEST_REQUIRE(hq->GetNumRealWares(GoodType::Hammer) == numHammers - 1u);
        BOOST_TEST_REQUIRE(hq->GetNumRealFigures(Job::Builder) == 0u);
    }
    // Ordering another one fails
    BOOST_TEST_REQUIRE(!hq->OrderJob(Job::Builder, wh, true));
    BOOST_TEST_REQUIRE(hq->GetNumRealFigures(Job::Builder) == 0u);
    BOOST_TEST_REQUIRE(hq->GetNumRealWares(GoodType::Hammer) == 0u);
}
