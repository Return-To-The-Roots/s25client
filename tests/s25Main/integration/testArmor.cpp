// Copyright (C) 2005 - 2025 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "RttrForeachPt.h"
#include "TradePathCache.h"
#include "addons/const_addons.h"
#include "buildings/nobBaseWarehouse.h"
#include "worldFixtures/WorldWithGCExecution.h"
#include "worldFixtures/initGameRNG.hpp"
#include <boost/test/unit_test.hpp>

struct ArmoredSoldierFixture : public WorldWithGCExecution3P
{
    ArmoredSoldierFixture()
    {
        for(auto i = 0; i < 3; i++)
        {
            auto* curWh = world.GetSpecObj<nobBaseWarehouse>(world.GetPlayer(i).GetHQPos());
            BOOST_TEST_REQUIRE(curWh);
            Inventory newGoods;
            for(auto soldier : SOLDIER_JOBS)
            {
                newGoods.armoredSoldiers[jobEnumToAmoredSoldierEnum(soldier)] = 2u;
                newGoods.people[soldier] = 3u;
                curWh->SetRealReserve(getSoldierRank(soldier), 0);
                curWh->SetReserveVisual(getSoldierRank(soldier), 0);
            }
            curWh->AddGoods(newGoods, true);
        }
    }
};

struct ArmorTradeFixture : public ArmoredSoldierFixture
{
    std::array<const GamePlayer*, 3> players;
    unsigned numHelpers, numDonkeys = {};
    std::array<unsigned, NUM_SOLDIER_RANKS> numArmoredSoldiers;
    std::array<unsigned, NUM_SOLDIER_RANKS> numSoldiers;

    const nobBaseWarehouse* curWh;
    ArmorTradeFixture()
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
        RTTR_EXEC_TILL(200 * curWh->GetNumRealWares(GoodType::Sword), curWh->GetNumRealWares(GoodType::Sword) == 0);
        // Get start count and check that we have some
        numHelpers = curWh->GetNumRealFigures(Job::Helper);
        numDonkeys = curWh->GetNumRealFigures(Job::PackDonkey);

        for(unsigned i = 0; i < NUM_SOLDIER_RANKS; i++)
        {
            numSoldiers[i] = curWh->GetNumRealFigures(SOLDIER_JOBS[i]);
            numArmoredSoldiers[i] = curWh->GetNumRealArmoredFigures(jobEnumToAmoredSoldierEnum(SOLDIER_JOBS[i]));
        }

        BOOST_TEST_REQUIRE(numHelpers > 10u);
        BOOST_TEST_REQUIRE(numDonkeys > 5u);
        BOOST_TEST_REQUIRE(numArmoredSoldiers[0] == 2u);
        BOOST_TEST_REQUIRE(numSoldiers[0] == 55u);
        for(unsigned i = 1; i < NUM_SOLDIER_RANKS; i++)
        {
            BOOST_TEST_REQUIRE(numArmoredSoldiers[i] == 2u);
            BOOST_TEST_REQUIRE(numSoldiers[i] == 3u);
        }

        // Enable trading
        this->ggs.setSelection(AddonId::TRADE, 1);
        world.CreateTradeGraphs();
    }

    void testExpectedFiguresInGlobalInventoryMatchWithHQInventory() const
    {
        for(unsigned i = 0; i < world.GetNumPlayers(); i++)
        {
            auto const& playerWh = world.GetSpecObj<nobBaseWarehouse>(players[i]->GetHQPos());
            auto const& globalInventoryPlayer = world.GetPlayer(i).GetInventory();
            for(unsigned i = 0; i < NUM_SOLDIER_RANKS; i++)
            {
                BOOST_TEST_REQUIRE(playerWh->GetNumRealArmoredFigures(jobEnumToAmoredSoldierEnum(SOLDIER_JOBS[i]))
                                   == globalInventoryPlayer[jobEnumToAmoredSoldierEnum(SOLDIER_JOBS[i])]);
                BOOST_TEST_REQUIRE(playerWh->GetNumRealFigures(SOLDIER_JOBS[i])
                                   == globalInventoryPlayer[SOLDIER_JOBS[i]]);
            }
        }
    }

    void testExpectedFigures() const
    {
        BOOST_TEST_REQUIRE(curWh->GetNumRealFigures(Job::Helper) == numHelpers);
        BOOST_TEST_REQUIRE(curWh->GetNumRealFigures(Job::PackDonkey) == numDonkeys);

        for(unsigned i = 0; i < NUM_SOLDIER_RANKS; i++)
        {
            BOOST_TEST_REQUIRE(curWh->GetNumRealArmoredFigures(jobEnumToAmoredSoldierEnum(SOLDIER_JOBS[i]))
                               == numArmoredSoldiers[i]);
            BOOST_TEST_REQUIRE(curWh->GetNumRealFigures(SOLDIER_JOBS[i]) == numSoldiers[i]);
        }
        testExpectedFiguresInGlobalInventoryMatchWithHQInventory();
    }

    void testAfterLeaving(unsigned numTradeItems)
    {
        // Run enough GFs so all trade caravans are out (~numTradeItems + 1 people need to leave taking 30GFs max each)
        RTTR_EXEC_TILL(30 * (numTradeItems + 1), curWh->GetLeavingFigures().empty());
        // Real count should not be changed
        // But helpers can be produced in the meantime
        BOOST_TEST_REQUIRE(curWh->GetNumRealFigures(Job::Helper) >= numHelpers);
        numHelpers = curWh->GetNumRealFigures(Job::Helper);
        testExpectedFigures();
        // Visual count should match real count
        BOOST_TEST_REQUIRE(curWh->GetNumVisualFigures(Job::Helper) == numHelpers);
        BOOST_TEST_REQUIRE(curWh->GetNumVisualFigures(Job::PackDonkey) == numDonkeys);
        for(unsigned i = 0; i < NUM_SOLDIER_RANKS; i++)
        {
            BOOST_TEST_REQUIRE(curWh->GetNumVisualArmoredFigures(jobEnumToAmoredSoldierEnum(SOLDIER_JOBS[i]))
                               == numArmoredSoldiers[i]);
            BOOST_TEST_REQUIRE(curWh->GetNumVisualFigures(SOLDIER_JOBS[i]) == numSoldiers[i]);
        }
    }
};

BOOST_FIXTURE_TEST_SUITE(GameCommandSuite, ArmorTradeFixture)

BOOST_AUTO_TEST_CASE(TradeArmoredFigures)
{
    initGameRNG();

    // Disable trading
    this->ggs.setSelection(AddonId::TRADE, 0);
    this->TradeOverLand(players[0]->GetHQPos(), Job::Officer, 3);
    testExpectedFigures();

    // Enable trading
    this->ggs.setSelection(AddonId::TRADE, 1);

    // Trade nothing -> Wrong
    this->TradeOverLand(players[0]->GetHQPos(), Job::Officer, 0);
    testExpectedFigures();

    const unsigned officerSoldiersTraded = 3;
    const unsigned officerSoldiersWithArmor = 2;

    // For figures we don't need donkeys
    BOOST_TEST_REQUIRE(numArmoredSoldiers[getSoldierRank(Job::Officer)] == 2u);
    BOOST_TEST_REQUIRE(numSoldiers[getSoldierRank(Job::Officer)] == 3u);

    this->TradeOverLand(players[0]->GetHQPos(), Job::Officer, 3);
    numArmoredSoldiers[getSoldierRank(Job::Officer)] -= officerSoldiersWithArmor;
    numSoldiers[getSoldierRank(Job::Officer)] -= officerSoldiersTraded;
    numHelpers -= 1; // Trade leader
    testExpectedFigures();
    testAfterLeaving(3);

    // Let caravan arrive (20GFs per node)
    unsigned distance = world.CalcDistance(curWh->GetPos(), players[0]->GetHQPos()) + 2;
    RTTR_SKIP_GFS(20 * distance);

    // Some were produced (at least every 170 GFs)
    numHelpers += (20 * distance) / 170;
    curWh = world.GetSpecObj<nobBaseWarehouse>(players[0]->GetHQPos());
    BOOST_TEST_REQUIRE(curWh);
    // Expected amount is our amount + 2 times the stuff send (1 because we did not send anything, and 2 as we received
    // them)
    numSoldiers[getSoldierRank(Job::Officer)] += 2 * officerSoldiersTraded;
    numArmoredSoldiers[getSoldierRank(Job::Officer)] += 2 * officerSoldiersWithArmor;
    numHelpers += 2 * 1; // Trade leader
    // helpers can be produced in the meantime
    BOOST_TEST_REQUIRE(curWh->GetNumRealFigures(Job::Helper) >= numHelpers);
    numHelpers = curWh->GetNumRealFigures(Job::Helper);
    testExpectedFigures();
}

BOOST_AUTO_TEST_CASE(ArmorTradeFail)
{
    initGameRNG();

    const unsigned officerSoldiersTraded = 3;
    const unsigned officerSoldiersWithArmor = 2;

    this->TradeOverLand(players[0]->GetHQPos(), Job::Officer, 3);
    // For figures we don't need donkeys
    numArmoredSoldiers[getSoldierRank(Job::Officer)] -= officerSoldiersWithArmor;
    numSoldiers[getSoldierRank(Job::Officer)] -= officerSoldiersTraded;
    numHelpers -= 1;
    testExpectedFigures();
    testAfterLeaving(3);

    // Make sure all of them are a bit outside
    RTTR_SKIP_GFS(40);

    // Add a ring of enemy owned land so they cannot pass
    std::vector<MapPoint> pts = world.GetPointsInRadius(curWh->GetPos(), 10);
    for(const MapPoint& pt : pts)
    {
        if(world.CalcDistance(pt, curWh->GetPos()) >= 8)
            world.SetOwner(pt, 2 + 1); // playerID = 2 -> Owner = +1
    }

    // Let them come in again (walk same way back, assume at most 8 nodes away + same as above)
    RTTR_SKIP_GFS(40 + 20 * 8);
    // Our stuff is back
    numArmoredSoldiers[getSoldierRank(Job::Officer)] += officerSoldiersWithArmor;
    numSoldiers[getSoldierRank(Job::Officer)] += officerSoldiersTraded;
    numHelpers += 1;
    // helpers can be produced in the meantime
    BOOST_TEST_REQUIRE(curWh->GetNumRealFigures(Job::Helper) >= numHelpers);
    numHelpers = curWh->GetNumRealFigures(Job::Helper);
    testExpectedFigures();
}

BOOST_AUTO_TEST_CASE(ArmorTradeFailDie)
{
    initGameRNG();

    const unsigned officerSoldiersTraded = 3;
    const unsigned officerSoldiersWithArmor = 2;

    this->TradeOverLand(players[0]->GetHQPos(), Job::Officer, 3);
    // Each donkey carries a ware and we need 2 leaders
    numArmoredSoldiers[getSoldierRank(Job::Officer)] -= officerSoldiersWithArmor;
    numSoldiers[getSoldierRank(Job::Officer)] -= officerSoldiersTraded;
    numHelpers -= 1;
    testExpectedFigures();
    testAfterLeaving(3);

    // Let them walk a bit
    RTTR_SKIP_GFS(40);

    // Now make everything enemy territory (e.g. enemy captured the building)
    // They will then not find any further route and die
    // See https://github.com/Return-To-The-Roots/s25client/issues/1336
    RTTR_FOREACH_PT(MapPoint, world.GetSize())
        world.SetOwner(pt, 2 + 1); // playerID = 2 -> Owner = +1
    // Let them all die
    RTTR_SKIP_GFS(40 + 20 * 8);
    // Don't care about helpers but check the rest: Still gone
    numHelpers = curWh->GetNumRealFigures(Job::Helper);
    testExpectedFigures();
}

BOOST_AUTO_TEST_SUITE_END()
