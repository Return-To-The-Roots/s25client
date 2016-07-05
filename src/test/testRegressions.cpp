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
#include "buildings/nobBaseWarehouse.h"
#include "GamePlayer.h"
#include "gameData/ShieldConsts.h"
#include "RTTR_AssertError.h"
#include "test/WorldFixture.h"
#include "test/CreateEmptyWorld.h"
#include <boost/test/unit_test.hpp>
#include <boost/array.hpp>

// This suite tests bugs that got fixed to avoid regressions
// So:
//      - Before you fix a bug, reproduce it here (or in a fitting suite) -> Test fails
//      - Fix the bug -> Test succeeds
BOOST_AUTO_TEST_SUITE(RegressionsSuite)

struct AddGoodsFixture: public WorldFixture<CreateEmptyWorld, 1>
{
    boost::array<unsigned, JOB_TYPES_COUNT> numPeople, numPeoplePlayer;
    boost::array<unsigned, WARE_TYPES_COUNT> numGoods, numGoodsPlayer;
    AddGoodsFixture()
    {
        GamePlayer& player = world.GetPlayer(0);
        numPeople = numPeoplePlayer = player.GetInventory().people;
        numGoods = numGoodsPlayer = player.GetInventory().goods;
    }

    /// Asserts that the expected and actual good count match for the HQ
    void testGoodsCountHQ()
    {
        nobBaseWarehouse& hq = *world.GetSpecObj<nobBaseWarehouse>(world.GetPlayer(0).GetHQPos());
        for(unsigned i = 0; i < JOB_TYPES_COUNT; i++)
        {
            BOOST_REQUIRE_EQUAL(hq.GetVisualFiguresCount(Job(i)), numPeople[i]);
            BOOST_REQUIRE_EQUAL(hq.GetRealFiguresCount(Job(i)), numPeople[i]);
        }
        for(unsigned i = 0; i < WARE_TYPES_COUNT; i++)
        {
            BOOST_REQUIRE_EQUAL(hq.GetVisualWaresCount(GoodType(i)), numGoods[i]);
            BOOST_REQUIRE_EQUAL(hq.GetRealWaresCount(GoodType(i)), numGoods[i]);
        }
    }
    /// Asserts that the expected and actual good count match for the player
    void testGoodsCountPlayer()
    {
        GamePlayer& player = world.GetPlayer(0);
        for(unsigned i = 0; i < JOB_TYPES_COUNT; i++)
            BOOST_REQUIRE_EQUAL(player.GetInventory().people[i], numPeoplePlayer[i]);
        for(unsigned i = 0; i < WARE_TYPES_COUNT; i++)
            BOOST_REQUIRE_EQUAL(player.GetInventory().goods[i], numGoodsPlayer[i]);
    }
};

BOOST_FIXTURE_TEST_CASE(AddGoods, AddGoodsFixture)
{
    GamePlayer& player = world.GetPlayer(0);
    nobBaseWarehouse& hq = *world.GetSpecObj<nobBaseWarehouse>(player.GetHQPos());
    testGoodsCountHQ();

    // Add nothing -> nothing changed
    Inventory newGoods;
    hq.AddGoods(newGoods, true);
    testGoodsCountHQ();
    testGoodsCountPlayer();

    // Add jobs
    for(unsigned i = 0; i < JOB_TYPES_COUNT; i++)
    {
        // Boat carrier gets divided upfront
        if(Job(i) == JOB_BOATCARRIER)
            continue;
        newGoods.Add(Job(i), i + 1);
        numPeople[i] += i + 1;
    }
    numPeoplePlayer = numPeople;
    hq.AddGoods(newGoods, true);
    testGoodsCountHQ();
    testGoodsCountPlayer();

    // Add only to hq but not to player
    for(unsigned i = 0; i < JOB_TYPES_COUNT; i++)
        numPeople[i] += newGoods.people[i];
    hq.AddGoods(newGoods, false);
    testGoodsCountHQ();
    testGoodsCountPlayer();

    // Add wares
    newGoods.clear();
    for(unsigned i = 0; i < WARE_TYPES_COUNT; i++)
    {
        // Only romand shields get added
        if(ConvertShields(GoodType(i)) == GD_SHIELDROMANS && GoodType(i) != GD_SHIELDROMANS)
            continue;
        newGoods.Add(GoodType(i), i + 2);
        numGoods[i] += i + 2;
    }
    numGoodsPlayer = numGoods;
    hq.AddGoods(newGoods, true);
    testGoodsCountHQ();
    testGoodsCountPlayer();

    // Add only to hq but not to player
    for(unsigned i = 0; i < WARE_TYPES_COUNT; i++)
        numGoods[i] += newGoods.goods[i];
    hq.AddGoods(newGoods, false);
    testGoodsCountHQ();
    testGoodsCountPlayer();

#if RTTR_ENABLE_ASSERTS
    RTTR_AssertEnableBreak = false;
    newGoods.clear();
    newGoods.Add(JOB_BOATCARRIER);
    BOOST_CHECK_THROW(hq.AddGoods(newGoods, false), RTTR_AssertError);
    newGoods.clear();
    newGoods.Add(GD_SHIELDAFRICANS);
    BOOST_CHECK_THROW(hq.AddGoods(newGoods, false), RTTR_AssertError);
    RTTR_AssertEnableBreak = true;
#endif
}

BOOST_AUTO_TEST_SUITE_END()
