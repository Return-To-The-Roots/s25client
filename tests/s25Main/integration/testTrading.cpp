// Copyright (c) 2016 - 2020 Settlers Freaks (sf-team at siedler25.org)
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

#include "addons/const_addons.h"
#include "buildings/nobBaseWarehouse.h"
#include "postSystem/PostBox.h"
#include "postSystem/PostMsgWithBuilding.h"
#include "worldFixtures/WorldWithGCExecution.h"
#include "worldFixtures/initGameRNG.hpp"
#include "gameData/JobConsts.h"
#include <rttr/test/LogAccessor.hpp>
#include <boost/test/unit_test.hpp>
#include <boost/variant/variant.hpp>

BOOST_AUTO_TEST_SUITE(GameCommandSuite)

struct TradeFixture : public WorldWithGCExecution3P
{
    std::array<const GamePlayer*, 3> players;
    unsigned numHelpers, numWoodcutters, numDonkeys, numBoards, numSaws, numSwords;
    const nobBaseWarehouse* curWh;
    TradeFixture()
    {
        curPlayer = 1;
        world.GetPlayer(0).team = TM_TEAM1; //-V525
        world.GetPlayer(1).team = TM_TEAM1;
        world.GetPlayer(2).team = TM_TEAM2;
        for(unsigned i = 0; i < world.GetNumPlayers(); i++)
        {
            world.GetPlayer(i).MakeStartPacts();
            players[i] = &world.GetPlayer(i);
        }
        BOOST_REQUIRE(players[1]->IsAlly(0));
        BOOST_REQUIRE(!players[1]->IsAlly(2));
        curWh = world.GetSpecObj<nobBaseWarehouse>(players[1]->GetHQPos());
        BOOST_REQUIRE(curWh);
        // Get start count and check that we have some
        numHelpers = curWh->GetNumRealFigures(JOB_HELPER);
        numWoodcutters = curWh->GetNumRealFigures(JOB_WOODCUTTER);
        numDonkeys = curWh->GetNumRealFigures(JOB_PACKDONKEY);
        numBoards = curWh->GetNumRealWares(GD_BOARDS);
        numSaws = curWh->GetNumRealWares(GD_SAW);
        numSwords = curWh->GetNumRealWares(GD_SWORD);
        BOOST_REQUIRE_GT(numHelpers, 10u);
        BOOST_REQUIRE_GT(numDonkeys, 5u);
        BOOST_REQUIRE_GT(numBoards, 10u);
        BOOST_REQUIRE_GT(numSaws, 0u);
        BOOST_REQUIRE_GT(numWoodcutters, 2u);

        // Enable trading
        this->ggs.setSelection(AddonId::TRADE, 1);
    }

    void testExpectedWares()
    {
        BOOST_REQUIRE_EQUAL(curWh->GetNumRealFigures(JOB_HELPER), numHelpers);
        BOOST_REQUIRE_EQUAL(curWh->GetNumRealFigures(JOB_WOODCUTTER), numWoodcutters);
        BOOST_REQUIRE_EQUAL(curWh->GetNumRealFigures(JOB_PACKDONKEY), numDonkeys);
        BOOST_REQUIRE_EQUAL(curWh->GetNumRealWares(GD_BOARDS), numBoards);
        BOOST_REQUIRE_EQUAL(curWh->GetNumRealWares(GD_SAW), numSaws);
    }

    void testAfterLeaving(unsigned numTradeItems)
    {
        // Run enough GFs so all trade caravans are out (~numTradeItems + 1 people need to leave taking 30GFs max each)
        RTTR_EXEC_TILL(30 * (numTradeItems + 1), curWh->GetLeavingFigures().empty());
        // Real count should not be changed
        // But helpers can be produced in the meantime
        BOOST_REQUIRE_GE(curWh->GetNumRealFigures(JOB_HELPER), numHelpers);
        numHelpers = curWh->GetNumRealFigures(JOB_HELPER);
        testExpectedWares();
        // Visual count should match real count
        BOOST_REQUIRE_EQUAL(curWh->GetNumVisualFigures(JOB_HELPER), numHelpers);
        BOOST_REQUIRE_EQUAL(curWh->GetNumVisualFigures(JOB_WOODCUTTER), numWoodcutters);
        BOOST_REQUIRE_EQUAL(curWh->GetNumVisualFigures(JOB_PACKDONKEY), numDonkeys);
        BOOST_REQUIRE_EQUAL(curWh->GetNumVisualWares(GD_BOARDS), numBoards);
        BOOST_REQUIRE_EQUAL(curWh->GetNumVisualWares(GD_SAW), numSaws);
    }
};

BOOST_FIXTURE_TEST_CASE(TradeWares, TradeFixture)
{
    initGameRNG();

    // Disable trading
    this->ggs.setSelection(AddonId::TRADE, 0);
    this->TradeOverLand(players[0]->GetHQPos(), GD_BOARDS, 2);
    testExpectedWares();

    // Enable trading
    this->ggs.setSelection(AddonId::TRADE, 1);

    // Trade self -> Wrong
    this->TradeOverLand(players[1]->GetHQPos(), GD_BOARDS, 2); //-V525
    testExpectedWares();
    // Trade enemy -> Wrong
    this->TradeOverLand(players[2]->GetHQPos(), GD_BOARDS, 2);
    testExpectedWares();
    // Trade nothing -> Wrong
    this->TradeOverLand(players[0]->GetHQPos(), GD_BOARDS, 0);
    testExpectedWares();
    // Trade ally -> Ok
    this->TradeOverLand(players[0]->GetHQPos(), GD_BOARDS, 2);
    // Each donkey carries a ware and we need a leader
    numBoards -= 2;
    numDonkeys -= 2;
    numHelpers -= 1;
    testExpectedWares();
    testAfterLeaving(2);

    // Let caravan arrive (20GFs per node)
    unsigned distance = world.CalcDistance(curWh->GetPos(), players[0]->GetHQPos()) + 2;
    RTTR_SKIP_GFS(20 * distance);
    // Some were recruited
    numHelpers -= numSwords;
    // And some were produced (at least every 170 GFs)
    numHelpers += (20 * distance) / 170;
    curWh = world.GetSpecObj<nobBaseWarehouse>(players[0]->GetHQPos());
    BOOST_REQUIRE(curWh);
    // Expected amount is our amount + 2 times the stuff send (1 because we did not send anything, and 2 as we received
    // them)
    numBoards += 2 * 2;
    numDonkeys += 2 * 2;
    numHelpers += 2 * 1;
    // helpers can be produced in the meantime
    BOOST_REQUIRE_GE(curWh->GetNumRealFigures(JOB_HELPER), numHelpers);
    numHelpers = curWh->GetNumRealFigures(JOB_HELPER);
    testExpectedWares();
}

BOOST_FIXTURE_TEST_CASE(TradeFigures, TradeFixture)
{
    initGameRNG();

    // Disable trading
    this->ggs.setSelection(AddonId::TRADE, 0);
    this->TradeOverLand(players[0]->GetHQPos(), JOB_WOODCUTTER, 2);
    testExpectedWares();

    // Enable trading
    this->ggs.setSelection(AddonId::TRADE, 1);

    // Trade nothing -> Wrong
    this->TradeOverLand(players[0]->GetHQPos(), JOB_WOODCUTTER, 0);
    testExpectedWares();

    // For figures we don't need donkeys
    this->TradeOverLand(players[0]->GetHQPos(), JOB_WOODCUTTER, 2);
    numWoodcutters -= 2;
    numHelpers -= 1;
    testExpectedWares();
    testAfterLeaving(2);

    // Let caravan arrive (20GFs per node)
    unsigned distance = world.CalcDistance(curWh->GetPos(), players[0]->GetHQPos()) + 2;
    RTTR_SKIP_GFS(20 * distance);
    // Some were recruited
    numHelpers -= numSwords;
    // And some were produced (at least every 170 GFs)
    numHelpers += (20 * distance) / 170;
    curWh = world.GetSpecObj<nobBaseWarehouse>(players[0]->GetHQPos());
    BOOST_REQUIRE(curWh);
    // Expected amount is our amount + 2 times the stuff send (1 because we did not send anything, and 2 as we received
    // them)
    numWoodcutters += 2 * 2;
    numHelpers += 2 * 1;
    // helpers can be produced in the meantime
    BOOST_REQUIRE_GE(curWh->GetNumRealFigures(JOB_HELPER), numHelpers);
    numHelpers = curWh->GetNumRealFigures(JOB_HELPER);
    testExpectedWares();
}

BOOST_FIXTURE_TEST_CASE(TradeToMuch, TradeFixture)
{
    initGameRNG();

    // Trade more wares than available (not limited by donkeys)
    BOOST_REQUIRE_LT(numSaws, numDonkeys);
    this->TradeOverLand(players[0]->GetHQPos(), GD_SAW, numSaws * 2);
    numDonkeys -= numSaws;
    numSaws = 0;
    numHelpers -= 1;
    testExpectedWares();

    // Trade more than donkeys available -> Trade what is possible, limited by donkeys
    BOOST_REQUIRE_GT(numBoards, numDonkeys);
    this->TradeOverLand(players[0]->GetHQPos(), GD_BOARDS, numBoards);
    numBoards -= numDonkeys;
    numDonkeys = 0;
    numHelpers -= 1;
    testExpectedWares();

    // Trade to many figures
    this->TradeOverLand(players[0]->GetHQPos(), JOB_WOODCUTTER, numWoodcutters * 2);
    numWoodcutters = 0;
    numHelpers -= 1;
    testExpectedWares();
    // Recruited soldiers
    numHelpers -= numSwords;
    testAfterLeaving(20);
}

BOOST_FIXTURE_TEST_CASE(TradeFail, TradeFixture)
{
    initGameRNG();

    this->TradeOverLand(players[0]->GetHQPos(), GD_BOARDS, 2);
    // Each donkey carries a ware and we need a leader
    numBoards -= 2;
    numDonkeys -= 2;
    numHelpers -= 1;
    testExpectedWares();
    testAfterLeaving(2);

    // Make sure all of them are a bit outside
    RTTR_SKIP_GFS(40);

    // Start a trade that will fail once they leave the bld
    this->TradeOverLand(players[0]->GetHQPos(), JOB_WOODCUTTER, 2);
    numHelpers -= 1;
    numWoodcutters -= 2;
    testExpectedWares();

    // Add a ring off enemy owned land so they cannot pass
    std::vector<MapPoint> pts = world.GetPointsInRadius(curWh->GetPos(), 10);
    for(const MapPoint& pt : pts)
    {
        if(world.CalcDistance(pt, curWh->GetPos()) >= 8)
            world.SetOwner(pt, 2 + 1); // playerID = 2 -> Owner = +1
    }
    // New trade fails
    this->TradeOverLand(players[0]->GetHQPos(), GD_BOARDS, 2);
    testExpectedWares();

    // Let them come in again (walk same way back, assume at most 8 nodes away + same as above)
    RTTR_SKIP_GFS(40 + 20 * 8);
    // Recruited soldiers
    numHelpers -= numSwords;
    // Our stuff is back
    numBoards += 2;
    numDonkeys += 2;
    numHelpers += 1 + 1;
    numWoodcutters += 2;
    // helpers can be produced in the meantime
    BOOST_REQUIRE_GE(curWh->GetNumRealFigures(JOB_HELPER), numHelpers);
    numHelpers = curWh->GetNumRealFigures(JOB_HELPER);
    testExpectedWares();
}

BOOST_FIXTURE_TEST_CASE(TradeMessages, TradeFixture)
{
    initGameRNG();
    const PostBox& postbox = world.GetPostMgr().AddPostBox(0);

    this->TradeOverLand(players[0]->GetHQPos(), JOB_WOODCUTTER, 2);
    numHelpers -= 1;
    numWoodcutters -= 2;
    testAfterLeaving(2);

    unsigned distance = world.CalcDistance(curWh->GetPos(), players[0]->GetHQPos()) + 2;
    RTTR_SKIP_GFS(20 * distance);
    // warriors are recruited
    numHelpers -= numSwords;

    const PostMsg* post = postbox.GetMsg(0);
    BOOST_REQUIRE(post);
    const auto* msg = dynamic_cast<const PostMsgWithBuilding*>(post);
    BOOST_REQUIRE(msg->GetText().find('2') != std::string::npos);
    BOOST_REQUIRE(msg->GetText().find(_(JOB_NAMES[JOB_WOODCUTTER])) != std::string::npos);
    BOOST_REQUIRE(msg->GetText().find(players[1]->name) != std::string::npos);

    this->TradeOverLand(players[0]->GetHQPos(), GD_BOARDS, 2);
    numHelpers -= 1;
    numDonkeys -= 2;
    numBoards -= 2;
    testAfterLeaving(2);

    distance = world.CalcDistance(curWh->GetPos(), players[0]->GetHQPos()) + 2;
    RTTR_SKIP_GFS(20 * distance);

    const PostMsg* post2 = postbox.GetMsg(1);
    BOOST_REQUIRE(post2);
    const auto* msg2 = dynamic_cast<const PostMsgWithBuilding*>(post2);
    BOOST_REQUIRE(msg2->GetText().find('2') != std::string::npos);
    BOOST_REQUIRE(msg2->GetText().find(_(WARE_NAMES[GD_BOARDS])) != std::string::npos);
    BOOST_REQUIRE(msg2->GetText().find(players[1]->name) != std::string::npos);
}
BOOST_AUTO_TEST_SUITE_END()
