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
    std::array<unsigned, NUM_JOB_TYPES> numPeople, numPeoplePlayer;
    std::array<unsigned, NUM_WARE_TYPES> numGoods, numGoodsPlayer;
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
        for(unsigned i = 0; i < NUM_JOB_TYPES; i++)
        {
            BOOST_REQUIRE_EQUAL(hq.GetNumVisualFigures(Job(i)), numPeople[i]);
            BOOST_REQUIRE_EQUAL(hq.GetNumRealFigures(Job(i)), numPeople[i]);
        }
        for(unsigned i = 0; i < NUM_WARE_TYPES; i++)
        {
            BOOST_REQUIRE_EQUAL(hq.GetNumVisualWares(GoodType(i)), numGoods[i]);
            BOOST_REQUIRE_EQUAL(hq.GetNumRealWares(GoodType(i)), numGoods[i]);
        }
    }
    /// Asserts that the expected and actual good count match for the player
    void testNumGoodsPlayer()
    {
        GamePlayer& player = world.GetPlayer(0);
        for(unsigned i = 0; i < NUM_JOB_TYPES; i++)
            BOOST_REQUIRE_EQUAL(player.GetInventory().people[i], numPeoplePlayer[i]);
        for(unsigned i = 0; i < NUM_WARE_TYPES; i++)
            BOOST_REQUIRE_EQUAL(player.GetInventory().goods[i], numGoodsPlayer[i]);
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
    for(unsigned i = 0; i < NUM_JOB_TYPES; i++)
    {
        // Boat carrier gets divided upfront
        if(Job(i) == JOB_BOATCARRIER)
            continue;
        newGoods.Add(Job(i), i + 1);
        numPeople[i] += i + 1;
    }
    numPeoplePlayer = numPeople;
    hq.AddGoods(newGoods, true);
    testNumGoodsHQ();
    testNumGoodsPlayer();

    // Add only to hq but not to player
    for(unsigned i = 0; i < NUM_JOB_TYPES; i++)
        numPeople[i] += newGoods.people[i];
    hq.AddGoods(newGoods, false);
    testNumGoodsHQ();
    testNumGoodsPlayer();

    // Add wares
    newGoods.clear();
    for(unsigned i = 0; i < NUM_WARE_TYPES; i++)
    {
        // Only roman shields get added
        if(ConvertShields(GoodType(i)) == GD_SHIELDROMANS && GoodType(i) != GD_SHIELDROMANS)
            continue;
        newGoods.Add(GoodType(i), i + 2);
        numGoods[i] += i + 2;
    }
    numGoodsPlayer = numGoods;
    hq.AddGoods(newGoods, true);
    testNumGoodsHQ();
    testNumGoodsPlayer();

    // Add only to hq but not to player
    for(unsigned i = 0; i < NUM_WARE_TYPES; i++)
        numGoods[i] += newGoods.goods[i];
    hq.AddGoods(newGoods, false);
    testNumGoodsHQ();
    testNumGoodsPlayer();

#if RTTR_ENABLE_ASSERTS
    newGoods.clear();
    newGoods.Add(JOB_BOATCARRIER);
    RTTR_REQUIRE_ASSERT(hq.AddGoods(newGoods, false));
    newGoods.clear();
    newGoods.Add(GD_SHIELDAFRICANS);
    RTTR_REQUIRE_ASSERT(hq.AddGoods(newGoods, false));
#endif
}

BOOST_FIXTURE_TEST_CASE(OrderJob, EmptyWorldFixture1P)
{
    GamePlayer& player = world.GetPlayer(0);
    auto* hq = world.GetSpecObj<nobBaseWarehouse>(player.GetHQPos());
    auto* wh = static_cast<nobBaseWarehouse*>(
      BuildingFactory::CreateBuilding(world, BLD_STOREHOUSE, player.GetHQPos() + MapPoint(4, 0), 0, NAT_ROMANS));
    world.BuildRoad(0, false, hq->GetFlagPos(), {4, Direction::EAST});

    // Order all existing builders
    while(hq->GetNumRealFigures(JOB_BUILDER) > 0u)
    {
        const auto numBuilders = hq->GetNumRealFigures(JOB_BUILDER);
        BOOST_TEST(hq->OrderJob(JOB_BUILDER, wh, false));
        BOOST_TEST_REQUIRE(hq->GetNumRealFigures(JOB_BUILDER) == numBuilders - 1u);
    }
    // Ordering another one fails
    BOOST_TEST_REQUIRE(!hq->OrderJob(JOB_BUILDER, wh, false));
    BOOST_TEST_REQUIRE(hq->GetNumRealFigures(JOB_BUILDER) == 0u);
    // Recruit all possible builders
    while(hq->GetNumRealWares(GD_HAMMER) > 0u)
    {
        const auto numHammers = hq->GetNumRealWares(GD_HAMMER);
        BOOST_TEST(hq->OrderJob(JOB_BUILDER, wh, true));
        BOOST_TEST_REQUIRE(hq->GetNumRealWares(GD_HAMMER) == numHammers - 1u);
        BOOST_TEST_REQUIRE(hq->GetNumRealFigures(JOB_BUILDER) == 0u);
    }
    // Ordering another one fails
    BOOST_TEST_REQUIRE(!hq->OrderJob(JOB_BUILDER, wh, true));
    BOOST_TEST_REQUIRE(hq->GetNumRealFigures(JOB_BUILDER) == 0u);
    BOOST_TEST_REQUIRE(hq->GetNumRealWares(GD_HAMMER) == 0u);
}
