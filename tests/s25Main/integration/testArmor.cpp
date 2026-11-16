// Copyright (C) 2005 - 2026 Settlers Freaks (sf-team at siedler25.org)
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
            auto* curWh = world.getSpecObj<nobBaseWarehouse>(world.getPlayer(i).getHQPos());
            BOOST_TEST_REQUIRE(curWh);
            PeopleCounts soldiers;
            for(auto soldier : SOLDIER_JOBS)
            {
                soldiers[jobEnumToAmoredSoldierEnum(soldier)] = 2u;
                soldiers[soldier] = 3u;
                curWh->setRealReserve(getSoldierRank(soldier), 0);
                curWh->setReserveVisual(getSoldierRank(soldier), 0);
            }
            curWh->AddToInventory(soldiers, true);
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
        addStartResources();

        world.getPlayer(0).team = Team::Team1; //-V525
        world.getPlayer(1).team = Team::Team1;
        world.getPlayer(2).team = Team::Team2;
        for(unsigned i = 0; i < world.getNumPlayers(); i++)
        {
            world.getPlayer(i).MakeStartPacts();
            players[i] = &world.getPlayer(i);
        }
        BOOST_TEST_REQUIRE(players[1]->isAlly(0));
        BOOST_TEST_REQUIRE(!players[1]->isAlly(2));
        curWh = world.getSpecObj<nobBaseWarehouse>(players[1]->getHQPos());
        BOOST_TEST_REQUIRE(curWh);
        RTTR_EXEC_TILL(200 * curWh->getNumRealWares(GoodType::Sword), curWh->getNumRealWares(GoodType::Sword) == 0);
        // Get start count and check that we have some
        numHelpers = curWh->getNumRealFigures(Job::Helper);
        numDonkeys = curWh->getNumRealFigures(Job::PackDonkey);

        for(unsigned i = 0; i < NUM_SOLDIER_RANKS; i++)
        {
            numSoldiers[i] = curWh->getNumRealFigures(SOLDIER_JOBS[i]);
            numArmoredSoldiers[i] = curWh->getNumRealArmoredFigures(jobEnumToAmoredSoldierEnum(SOLDIER_JOBS[i]));
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
        initGameRNG();
    }

    void testExpectedFiguresInGlobalInventoryMatchWithHQInventory() const
    {
        for(const auto& player : world.getPlayers())
        {
            auto const& playerWh = world.getSpecObj<nobBaseWarehouse>(player.getHQPos());
            auto const& globalInventoryPlayer = player.getInventory();
            for(unsigned i = 0; i < NUM_SOLDIER_RANKS; i++)
            {
                BOOST_TEST(playerWh->getNumRealArmoredFigures(jobEnumToAmoredSoldierEnum(SOLDIER_JOBS[i]))
                           == globalInventoryPlayer[jobEnumToAmoredSoldierEnum(SOLDIER_JOBS[i])]);
                BOOST_TEST(playerWh->getNumRealFigures(SOLDIER_JOBS[i]) == globalInventoryPlayer[SOLDIER_JOBS[i]]);
            }
        }
    }

    void testExpectedFigures() const
    {
        BOOST_TEST(curWh->getNumRealFigures(Job::Helper) == numHelpers);
        BOOST_TEST(curWh->getNumRealFigures(Job::PackDonkey) == numDonkeys);

        for(unsigned i = 0; i < NUM_SOLDIER_RANKS; i++)
        {
            BOOST_TEST(curWh->getNumRealArmoredFigures(jobEnumToAmoredSoldierEnum(SOLDIER_JOBS[i]))
                       == numArmoredSoldiers[i]);
            BOOST_TEST(curWh->getNumRealFigures(SOLDIER_JOBS[i]) == numSoldiers[i]);
        }
        testExpectedFiguresInGlobalInventoryMatchWithHQInventory();
    }

    void testAfterLeaving(unsigned numTradeItems)
    {
        // Run enough GFs so all trade caravans are out (~numTradeItems + 1 people need to leave taking 30GFs max each)
        RTTR_EXEC_TILL(30 * (numTradeItems + 1), curWh->getLeavingFigures().empty());
        // Real count should not be changed
        // But helpers can be produced in the meantime
        BOOST_TEST(curWh->getNumRealFigures(Job::Helper) >= numHelpers);
        numHelpers = curWh->getNumRealFigures(Job::Helper);
        testExpectedFigures();
        // Visual count should match real count
        BOOST_TEST(curWh->getNumVisualFigures(Job::Helper) == numHelpers);
        BOOST_TEST(curWh->getNumVisualFigures(Job::PackDonkey) == numDonkeys);
        for(unsigned i = 0; i < NUM_SOLDIER_RANKS; i++)
        {
            BOOST_TEST(curWh->getNumVisualArmoredFigures(jobEnumToAmoredSoldierEnum(SOLDIER_JOBS[i]))
                       == numArmoredSoldiers[i]);
            BOOST_TEST(curWh->getNumVisualFigures(SOLDIER_JOBS[i]) == numSoldiers[i]);
        }
    }
};

BOOST_FIXTURE_TEST_SUITE(GameCommandSuite, ArmorTradeFixture)

BOOST_AUTO_TEST_CASE(TradeArmoredFigures)
{
    // Disable trading
    this->ggs.setSelection(AddonId::TRADE, 0);
    this->TradeOverLand(players[0]->getHQPos(), Job::Officer, 3);
    testExpectedFigures();

    // Enable trading
    this->ggs.setSelection(AddonId::TRADE, 1);

    // Trade nothing -> Wrong
    this->TradeOverLand(players[0]->getHQPos(), Job::Officer, 0);
    testExpectedFigures();

    const unsigned officerSoldiersTraded = 3;
    const unsigned officerSoldiersWithArmor = 2;
    const unsigned initialNumSoldiers = numSoldiers[getSoldierRank(Job::Officer)];
    const unsigned initialNumArmoredSoldiers = numArmoredSoldiers[getSoldierRank(Job::Officer)];
    const unsigned initialNumHelpers = numHelpers;

    // For figures we don't need donkeys
    BOOST_TEST_REQUIRE(numArmoredSoldiers[getSoldierRank(Job::Officer)] == officerSoldiersWithArmor);
    BOOST_TEST_REQUIRE(numSoldiers[getSoldierRank(Job::Officer)] == officerSoldiersTraded);

    this->TradeOverLand(players[0]->getHQPos(), Job::Officer, officerSoldiersTraded);
    numArmoredSoldiers[getSoldierRank(Job::Officer)] -= officerSoldiersWithArmor;
    numSoldiers[getSoldierRank(Job::Officer)] -= officerSoldiersTraded;
    numHelpers -= 1; // Trade leader
    testExpectedFigures();
    testAfterLeaving(3);

    // Let caravan arrive (20GFs per node)
    unsigned distance = world.CalcDistance(curWh->getPos(), players[0]->getHQPos()) + 2;
    RTTR_SKIP_GFS(20 * distance);

    curWh = world.getSpecObj<nobBaseWarehouse>(players[0]->getHQPos());
    BOOST_TEST_REQUIRE(curWh);
    numSoldiers[getSoldierRank(Job::Officer)] = initialNumSoldiers + officerSoldiersTraded;
    numArmoredSoldiers[getSoldierRank(Job::Officer)] = initialNumArmoredSoldiers + officerSoldiersWithArmor;
    numHelpers = initialNumHelpers + 1    // Trade leader
                 + (20 * distance) / 170; // Some were produced(at least every 170 GFs)
    // helpers can be produced in the meantime
    BOOST_TEST_REQUIRE(curWh->getNumRealFigures(Job::Helper) >= numHelpers);
    numHelpers = curWh->getNumRealFigures(Job::Helper);
    testExpectedFigures();
}

BOOST_AUTO_TEST_CASE(ArmorTradeFail)
{
    const unsigned officerSoldiersTraded = 3;
    const unsigned officerSoldiersWithArmor = 2;

    this->TradeOverLand(players[0]->getHQPos(), Job::Officer, 3);
    // For figures we don't need donkeys
    numArmoredSoldiers[getSoldierRank(Job::Officer)] -= officerSoldiersWithArmor;
    numSoldiers[getSoldierRank(Job::Officer)] -= officerSoldiersTraded;
    numHelpers -= 1;
    testExpectedFigures();
    testAfterLeaving(3);

    // Make sure all of them are a bit outside
    RTTR_SKIP_GFS(40);

    // Add a ring of enemy owned land so they cannot pass
    for(const MapPoint& pt : world.getPointsInRadius(curWh->getPos(), 10))
    {
        if(world.CalcDistance(pt, curWh->getPos()) >= 8)
            world.setOwner(pt, 2 + 1); // playerID = 2 -> Owner = +1
    }

    // Let them come in again (walk same way back, assume at most 8 nodes away + same as above)
    RTTR_SKIP_GFS(40 + 20 * 8);
    // Our stuff is back
    numArmoredSoldiers[getSoldierRank(Job::Officer)] += officerSoldiersWithArmor;
    numSoldiers[getSoldierRank(Job::Officer)] += officerSoldiersTraded;
    numHelpers += 1;
    // helpers can be produced in the meantime
    BOOST_TEST_REQUIRE(curWh->getNumRealFigures(Job::Helper) >= numHelpers);
    numHelpers = curWh->getNumRealFigures(Job::Helper);
    testExpectedFigures();
}

BOOST_AUTO_TEST_CASE(ArmorTradeFailDie)
{
    const unsigned officerSoldiersTraded = 3;
    const unsigned officerSoldiersWithArmor = 2;

    this->TradeOverLand(players[0]->getHQPos(), Job::Officer, 3);
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
    RTTR_FOREACH_PT(MapPoint, world.getSize())
        world.setOwner(pt, 2 + 1); // playerID = 2 -> Owner = +1
    // Let them all die
    RTTR_SKIP_GFS(40 + 20 * 8);
    // Don't care about helpers but check the rest: Still gone
    numHelpers = curWh->getNumRealFigures(Job::Helper);
    testExpectedFigures();
}

BOOST_AUTO_TEST_SUITE_END()
