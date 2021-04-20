// Copyright (c) 2016 - 2020 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "RttrForeachPt.h"
#include "TradePathCache.h"
#include "addons/const_addons.h"
#include "buildings/nobBaseWarehouse.h"
#include "postSystem/PostBox.h"
#include "postSystem/PostMsgWithBuilding.h"
#include "worldFixtures/WorldWithGCExecution.h"
#include "worldFixtures/initGameRNG.hpp"
#include "gameData/GoodConsts.h"
#include "gameData/JobConsts.h"
#include <rttr/test/LogAccessor.hpp>
#include <boost/test/unit_test.hpp>
#include <boost/variant/variant.hpp>

struct TradeFixture : public WorldWithGCExecution3P
{
    std::array<const GamePlayer*, 3> players;
    unsigned numHelpers, numWoodcutters, numDonkeys, numBoards, numSaws, numSwords;
    const nobBaseWarehouse* curWh;
    TradeFixture()
    {
        curPlayer = 1;
        world.GetPlayer(0).team = Team::Team1; //-V525
        world.GetPlayer(1).team = Team::Team1;
        world.GetPlayer(2).team = Team::Team2;
        for(unsigned i = 0; i < world.GetNumPlayers(); i++)
        {
            world.GetPlayer(i).MakeStartPacts();
            players[i] = &world.GetPlayer(i);
        }
        BOOST_TEST_REQUIRE(players[1]->IsAlly(0));
        BOOST_TEST_REQUIRE(!players[1]->IsAlly(2));
        curWh = world.GetSpecObj<nobBaseWarehouse>(players[1]->GetHQPos());
        BOOST_TEST_REQUIRE(curWh);
        // Get start count and check that we have some
        numHelpers = curWh->GetNumRealFigures(Job::Helper);
        numWoodcutters = curWh->GetNumRealFigures(Job::Woodcutter);
        numDonkeys = curWh->GetNumRealFigures(Job::PackDonkey);
        numBoards = curWh->GetNumRealWares(GoodType::Boards);
        numSaws = curWh->GetNumRealWares(GoodType::Saw);
        numSwords = curWh->GetNumRealWares(GoodType::Sword);
        BOOST_TEST_REQUIRE(numHelpers > 10u);
        BOOST_TEST_REQUIRE(numDonkeys > 5u);
        BOOST_TEST_REQUIRE(numBoards > 10u);
        BOOST_TEST_REQUIRE(numSaws > 0u);
        BOOST_TEST_REQUIRE(numWoodcutters > 2u);

        // Enable trading
        this->ggs.setSelection(AddonId::TRADE, 1);
        world.CreateTradeGraphs();
    }

    void testExpectedWares() const
    {
        BOOST_TEST_REQUIRE(curWh->GetNumRealFigures(Job::Helper) == numHelpers);
        BOOST_TEST_REQUIRE(curWh->GetNumRealFigures(Job::Woodcutter) == numWoodcutters);
        BOOST_TEST_REQUIRE(curWh->GetNumRealFigures(Job::PackDonkey) == numDonkeys);
        BOOST_TEST_REQUIRE(curWh->GetNumRealWares(GoodType::Boards) == numBoards);
        BOOST_TEST_REQUIRE(curWh->GetNumRealWares(GoodType::Saw) == numSaws);
    }

    void testAfterLeaving(unsigned numTradeItems)
    {
        // Run enough GFs so all trade caravans are out (~numTradeItems + 1 people need to leave taking 30GFs max each)
        RTTR_EXEC_TILL(30 * (numTradeItems + 1), curWh->GetLeavingFigures().empty());
        // Real count should not be changed
        // But helpers can be produced in the meantime
        BOOST_TEST_REQUIRE(curWh->GetNumRealFigures(Job::Helper) >= numHelpers);
        numHelpers = curWh->GetNumRealFigures(Job::Helper);
        testExpectedWares();
        // Visual count should match real count
        BOOST_TEST_REQUIRE(curWh->GetNumVisualFigures(Job::Helper) == numHelpers);
        BOOST_TEST_REQUIRE(curWh->GetNumVisualFigures(Job::Woodcutter) == numWoodcutters);
        BOOST_TEST_REQUIRE(curWh->GetNumVisualFigures(Job::PackDonkey) == numDonkeys);
        BOOST_TEST_REQUIRE(curWh->GetNumVisualWares(GoodType::Boards) == numBoards);
        BOOST_TEST_REQUIRE(curWh->GetNumVisualWares(GoodType::Saw) == numSaws);
    }
};

BOOST_FIXTURE_TEST_SUITE(GameCommandSuite, TradeFixture)

BOOST_AUTO_TEST_CASE(TradeWares)
{
    initGameRNG();

    // Disable trading
    this->ggs.setSelection(AddonId::TRADE, 0);
    this->TradeOverLand(players[0]->GetHQPos(), GoodType::Boards, 2);
    testExpectedWares();

    // Enable trading
    this->ggs.setSelection(AddonId::TRADE, 1);

    // Trade self -> Wrong
    this->TradeOverLand(players[1]->GetHQPos(), GoodType::Boards, 2); //-V525
    testExpectedWares();
    // Trade enemy -> Wrong
    this->TradeOverLand(players[2]->GetHQPos(), GoodType::Boards, 2);
    testExpectedWares();
    // Trade nothing -> Wrong
    this->TradeOverLand(players[0]->GetHQPos(), GoodType::Boards, 0);
    testExpectedWares();
    // Trade ally -> Ok
    this->TradeOverLand(players[0]->GetHQPos(), GoodType::Boards, 2);
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
    BOOST_TEST_REQUIRE(curWh);
    // Expected amount is our amount + 2 times the stuff send (1 because we did not send anything, and 2 as we received
    // them)
    numBoards += 2 * 2;
    numDonkeys += 2 * 2;
    numHelpers += 2 * 1;
    // helpers can be produced in the meantime
    BOOST_TEST_REQUIRE(curWh->GetNumRealFigures(Job::Helper) >= numHelpers);
    numHelpers = curWh->GetNumRealFigures(Job::Helper);
    testExpectedWares();
}

BOOST_AUTO_TEST_CASE(TradeFigures)
{
    initGameRNG();

    // Disable trading
    this->ggs.setSelection(AddonId::TRADE, 0);
    this->TradeOverLand(players[0]->GetHQPos(), Job::Woodcutter, 2);
    testExpectedWares();

    // Enable trading
    this->ggs.setSelection(AddonId::TRADE, 1);

    // Trade nothing -> Wrong
    this->TradeOverLand(players[0]->GetHQPos(), Job::Woodcutter, 0);
    testExpectedWares();

    // For figures we don't need donkeys
    this->TradeOverLand(players[0]->GetHQPos(), Job::Woodcutter, 2);
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
    BOOST_TEST_REQUIRE(curWh);
    // Expected amount is our amount + 2 times the stuff send (1 because we did not send anything, and 2 as we received
    // them)
    numWoodcutters += 2 * 2;
    numHelpers += 2 * 1;
    // helpers can be produced in the meantime
    BOOST_TEST_REQUIRE(curWh->GetNumRealFigures(Job::Helper) >= numHelpers);
    numHelpers = curWh->GetNumRealFigures(Job::Helper);
    testExpectedWares();
}

BOOST_AUTO_TEST_CASE(TradeToMuch)
{
    initGameRNG();

    // Trade more wares than available (not limited by donkeys)
    BOOST_TEST_REQUIRE(numSaws < numDonkeys);
    this->TradeOverLand(players[0]->GetHQPos(), GoodType::Saw, numSaws * 2);
    numDonkeys -= numSaws;
    numSaws = 0;
    numHelpers -= 1;
    testExpectedWares();

    // Trade more than donkeys available -> Trade what is possible, limited by donkeys
    BOOST_TEST_REQUIRE(numBoards > numDonkeys);
    this->TradeOverLand(players[0]->GetHQPos(), GoodType::Boards, numBoards);
    numBoards -= numDonkeys;
    numDonkeys = 0;
    numHelpers -= 1;
    testExpectedWares();

    // Trade to many figures
    this->TradeOverLand(players[0]->GetHQPos(), Job::Woodcutter, numWoodcutters * 2);
    numWoodcutters = 0;
    numHelpers -= 1;
    testExpectedWares();
    // Recruited soldiers
    numHelpers -= numSwords;
    testAfterLeaving(20);
}

BOOST_AUTO_TEST_CASE(TradeFail)
{
    initGameRNG();

    this->TradeOverLand(players[0]->GetHQPos(), GoodType::Boards, 2);
    // Each donkey carries a ware and we need a leader
    numBoards -= 2;
    numDonkeys -= 2;
    numHelpers -= 1;
    testExpectedWares();
    testAfterLeaving(2);

    // Make sure all of them are a bit outside
    RTTR_SKIP_GFS(40);

    // Start a trade that will fail once they leave the bld
    this->TradeOverLand(players[0]->GetHQPos(), Job::Woodcutter, 2);
    numHelpers -= 1;
    numWoodcutters -= 2;
    testExpectedWares();

    // Add a ring of enemy owned land so they cannot pass
    std::vector<MapPoint> pts = world.GetPointsInRadius(curWh->GetPos(), 10);
    for(const MapPoint& pt : pts)
    {
        if(world.CalcDistance(pt, curWh->GetPos()) >= 8)
            world.SetOwner(pt, 2 + 1); // playerID = 2 -> Owner = +1
    }
    // New trade fails
    this->TradeOverLand(players[0]->GetHQPos(), GoodType::Boards, 2);
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
    BOOST_TEST_REQUIRE(curWh->GetNumRealFigures(Job::Helper) >= numHelpers);
    numHelpers = curWh->GetNumRealFigures(Job::Helper);
    testExpectedWares();
}

BOOST_AUTO_TEST_CASE(TradeFailDie)
{
    initGameRNG();

    this->TradeOverLand(players[0]->GetHQPos(), GoodType::Boards, 2);
    this->TradeOverLand(players[0]->GetHQPos(), Job::Woodcutter, 2);
    // Each donkey carries a ware and we need 2 leaders
    numBoards -= 2;
    numDonkeys -= 2;
    numHelpers -= 2;
    numWoodcutters -= 2;
    testExpectedWares();
    testAfterLeaving(4);

    // Let them walk a bit
    RTTR_SKIP_GFS(40);

    // Now make everything enemy territory (e.g. enemy captured the building)
    // They will then not find any further route and die
    // See https://github.com/Return-To-The-Roots/s25client/issues/1336
    RTTR_FOREACH_PT(MapPoint, world.GetSize())
        world.SetOwner(pt, 2 + 1); // playerID = 2 -> Owner = +1
    // Let them all die
    RTTR_SKIP_GFS(50);
    // Don't care about helpers but check the rest: Still gone
    numHelpers = curWh->GetNumRealFigures(Job::Helper);
    testExpectedWares();
}

BOOST_AUTO_TEST_CASE(TradeMessages)
{
    initGameRNG();
    const PostBox& postbox = world.GetPostMgr().AddPostBox(0);

    this->TradeOverLand(players[0]->GetHQPos(), Job::Woodcutter, 2);
    numHelpers -= 1;
    numWoodcutters -= 2;
    testAfterLeaving(2);

    unsigned distance = world.CalcDistance(curWh->GetPos(), players[0]->GetHQPos()) + 2;
    RTTR_SKIP_GFS(20 * distance);
    // warriors are recruited
    numHelpers -= numSwords;

    const PostMsg* post = postbox.GetMsg(0);
    BOOST_TEST_REQUIRE(post);
    const auto* msg = dynamic_cast<const PostMsgWithBuilding*>(post);
    BOOST_TEST_REQUIRE(msg->GetText().find('2') != std::string::npos);
    BOOST_TEST_REQUIRE(msg->GetText().find(_(JOB_NAMES[Job::Woodcutter])) != std::string::npos);
    BOOST_TEST_REQUIRE(msg->GetText().find(players[1]->name) != std::string::npos);

    this->TradeOverLand(players[0]->GetHQPos(), GoodType::Boards, 2);
    numHelpers -= 1;
    numDonkeys -= 2;
    numBoards -= 2;
    testAfterLeaving(2);

    distance = world.CalcDistance(curWh->GetPos(), players[0]->GetHQPos()) + 2;
    RTTR_SKIP_GFS(20 * distance);

    const PostMsg* post2 = postbox.GetMsg(1);
    BOOST_TEST_REQUIRE(post2);
    const auto* msg2 = dynamic_cast<const PostMsgWithBuilding*>(post2);
    BOOST_TEST_REQUIRE(msg2->GetText().find('2') != std::string::npos);
    BOOST_TEST_REQUIRE(msg2->GetText().find(_(WARE_NAMES[GoodType::Boards])) != std::string::npos);
    BOOST_TEST_REQUIRE(msg2->GetText().find(players[1]->name) != std::string::npos);
}

BOOST_AUTO_TEST_CASE(GetWarehousesForTrading)
{
    std::array<const nobBaseWarehouse*, 3> hqs{};
    for(unsigned i = 0; i < players.size(); i++)
        hqs[i] = world.GetSpecObj<nobBaseWarehouse>(players[i]->GetHQPos());

    for(int i = 0; i < 2; i++)
    { // Run the same test twice
        for(unsigned iStart = 0; iStart < players.size(); iStart++)
        {
            for(unsigned iGoal = 0; iGoal < players.size(); iGoal++)
            {
                const std::vector<nobBaseWarehouse*> possibleWhs =
                  players[iStart]->GetWarehousesForTrading(*hqs[iGoal]);
                if(iStart == iGoal) // Trade to self is not possible
                    BOOST_TEST(possibleWhs.empty());
                else if(iStart == 2 || iGoal == 2) // Player 2 has no ally -> no trade
                    BOOST_TEST(possibleWhs.empty());
                else
                { // Other player --> Only wh is the HQ
                    BOOST_TEST_REQUIRE(possibleWhs.size() == 1u);
                    BOOST_TEST(possibleWhs[0] == hqs[iStart]);
                }
            }
        }
        if(i == 0)
        {
            // Poison the tradepath cache with invalid entries. This is possible if e.g. a route is no longer possible
            // or a wh was destroyed
            TradePathCache& cache = world.GetTradePathCache();
            const unsigned oldCacheSize = cache.size();
            // The above should have added 1 entry for the connection of player 1 and 2 whs
            BOOST_TEST(oldCacheSize == 1u);
            cache.addEntry(
              TradePath(MapPoint(2, 2), MapPoint(5, 1), {Direction::East, Direction::East, Direction::NorthEast}), 0);
            BOOST_TEST(cache.size() == oldCacheSize + 1u);
            // Adding the reverse of an allied player does not add a new entry
            cache.addEntry(
              TradePath(MapPoint(5, 1), MapPoint(2, 2), {Direction::West, Direction::West, Direction::SouthWest}), 1);
            BOOST_TEST(cache.size() == oldCacheSize + 1u);
            // Adding the same route for an enemy player does add a new entry
            cache.addEntry(
              TradePath(MapPoint(2, 2), MapPoint(5, 1), {Direction::East, Direction::East, Direction::NorthEast}), 2);
            BOOST_TEST(cache.size() == oldCacheSize + 2u);

            // Add a few more until the cache is full
            cache.addEntry(TradePath(MapPoint(2, 2), MapPoint(3, 2), std::vector<Direction>(1, Direction::East)), 0);
            cache.addEntry(TradePath(MapPoint(2, 2), MapPoint(4, 2), std::vector<Direction>(2, Direction::East)), 0);
            cache.addEntry(TradePath(MapPoint(2, 2), MapPoint(5, 2), std::vector<Direction>(3, Direction::East)), 0);
            cache.addEntry(TradePath(MapPoint(2, 2), MapPoint(6, 2), std::vector<Direction>(4, Direction::East)), 0);
            cache.addEntry(TradePath(MapPoint(2, 2), MapPoint(7, 2), std::vector<Direction>(5, Direction::East)), 0);
            cache.addEntry(TradePath(MapPoint(2, 2), MapPoint(8, 2), std::vector<Direction>(6, Direction::East)), 0);
            cache.addEntry(TradePath(MapPoint(2, 2), MapPoint(9, 2), std::vector<Direction>(7, Direction::East)), 0);
            cache.addEntry(TradePath(MapPoint(2, 2), MapPoint(10, 2), std::vector<Direction>(8, Direction::East)), 0);
            BOOST_TEST(cache.size() == 10u);
            // Cache is full so this replaces the oldest entry
            cache.addEntry(TradePath(MapPoint(2, 2), MapPoint(11, 2), std::vector<Direction>(9, Direction::East)), 0);
            BOOST_TEST(cache.size() == 10u);
        }
    }
}

BOOST_AUTO_TEST_SUITE_END()
