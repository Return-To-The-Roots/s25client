// Copyright (C) 2005 - 2026 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "GameEvent.h"
#include "PointOutput.h"
#include "Ware.h"
#include "buildings/nobBaseWarehouse.h"
#include "buildings/nobMilitary.h"
#include "factories/BuildingFactory.h"
#include "figures/nofAggressiveDefender.h"
#include "figures/nofAttacker.h"
#include "figures/nofCarrier.h"
#include "figures/nofDefender.h"
#include "figures/nofPassiveSoldier.h"
#include "helpers/Range.h"
#include "helpers/containerUtils.h"
#include "helpers/pointerContainerUtils.h"
#include "pathfinding/FindPathForRoad.h"
#include "worldFixtures/WorldWithGCExecution.h"
#include "worldFixtures/initGameRNG.hpp"
#include "worldFixtures/terrainHelpers.h"
#include "world/GameWorldViewer.h"
#include "nodeObjs/noFlag.h"
#include "gameTypes/GameTypesOutput.h"
#include "gameData/MilitaryConsts.h"
#include "gameData/SettingTypeConv.h"
#include "rttr/test/random.hpp"
#include <rttr/test/testHelpers.hpp>
#include <boost/test/data/monomorphic.hpp>
#include <boost/test/data/test_case.hpp>
#include <boost/test/unit_test.hpp>
#include <array>
#include <iostream>
#include <numeric>

using SoldierState = nofActiveSoldier::SoldierState;

// LCOV_EXCL_START
static std::ostream& operator<<(std::ostream& out, const SoldierState s)
{
    return out << static_cast<unsigned>(rttr::enumCast(s));
}
// LCOV_EXCL_STOP

BOOST_AUTO_TEST_SUITE(AttackSuite)

namespace {

namespace dataset = boost::unit_test::data;

struct AttackDefaults
{
    static constexpr unsigned width = 20;
    static constexpr unsigned height = 12;
};

template<typename T>
auto calcSum(const T& collection)
{
    return std::accumulate(std::begin(collection), std::end(collection), 0u);
}

std::array<unsigned, NUM_SOLDIER_RANKS> countTroopsByRank(const nobMilitary& bld)
{
    std::array<unsigned, NUM_SOLDIER_RANKS> counts{};
    for(const auto& soldier : bld.getTroops())
        ++counts[soldier.getRank()];
    return counts;
}

void deliverCoin(nobMilitary& bld, GameWorld& world, const MapPoint hqPos)
{
    auto* hq = world.getSpecObj<nobBaseWarehouse>(hqPos);
    BOOST_TEST_REQUIRE(hq);

    hq->addToInventory(GoodCounts::make(GoodType::Coins, 1), true);
    bld.searchCoins();
}

/// Reschedule the walk event of the obj to be executed in numGFs GFs
void rescheduleWalkEvent(TestEventManager& em, noMovable& obj, unsigned numGFs)
{
    std::vector<const GameEvent*> evts = em.getObjEvents(obj);
    for(const GameEvent* ev : evts)
    {
        if(ev->id == 0)
        {
            em.rescheduleEvent(ev, em.getCurrentGF() + numGFs);
            return;
        }
    }
    BOOST_TEST_FAIL("Event not found"); // LCOV_EXCL_LINE
}

/// Move the object next to the given point. The next walk event will make it reach that point
// LCOV_EXCL_START
void moveObjTo(GameWorldBase& world, noFigure& obj, const MapPoint& pos)
{
    std::unique_ptr<noFigure> ownedObj;
    if(world.hasFigureAt(obj.getPos(), obj))
        ownedObj = world.removeFigure(obj.getPos(), obj);
    else
        ownedObj = world.removeFigure(world.getNeighbour(obj.getPos(), obj.getCurMoveDir() + 3u), obj);
    obj.setPos(world.getNeighbour(pos, Direction::West));
    world.addFigure(obj.getPos(), std::move(ownedObj));
    if(obj.isMoving())
        obj.faceDir(Direction::East);
    else
        obj.startWalking(Direction::East);
}
// LCOV_EXCL_STOP

template<unsigned T_numPlayers, unsigned T_width, unsigned T_height>
struct AttackFixtureBase : public WorldWithGCExecution<T_numPlayers, T_width, T_height>
{
    /// Positions of the players HQ
    std::array<MapPoint, T_numPlayers> hqPos;
    using Parent = WorldWithGCExecution<T_numPlayers, T_width, T_height>;
    using Parent::curPlayer;
    using Parent::world;

    AttackFixtureBase()
    {
        for(unsigned i = 0; i < T_numPlayers; i++)
        {
            curPlayer = i;
            hqPos[i] = world.getPlayer(i).getHQPos();
            makeVisible(hqPos[i]);
            this->ChangeMilitary(MILITARY_SETTINGS_SCALE);
        }
        curPlayer = 0;
        initGameRNG();
    }

    void makeVisible(const MapPoint& pt)
    {
        for(unsigned i = 0; i < T_numPlayers; i++)
            world.makeVisibleAroundPoint(pt, 1, i);
    }

    void addSoldiers(MapPoint bldPos, unsigned numSoldiers, Job soldierType, unsigned numArmoredSoldiers = 0)
    {
        auto const rank = getSoldierRank(soldierType);
        BOOST_TEST_REQUIRE(rank <= world.getGGS().getMaxMilitaryRank());
        auto* bld = world.template getSpecObj<nobMilitary>(bldPos);
        BOOST_TEST_REQUIRE(bld);
        const unsigned oldNumSoldiers = bld->getNumTroops();
        for(unsigned i = 0; i < numSoldiers; i++)
        {
            auto& soldier =
              world.addFigure(bldPos, std::make_unique<nofPassiveSoldier>(bldPos, bld->getPlayer(), bld, bld, rank));
            world.getPlayer(bld->getPlayer()).increaseInventoryJob(soldier.getJobType(), 1);
            if(numArmoredSoldiers > 0)
            {
                world.getPlayer(bld->getPlayer())
                  .increaseInventoryJob(jobEnumToAmoredSoldierEnum(soldier.getJobType()), 1);
                soldier.setArmor(true);
                numArmoredSoldiers--;
            }
            // Let him "walk" to goal -> Already reached -> Added and all internal states set correctly
            soldier.walkToGoal();
            BOOST_TEST_REQUIRE(!soldier.getGoal());
        }
        BOOST_TEST_REQUIRE(bld->getNumTroops() == oldNumSoldiers + numSoldiers);
    }

    void addSoldiers(MapPoint bldPos, unsigned numWeak, unsigned numStrong)
    {
        addSoldiers(bldPos, numWeak, Job::Private);
        addSoldiers(bldPos, numStrong, Job::General);
    }

    // Named constants for indices to make access to the returned array easier to read
    static constexpr unsigned real = 0;
    static constexpr unsigned visual = 1;
    static auto getNumSoldiers(const nobBaseWarehouse& wh)
    {
        std::array<std::array<unsigned, NUM_SOLDIER_RANKS>, 2> soldiers{};
        for(const auto job : SOLDIER_JOBS)
        {
            soldiers[real][getSoldierRank(job)] = wh.getNumRealFigures(job);
            soldiers[visual][getSoldierRank(job)] = wh.getNumVisualFigures(job);
        }
        return soldiers;
    }
    // Get counts of soldiers
    auto getTotalSoldiers(unsigned player)
    {
        const auto& inv = world.getPlayer(player).getInventory();
        std::array<unsigned, NUM_SOLDIER_RANKS> soldiers{};
        for(const auto job : SOLDIER_JOBS)
            soldiers[getSoldierRank(job)] = inv[job];
        return soldiers;
    }
};

// Size is chosen based on current maximum attacking distances!
struct NumSoldierTestFixture : public AttackFixtureBase<3, 56, 38>
{
    /// Tested positions for military buildings
    MapPoint milBld0Pos, milBld1NearPos, milBld1FarPos;
    /// Military buildings of players 0 and 1 (for 1, one is close and one far away from player 0)
    const nobMilitary *milBld0, *milBld1Near, *milBld1Far;
    GameWorldViewer gwv;

    NumSoldierTestFixture() : gwv(curPlayer, world)
    {
        // Add some soldiers to the HQs
        for(const auto pos : hqPos)
        {
            world.getSpecObj<nobBaseWarehouse>(pos)->addToInventory(
              PeopleCounts::make(Job::General, rttr::test::randomValue(3, 10)), true);
        }
        // Assert player positions: 0: Top-Left, 1: Top-Right, 2: Bottom-Middle
        BOOST_TEST_REQUIRE(hqPos[0].x < hqPos[1].x);
        BOOST_TEST_REQUIRE(hqPos[0].y < hqPos[2].y);
        BOOST_TEST_REQUIRE(hqPos[1].y < hqPos[2].y);

        // Build some military buildings
        milBld0Pos = hqPos[0] + MapPoint(7, 0);
        BOOST_TEST_REQUIRE(world.getBQ(milBld0Pos, 0) == BuildingQuality::Castle);
        milBld0 = dynamic_cast<nobMilitary*>(
          BuildingFactory::createBuilding(world, BuildingType::Watchtower, milBld0Pos, 0, Nation::Babylonians));
        BOOST_TEST_REQUIRE(milBld0);

        milBld1NearPos = hqPos[1] - MapPoint(7, 0);
        BOOST_TEST_REQUIRE(world.getBQ(milBld1NearPos, 1) == BuildingQuality::Castle);
        milBld1Near = dynamic_cast<nobMilitary*>(
          BuildingFactory::createBuilding(world, BuildingType::Watchtower, milBld1NearPos, 1, Nation::Romans));
        BOOST_TEST_REQUIRE(milBld1Near);

        milBld1FarPos = hqPos[1] + MapPoint(3, 1);
        BOOST_TEST_REQUIRE(world.getBQ(milBld1FarPos, 1) == BuildingQuality::Castle);
        milBld1Far = dynamic_cast<nobMilitary*>(
          BuildingFactory::createBuilding(world, BuildingType::Watchtower, milBld1FarPos, 1, Nation::Romans));
        BOOST_TEST_REQUIRE(milBld1Far);
        makeVisible(milBld0Pos);
        makeVisible(milBld1NearPos);
        makeVisible(milBld1FarPos);
    }

    void setCurPlayer(unsigned playerIdx)
    {
        curPlayer = playerIdx;
        gwv.changePlayer(playerIdx, false);
    }
};

template<unsigned T_numPlayers = 2, unsigned T_width = AttackDefaults::width,
         unsigned T_height = AttackDefaults::height>
struct AttackFixture : public AttackFixtureBase<T_numPlayers, T_width, T_height>
{
    using Parent = AttackFixtureBase<T_numPlayers, T_width, T_height>;
    using Parent::curPlayer;
    using Parent::hqPos;
    using Parent::makeVisible;
    using Parent::world;

    /// Tested positions for military buildings
    MapPoint milBld0Pos, milBld1Pos;
    /// Military buildings of players 0 and 1
    nobMilitary *milBld0, *milBld1;

    AttackFixture()
    {
        // Build some military buildings far away enough for holding some area outside HQ
        milBld0Pos = world.makeMapPoint(hqPos[0] + Position(0, 6));
        BOOST_TEST_REQUIRE(world.getBQ(milBld0Pos, 0) == BuildingQuality::Castle);
        milBld0 = static_cast<nobMilitary*>(
          BuildingFactory::createBuilding(world, BuildingType::Watchtower, milBld0Pos, 0, Nation::Babylonians));
        BOOST_TEST_REQUIRE(milBld0);

        milBld1Pos = world.makeMapPoint(hqPos[1] + Position(0, 6));
        BOOST_TEST_REQUIRE(world.getBQ(milBld1Pos, 1) == BuildingQuality::Castle);
        milBld1 = static_cast<nobMilitary*>(
          BuildingFactory::createBuilding(world, BuildingType::Watchtower, milBld1Pos, 1, Nation::Romans));
        BOOST_TEST_REQUIRE(milBld1);
        makeVisible(milBld0Pos);
        makeVisible(milBld1Pos);
    }

    /// Assert that attacking the given building from attackSrc fails
    void testFailingAttack(const GameWorldViewer& gwv, const MapPoint& bldPos, const nobMilitary& attackSrc,
                           unsigned numSoldiersLeft = 6u)
    {
        BOOST_TEST_REQUIRE(attackSrc.getNumTroops() == numSoldiersLeft);
        // No available soldiers
        BOOST_TEST_REQUIRE(gwv.getNumSoldiersForAttack(bldPos) == 0u);
        this->Attack(bldPos, 1, true);
        // Same left
        BOOST_TEST_REQUIRE(attackSrc.getNumTroops() == numSoldiersLeft);
    }
};
} // namespace

BOOST_FIXTURE_TEST_CASE(NumSoldiersForAttack, NumSoldierTestFixture)
{
    addSoldiers(milBld0Pos, 5, Job::General);
    addSoldiers(milBld0Pos, 1, Job::Sergeant);
    addSoldiers(milBld1NearPos, 6, Job::General);
    addSoldiers(milBld1FarPos, 6, Job::Private);

    // Functions related to stationed soldiers
    BOOST_TEST_REQUIRE(milBld0->hasMaxRankSoldier());
    BOOST_TEST_REQUIRE(milBld1Near->hasMaxRankSoldier());
    BOOST_TEST_REQUIRE(!milBld1Far->hasMaxRankSoldier());
    BOOST_TEST_REQUIRE(milBld1Near->getSoldiersStrength() > milBld1Far->getSoldiersStrength());
    BOOST_TEST_REQUIRE(milBld1Near->getSoldiersStrength() > milBld0->getSoldiersStrength());

    // Player 2 has no military blds -> Can't attack
    setCurPlayer(2);
    BOOST_TEST_REQUIRE(gwv.getNumSoldiersForAttack(hqPos[0]) == 0u);
    BOOST_TEST_REQUIRE(gwv.getNumSoldiersForAttack(hqPos[1]) == 0u);
    BOOST_TEST_REQUIRE(gwv.getNumSoldiersForAttack(milBld0Pos) == 0u);
    BOOST_TEST_REQUIRE(gwv.getNumSoldiersForAttack(milBld1NearPos) == 0u);
    BOOST_TEST_REQUIRE(gwv.getNumSoldiersForAttack(milBld1FarPos) == 0u);
    setCurPlayer(1);
    // No self attack
    BOOST_TEST_REQUIRE(gwv.getNumSoldiersForAttack(hqPos[1]) == 0u);
    BOOST_TEST_REQUIRE(gwv.getNumSoldiersForAttack(milBld1NearPos) == 0u);
    BOOST_TEST_REQUIRE(gwv.getNumSoldiersForAttack(milBld1FarPos) == 0u);
    // Attack both others
    BOOST_TEST_REQUIRE(gwv.getNumSoldiersForAttack(hqPos[0]) == 5u);
    BOOST_TEST_REQUIRE(gwv.getNumSoldiersForAttack(hqPos[2]) == 5u);
    // This is in the extended range of the far bld -> 2 more (with current range)
    BOOST_TEST_REQUIRE(gwv.getNumSoldiersForAttack(milBld0Pos) == 7u);
    setCurPlayer(0);
    // No self attack
    BOOST_TEST_REQUIRE(gwv.getNumSoldiersForAttack(hqPos[0]) == 0u);
    BOOST_TEST_REQUIRE(gwv.getNumSoldiersForAttack(milBld0Pos) == 0u);
    // Attack both others
    BOOST_TEST_REQUIRE(gwv.getNumSoldiersForAttack(hqPos[1]) == 5u);
    BOOST_TEST_REQUIRE(gwv.getNumSoldiersForAttack(hqPos[2]) == 5u);
    BOOST_TEST_REQUIRE(gwv.getNumSoldiersForAttack(milBld1NearPos) == 5u);
    // Counterpart: 2 possible for far bld
    BOOST_TEST_REQUIRE(gwv.getNumSoldiersForAttack(milBld1FarPos) == 2u);

    // Test in peaceful mode -- no attacks should be possible
    this->ggs.setSelection(AddonId::PEACEFULMODE, 1);
    setCurPlayer(1);
    BOOST_TEST_REQUIRE(gwv.getNumSoldiersForAttack(hqPos[0]) == 0u);
    BOOST_TEST_REQUIRE(gwv.getNumSoldiersForAttack(hqPos[2]) == 0u);
    BOOST_TEST_REQUIRE(gwv.getNumSoldiersForAttack(milBld0Pos) == 0u);
    setCurPlayer(0);
    BOOST_TEST_REQUIRE(gwv.getNumSoldiersForAttack(hqPos[1]) == 0u);
    BOOST_TEST_REQUIRE(gwv.getNumSoldiersForAttack(hqPos[2]) == 0u);
    BOOST_TEST_REQUIRE(gwv.getNumSoldiersForAttack(milBld1NearPos) == 0u);
}

BOOST_FIXTURE_TEST_CASE(startAttack, AttackFixture<>)
{
    GameWorldViewer gwv(curPlayer, world);

    // Add soldiers (3 strong, 3 weak)
    addSoldiers(milBld0Pos, 3, 3);

    const nobMilitary& attackSrc = *milBld0;
    const MapPoint usualBldPos = hqPos[1] - MapPoint(2, 3);
    BOOST_TEST_REQUIRE(world.getBQ(usualBldPos, 1) >= BuildingQuality::Hut);
    const noBuilding* usualBld =
      BuildingFactory::createBuilding(world, BuildingType::Woodcutter, usualBldPos, 1, Nation::Romans);
    BOOST_TEST_REQUIRE(usualBld);
    const MapPoint storehousePos = hqPos[1] - MapPoint(2, 0);
    BOOST_TEST_REQUIRE(world.getBQ(storehousePos, 1) >= BuildingQuality::House);
    const noBuilding* storeHouse =
      BuildingFactory::createBuilding(world, BuildingType::Storehouse, storehousePos, 1, Nation::Romans);
    BOOST_TEST_REQUIRE(storeHouse);
    makeVisible(usualBldPos);
    makeVisible(storehousePos);

    world.getPlayer(0).team = Team::Team1;
    world.getPlayer(1).team = Team::Team1;
    for(unsigned i = 0; i < 2; i++)
        world.getPlayer(i).makeStartPacts();
    // Try to attack ally -> Fail
    testFailingAttack(gwv, milBld1Pos, attackSrc);

    world.getPlayer(0).team = Team::Team1;
    world.getPlayer(1).team = Team::Team2;
    for(unsigned i = 0; i < 2; i++)
        world.getPlayer(i).makeStartPacts();

    // Try to attack non-military bld -> Fail
    testFailingAttack(gwv, usualBldPos, attackSrc);

    // Try to attack storehouse -> Fail
    testFailingAttack(gwv, storehousePos, attackSrc);

    // Try to attack newly build bld -> Fail
    BOOST_TEST_REQUIRE(world.calcVisiblityWithAllies(milBld1Pos, curPlayer) == Visibility::Visible);
    BOOST_TEST_REQUIRE(milBld1->isNewBuilt());
    testFailingAttack(gwv, milBld1Pos, attackSrc);

    // Add soldier
    addSoldiers(milBld1Pos, 1, 0);
    BOOST_TEST_REQUIRE(!milBld1->isNewBuilt());
    // Try to attack invisible bld -> Fail
    MapNode& node = world.getNodeWriteable(milBld1Pos);
    node.fow[0].visibility = Visibility::FogOfWar;
    BOOST_TEST_REQUIRE(world.calcVisiblityWithAllies(milBld1Pos, curPlayer) == Visibility::FogOfWar);
    testFailingAttack(gwv, milBld1Pos, attackSrc);

    // Attack it
    node.fow[0].visibility = Visibility::Visible;
    BOOST_TEST_REQUIRE(attackSrc.getNumTroops() == 6u);
    auto itTroops = attackSrc.getTroops().begin();
    for(int i = 0; i < 3; i++, ++itTroops)
        BOOST_TEST_REQUIRE(itTroops->getRank() == 0u);
    for(int i = 3; i < 6; i++, ++itTroops)
        BOOST_TEST_REQUIRE(itTroops->getRank() == 4u);
    this->Attack(milBld1Pos, 1, true);
    // 1 strong soldier has left
    BOOST_TEST_REQUIRE(attackSrc.getNumTroops() == 5u);
    itTroops = attackSrc.getTroops().begin();
    for(int i = 0; i < 3; i++, ++itTroops)
        BOOST_TEST_REQUIRE(itTroops->getRank() == 0u);
    for(int i = 3; i < 5; i++, ++itTroops)
        BOOST_TEST_REQUIRE(itTroops->getRank() == 4u);
    // Attack with 1 weak soldier
    this->Attack(milBld1Pos, 1, false);
    // 1 weak soldier has left
    BOOST_TEST_REQUIRE(attackSrc.getNumTroops() == 4u);
    itTroops = attackSrc.getTroops().begin();
    for(int i = 0; i < 2; i++, ++itTroops)
        BOOST_TEST_REQUIRE(itTroops->getRank() == 0u);
    for(int i = 2; i < 4; i++, ++itTroops)
        BOOST_TEST_REQUIRE(itTroops->getRank() == 4u);
    // -> 2 strong, 2 weak remaining, attack with 3 weak ones -> 1 strong remaining
    this->Attack(milBld1Pos, 3, false);
    BOOST_TEST_REQUIRE(attackSrc.getNumTroops() == 1u);
    BOOST_TEST(attackSrc.getTroops().front().getRank() == 4u);

    // None left
    testFailingAttack(gwv, milBld1Pos, attackSrc, 1u);
}

BOOST_FIXTURE_TEST_CASE(TroopLimitCommandRejectsInvalidSerializedRank, AttackFixture<>)
{
    std::array<unsigned, NUM_SOLDIER_RANKS> expectedLimits{};
    const auto firstLimit = rttr::test::randomValue(1u, 20u);
    const auto limitStep = rttr::test::randomValue(1u, 5u);
    // Use unique values to detect overwriting
    for(unsigned rank = 0; rank < expectedLimits.size(); ++rank)
    {
        expectedLimits[rank] = firstLimit + rank * limitStep;
        this->SetTroopLimit(milBld0Pos, rank, expectedLimits[rank]);
    }

    for(unsigned rank = 0; rank < expectedLimits.size(); ++rank)
        BOOST_TEST_REQUIRE(milBld0->getTroopLimit(rank) == expectedLimits[rank]);

    BOOST_REQUIRE_THROW(this->SetTroopLimit(milBld0Pos, NUM_SOLDIER_RANKS, expectedLimits.back() + limitStep),
                        std::range_error);

    for(unsigned rank = 0; rank < expectedLimits.size(); ++rank)
        BOOST_TEST_REQUIRE(milBld0->getTroopLimit(rank) == expectedLimits[rank]);
}

BOOST_FIXTURE_TEST_CASE(ConquerBld, AttackFixture<>)
{
    addSoldiers(milBld0Pos, 1, 5);
    addSoldiers(milBld1Pos, 1, Job::Private);
    addSoldiers(milBld1Pos, 1, Job::PrivateFirstClass);

    // Add soldiers and build road so that HQ and send additional soldiers to the building starting the attack
    PeopleCounts soldiers;
    soldiers[Job::Sergeant] = 10;
    ensureNonNull(world.getSpecObj<nobBaseWarehouse>(hqPos[curPlayer])).addToInventory(soldiers, true);
    buildRoadForBlds(milBld0Pos, hqPos[0]);

    // Start attack ->1 (weak one first)
    this->Attack(milBld1Pos, 1, false);
    this->Attack(milBld1Pos, 5, false);
    BOOST_TEST_REQUIRE(milBld0->getNumTroops() == 1u);
    BOOST_TEST_REQUIRE(milBld1->getNumTroops() == 2u);
    // Run till attackers reach bld. 1 Soldier will leave for them.
    // 1 stays inside till an attacker is at door
    // 20 GFs/node + 30 GFs for leaving
    const unsigned distance = world.calcDistance(milBld0Pos, milBld1Pos);
    RTTR_EXEC_TILL(distance * 20 + 30, milBld1->getNumTroops() == 1);
    BOOST_TEST_REQUIRE(milBld1->getNumTroops() + milBld1->getLeavingFigures().size() == 2u);
    const Inventory& attackedPlInventory = world.getPlayer(1).getInventory();
    const unsigned oldWeakSoldierCt = attackedPlInventory.people[Job::Private];
    const unsigned oldStrongerSoldierCt = attackedPlInventory.people[Job::PrivateFirstClass];
    const unsigned oldAttackerStrongSoldierCt = world.getPlayer(curPlayer).getInventory().people[Job::General];

    // 1st soldier will walk towards attacker and will be killed
    // Once an attacker reaches the flag, the bld will send a defender
    BOOST_TEST_REQUIRE(!milBld1->getDefender());
    RTTR_EXEC_TILL(300, milBld1->getNumTroops() == 0);
    // Defender deployed, attacker at flag
    BOOST_TEST_REQUIRE(milBld1->getDefender());
    {
        const auto figures = world.getFigures(milBld1->getFlagPos());
        BOOST_TEST_REQUIRE(figures.size() == 1u);
        const auto& attacker = dynamic_cast<const nofAttacker&>(*figures.begin());
        BOOST_TEST_REQUIRE(static_cast<const nofAttacker&>(attacker).getPlayer() == curPlayer);
    }

    // Lets fight
    RTTR_EXEC_TILL(1000, milBld1->isBeingCaptured());
    // Let others in
    RTTR_EXEC_TILL(200, !milBld1->isBeingCaptured());
    // Building conquered
    BOOST_TEST_REQUIRE(milBld1->getPlayer() == curPlayer);
    // 1 soldier must be inside
    BOOST_TEST_REQUIRE(milBld1->getNumTroops() > 1u);
    // Weak soldier must be dead
    BOOST_TEST_REQUIRE(attackedPlInventory.people[Job::Private] == oldWeakSoldierCt - 1);
    // Src building gets refilled
    RTTR_EXEC_TILL(800, milBld0->getNumTroops() == 6u);
    // We may have lost soldiers
    BOOST_TEST_REQUIRE(world.getPlayer(curPlayer).getInventory().people[Job::General] <= oldAttackerStrongSoldierCt);
    // The enemy may have lost his stronger soldier
    BOOST_TEST_REQUIRE(attackedPlInventory.people[Job::PrivateFirstClass] <= oldStrongerSoldierCt);
    // But only one
    BOOST_TEST_REQUIRE(attackedPlInventory.people[Job::PrivateFirstClass] >= oldStrongerSoldierCt - 1);
    // At least 2 survivors
    BOOST_TEST_REQUIRE(milBld1->getNumTroops() > 2u);

    // Points around bld should be ours
    const std::vector<MapPoint> pts = world.getPointsInRadius(milBld1Pos, 3);
    for(const MapPoint& pt : pts)
    {
        BOOST_TEST_REQUIRE(world.getNode(pt).owner == curPlayer + 1u);
    }
}

BOOST_FIXTURE_TEST_CASE(ArmoredSoldierLosesArmorInFight, AttackFixture<>)
{
    addSoldiers(milBld0Pos, 3, Job::General, 2);
    addSoldiers(milBld1Pos, 1, Job::Private, 1);

    BOOST_TEST_REQUIRE(milBld0->getNumTroops() == 3u);
    BOOST_TEST_REQUIRE(milBld1->getNumTroops() == 1u);

    // Start attack ->1 (strong one first)
    this->Attack(milBld1Pos, 1, true);
    RTTR_EXEC_TILL(30, milBld0->getNumTroops() == 2);

    auto countArmoredSoldiers = [](nobMilitary* bld) {
        const auto troops = bld->getTroops();
        return std::count_if(std::begin(troops), std::end(troops),
                             [](const auto& soldier) { return soldier.hasArmor(); });
    };

    auto countNotArmoredSoldiers = [](nobMilitary* bld) {
        const auto troops = bld->getTroops();
        return std::count_if(std::begin(troops), std::end(troops),
                             [](const auto& soldier) { return !soldier.hasArmor(); });
    };

    BOOST_TEST_REQUIRE(countArmoredSoldiers(milBld0) == 1u);
    BOOST_TEST_REQUIRE(countNotArmoredSoldiers(milBld0) == 1u);

    // Start attack ->1 (weak one first)
    this->Attack(milBld1Pos, 1, false);
    RTTR_EXEC_TILL(30, milBld0->getNumTroops() == 1);
    BOOST_TEST_REQUIRE(countArmoredSoldiers(milBld0) == 1u);
    BOOST_TEST_REQUIRE(countNotArmoredSoldiers(milBld0) == 0u);

    const Inventory& attackedPlInventory = world.getPlayer(1).getInventory();
    const unsigned numOldWeakSoldiers = attackedPlInventory.people[Job::Private];
    const unsigned numOldWeakSoldiersWithArmor = attackedPlInventory[jobEnumToAmoredSoldierEnum(Job::Private)];

    // Run till attackers reach flag of bld. The bld will send a defender.
    // 20 GFs/node + 60 GFs for leaving
    const unsigned distance = world.calcDistance(milBld0Pos, milBld1Pos);
    RTTR_EXEC_TILL(distance * 20 + 60, milBld1->getDefender() != nullptr);

    const auto soldiers = world.getFigures(milBld1->getFlagPos());
    BOOST_TEST_REQUIRE(soldiers.size() == 1u);
    const auto& attacker = dynamic_cast<const nofAttacker&>(*soldiers.begin());
    BOOST_TEST_REQUIRE(static_cast<const nofAttacker&>(attacker).getPlayer() == curPlayer);

    // Lets fight until defender has no armor anymore
    // He should not lose a hitpoint but the armor
    RTTR_EXEC_TILL(1000, milBld1->getDefender()->hasArmor() == false);
    BOOST_TEST(attackedPlInventory[jobEnumToAmoredSoldierEnum(Job::Private)] == numOldWeakSoldiersWithArmor - 1);
    BOOST_TEST(attackedPlInventory.people[Job::Private] == numOldWeakSoldiers);
    BOOST_TEST(milBld1->getDefender()->getHitpoints() == HITPOINTS[milBld1->getDefender()->getRank()]);
}

BOOST_FIXTURE_TEST_CASE(TroopLimitKeepsOrderedRestrictedSoldier, AttackFixture<>)
{
    MilitarySettings milSettings = MILITARY_SETTINGS_SCALE;
    milSettings[4 + rttr::enumCast(FrontierDistance::Far)] = 0;
    this->ChangeMilitary(milSettings);

    auto* hq = world.getSpecObj<nobBaseWarehouse>(hqPos[0]);
    BOOST_TEST_REQUIRE(hq);

    const Job soldierJob = Job::Private;
    const unsigned soldierRank = getSoldierRank(soldierJob);
    hq->addToInventory(PeopleCounts::make(soldierJob, 2), true);
    const unsigned hqSoldiersBefore = hq->getNumRealFigures(soldierJob);
    BOOST_TEST_REQUIRE(hqSoldiersBefore > 0u);

    buildRoadForBlds(milBld0Pos, hqPos[0]);
    milBld0->regulateTroops();

    BOOST_TEST_REQUIRE(milBld0->getNumTroops() == 0u);
    BOOST_TEST_REQUIRE(hq->getLeavingFigures().size() == 1u);
    BOOST_TEST_REQUIRE(hq->getNumRealFigures(soldierJob) + 1 == hqSoldiersBefore);

    this->SetInventorySetting(hqPos[0], soldierJob, EInventorySetting::Stop);
    milBld0->SetTroopLimit(soldierRank, 0);

    BOOST_TEST_REQUIRE(milBld0->getNumTroops() == 0u);
    BOOST_TEST_REQUIRE(hq->getLeavingFigures().size() == 1u);
    BOOST_TEST_REQUIRE(hq->getNumRealFigures(soldierJob) + 1 == hqSoldiersBefore);

    RTTR_EXEC_TILL(500, milBld0->getNumTroops() == 1u);

    BOOST_TEST_REQUIRE(milBld0->getNumTroops() == 1u);
    BOOST_TEST_REQUIRE(hq->getNumRealFigures(soldierJob) + 1 == hqSoldiersBefore);
    BOOST_TEST_REQUIRE(milBld0->getLeavingFigures().empty());
}

BOOST_FIXTURE_TEST_CASE(CoinTrainingUpgradesRankChainByDefault, AttackFixture<>)
{
    addSoldiers(milBld0Pos, 1, Job::Private);
    addSoldiers(milBld0Pos, 1, Job::PrivateFirstClass);
    addSoldiers(milBld0Pos, 1, Job::Sergeant);

    buildRoadForBlds(milBld0Pos, hqPos[0]);
    deliverCoin(*milBld0, world, hqPos[0]);

    RTTR_EXEC_TILL(5000, countTroopsByRank(*milBld0)[3] == 1u);

    const auto counts = countTroopsByRank(*milBld0);
    BOOST_TEST_REQUIRE(counts[0] == 0u);
    BOOST_TEST_REQUIRE(counts[1] == 1u);
    BOOST_TEST_REQUIRE(counts[2] == 1u);
    BOOST_TEST_REQUIRE(counts[3] == 1u);
}

BOOST_FIXTURE_TEST_CASE(SingleSoldierCoinTrainingUpgradesOnlyLowestRankSoldier, AttackFixture<>)
{
    this->ggs.setSelection(AddonId::SINGLE_SOLDIER_COIN_TRAINING, 1);

    addSoldiers(milBld0Pos, 1, Job::Private);
    addSoldiers(milBld0Pos, 1, Job::PrivateFirstClass);
    addSoldiers(milBld0Pos, 1, Job::Sergeant);

    buildRoadForBlds(milBld0Pos, hqPos[0]);
    deliverCoin(*milBld0, world, hqPos[0]);

    RTTR_EXEC_TILL(5000, countTroopsByRank(*milBld0)[1] == 2u);

    const auto counts = countTroopsByRank(*milBld0);
    BOOST_TEST_REQUIRE(counts[0] == 0u);
    BOOST_TEST_REQUIRE(counts[1] == 2u);
    BOOST_TEST_REQUIRE(counts[2] == 1u);
    BOOST_TEST_REQUIRE(counts[3] == 0u);
}

BOOST_FIXTURE_TEST_CASE(ConquerBldCoinAddonEnable, AttackFixture<>)
{
    this->ggs.setSelection(AddonId::COINS_CAPTURED_BLD, 1); // addon is active on second run

    addSoldiers(milBld0Pos, 1, 5);
    addSoldiers(milBld1Pos, 1, Job::Private);

    // ensure that coins are disabled
    milBld1->SetCoinsAllowed(false);
    BOOST_TEST_REQUIRE(milBld1->isGoldDisabled());

    // Start attack -> 1
    this->Attack(milBld1Pos, 6, false);
    BOOST_TEST_REQUIRE(milBld0->getNumTroops() == 1u);

    RTTR_EXEC_TILL(2000, milBld1->getPlayer() == curPlayer);

    // check if coins were enabled after building was captured
    BOOST_TEST_REQUIRE(!milBld1->isGoldDisabled());
}

BOOST_FIXTURE_TEST_CASE(ConquerBldCoinAddonDisable, AttackFixture<>)
{
    this->ggs.setSelection(AddonId::COINS_CAPTURED_BLD, 2); // addon is active on second run

    addSoldiers(milBld0Pos, 1, 5);
    addSoldiers(milBld1Pos, 1, Job::Private);

    // ensure that coins are enabled
    milBld1->SetCoinsAllowed(true);
    BOOST_TEST_REQUIRE(!milBld1->isGoldDisabled());

    // Start attack -> 1
    this->Attack(milBld1Pos, 6, false);
    BOOST_TEST_REQUIRE(milBld0->getNumTroops() == 1u);

    RTTR_EXEC_TILL(2000, milBld1->getPlayer() == curPlayer);

    // check if coins were disabled after building was captured
    BOOST_TEST_REQUIRE(milBld1->isGoldDisabled());
}

enum Case
{
    EnableAfterDisable,
    DisableAfterEnable,
    KeepEnabled,
    KeepDisabled
};

BOOST_DATA_TEST_CASE_F(AttackFixture<>, ConquerBldArmorAddon,
                       dataset::make(std::array{Case::EnableAfterDisable, Case::DisableAfterEnable, Case::KeepEnabled,
                                                Case::KeepDisabled}))
{
    if(sample == Case::EnableAfterDisable)
        this->ggs.setSelection(AddonId::ARMOR_CAPTURED_BLD, 1);
    else if(sample == Case::DisableAfterEnable)
        this->ggs.setSelection(AddonId::ARMOR_CAPTURED_BLD, 2);
    else
        this->ggs.setSelection(AddonId::ARMOR_CAPTURED_BLD, 0);

    addSoldiers(milBld0Pos, 1, 5);
    addSoldiers(milBld1Pos, 1, Job::Private);

    // ensure that armor are enabled/disabled
    bool armorAllowedBeforeAttack = (sample == Case::KeepEnabled || sample == Case::DisableAfterEnable);
    milBld1->SetArmorAllowed(armorAllowedBeforeAttack);
    BOOST_TEST_REQUIRE(milBld1->isArmorAllowed() == armorAllowedBeforeAttack);

    // Start attack -> 1
    this->Attack(milBld1Pos, 5, false);
    BOOST_TEST_REQUIRE(milBld0->getNumTroops() == 1u);

    RTTR_EXEC_TILL(2000, milBld1->getPlayer() == curPlayer);

    // check if armor were enabled/disabled after building was captured
    bool armorAlloweAfterCaptured = (sample == Case::KeepEnabled || sample == Case::EnableAfterDisable);
    BOOST_TEST_REQUIRE(milBld1->isArmorAllowed() == armorAlloweAfterCaptured);
}

using AttackFixture4P = AttackFixture<4, 32, 34>;
BOOST_FIXTURE_TEST_CASE(ConquerWithMultipleWalkingIn, AttackFixture4P)
{
    world.getPlayer(0).team = Team::Team1; //-V525
    world.getPlayer(1).team = Team::None;
    world.getPlayer(2).team = Team::Team1; // Allied to 0
    world.getPlayer(3).team = Team::Team2; // Hostile to 0
    for(unsigned i = 0; i < 4; i++)
        world.getPlayer(i).makeStartPacts();
    MilitarySettings milSettings = MILITARY_SETTINGS_SCALE;
    milSettings[2] = 0; // No aggressive defenders for attacker
    this->ChangeMilitary(milSettings);

    addSoldiers(milBld0Pos, 0, 6);
    addSoldiers(milBld1Pos, 1, Job::Private);
    const MapPoint milBld1FlagPos = milBld1->getFlagPos();

    // Scenario 1: Attack with one soldier.
    // Once enemy is defeated we walk in with another soldier of the enemy who wants to occupy its building.
    // The other soldier is faster -> we have to fight him
    this->Attack(milBld1Pos, 1, true);
    BOOST_TEST_REQUIRE(milBld0->getLeavingFigures().size() == 1u); //-V807
    auto& attacker = dynamic_cast<nofAttacker&>(milBld0->getLeavingFigures().front());
    // Let him come out
    RTTR_EXEC_TILL(70, milBld0->getLeavingFigures().empty()); //-V807
    moveObjTo(world, attacker, milBld1FlagPos);
    BOOST_TEST_REQUIRE(!milBld1->isDoorOpen());
    const auto flagFigs = world.getFigures(milBld1FlagPos);
    RTTR_EXEC_TILL(70, flagFigs.size() == 1u && flagFigs.begin()->getGOT() == GO_Type::Fighting); //-V807
    BOOST_TEST_REQUIRE(!milBld1->isDoorOpen());
    // Speed up fight by reducing defenders HP to 1
    auto* defender = const_cast<nofDefender*>(milBld1->getDefender());
    while(defender->getHitpoints() > 1u)
        defender->takeHit();
    RTTR_EXEC_TILL(500, milBld1->getDefender() == nullptr);
    // Defender defeated. Attacker moving in.
    BOOST_TEST_REQUIRE(attacker.isMoving());
    BOOST_TEST_REQUIRE(attacker.getCurMoveDir() == Direction::NorthWest);
    // Door opened
    BOOST_TEST_REQUIRE(milBld1->isDoorOpen());
    // New soldiers walked in
    addSoldiers(milBld1Pos, 4, Job::Private);
    // Let attacker walk in (try it at least)
    RTTR_EXEC_TILL(20, attacker.getPos() == milBld1Pos);
    RTTR_EXEC_TILL(20, attacker.getPos() == milBld1FlagPos);
    // New fight and door closed
    RTTR_EXEC_TILL(70, flagFigs.size() == 1u && flagFigs.front().getGOT() == GO_Type::Fighting);
    BOOST_TEST_REQUIRE(!milBld1->isDoorOpen());

    // Scenario 2: Attacker successful
    // We want all possible troops:
    // 1. Attackers from this building
    // 2. Aggressive defenders from this building
    // 3. Allied aggressor towards this bld
    // 4. Hostile aggressor towards this bld
    // 5. Occupying soldier of the player on the way in the building
    // 1.
    curPlayer = 1;
    this->Attack(milBld0Pos, 1, false);
    BOOST_TEST_REQUIRE(milBld1->getLeavingFigures().size() == 1u);
    auto& attackerFromPl0 = dynamic_cast<nofAttacker&>(milBld1->getLeavingFigures().front());
    // 2.
    curPlayer = 0;
    this->Attack(milBld1Pos, 1, true);
    // Move him directly out
    BOOST_TEST_REQUIRE(milBld0->getLeavingFigures().size() == 1u);
    auto& secAttacker = dynamic_cast<nofAttacker&>(milBld0->getLeavingFigures().front());
    RTTR_EXEC_TILL(70, milBld0->getLeavingFigures().empty()); //-V807
    moveObjTo(world, secAttacker, world.makeMapPoint(milBld1FlagPos - Position(15, 0)));
    nofAggressiveDefender& aggDefender = ensureNonNull(milBld1->sendAggressiveDefender(secAttacker));
    secAttacker.letsFight(aggDefender);
    // 3.
    curPlayer = 2;
    MapPoint bldPos = hqPos[curPlayer] + MapPoint(3, 0);
    auto* alliedBld = static_cast<nobMilitary*>(
      BuildingFactory::createBuilding(world, BuildingType::Guardhouse, bldPos, curPlayer, Nation::Africans));
    addSoldiers(bldPos, 2, Job::Private);
    this->Attack(milBld1Pos, 1, false);
    BOOST_TEST_REQUIRE(alliedBld->getLeavingFigures().size() == 1u);
    auto& alliedAttacker = dynamic_cast<nofAttacker&>(alliedBld->getLeavingFigures().front());
    // 4.
    curPlayer = 3;
    bldPos = hqPos[curPlayer] + MapPoint(3, 0);
    auto* hostileBld = static_cast<nobMilitary*>(
      BuildingFactory::createBuilding(world, BuildingType::Guardhouse, bldPos, curPlayer, Nation::Africans));
    addSoldiers(bldPos, 2, Job::Private);
    this->Attack(milBld1Pos, 1, false);
    BOOST_TEST_REQUIRE(hostileBld->getLeavingFigures().size() == 1u);
    auto& hostileAttacker = dynamic_cast<nofAttacker&>(hostileBld->getLeavingFigures().front());

    // Make sure all other soldiers left their buildings (<=30GFs each + 20 for walking to flag and a bit further)
    RTTR_SKIP_GFS(30 + 20 + 10);
    // And suspend them to inspect them later on
    rescheduleWalkEvent(em, attackerFromPl0, 10000);
    rescheduleWalkEvent(em, secAttacker, 10000);
    rescheduleWalkEvent(em, alliedAttacker, 10000);
    rescheduleWalkEvent(em, hostileAttacker, 10000);
    // We got 2 from milBld1
    RTTR_SKIP_GFS(30);
    rescheduleWalkEvent(em, aggDefender, 10000);
    // Let defenders (2!) die
    defender = const_cast<nofDefender*>(milBld1->getDefender());
    while(defender->getHitpoints() > 1u)
        defender->takeHit();
    RTTR_EXEC_TILL(500, milBld1->getNumTroops() == 0u);
    defender = const_cast<nofDefender*>(milBld1->getDefender());
    while(defender->getHitpoints() > 1u)
        defender->takeHit();
    RTTR_EXEC_TILL(500, milBld1->getDefender() == nullptr);
    // Defender defeated. Attacker moving in.
    BOOST_TEST_REQUIRE(attacker.isMoving());
    BOOST_TEST_REQUIRE(attacker.getCurMoveDir() == Direction::NorthWest);
    // Door opened
    BOOST_TEST_REQUIRE(milBld1->isDoorOpen());
    // Give him a bit of a head start
    RTTR_SKIP_GFS(1);
    // 5. Create new soldier who walks in after the attacker
    auto& newSld =
      world.addFigure(milBld1FlagPos, std::make_unique<nofPassiveSoldier>(milBld1FlagPos, 1, milBld1, milBld1, 0));
    ;
    milBld1->gotWorker(newSld.getJobType(), newSld);
    newSld.actAtFirst();
    // Attacker faster -> Bld taken
    RTTR_EXEC_TILL(30, milBld1->getPlayer() == 0u);
    // New soldier should be wandering
    RTTR_EXEC_TILL(10, newSld.isWandering());
    // And door should be closed (latest after wandering soldier left flag)
    RTTR_EXEC_TILL(40, !milBld1->isDoorOpen());

    // 1. Attackers from this building
    // No home -> Wander
    BOOST_TEST_REQUIRE(!attackerFromPl0.getHomeBld());
    rescheduleWalkEvent(em, attackerFromPl0, 1);
    RTTR_EXEC_TILL(2, attackerFromPl0.isWandering());
    // 2. Aggressive defenders from this building
    // No further attack (unless already fighting) and wander
    // The attacker proceeds to the building and occupies it
    rescheduleWalkEvent(em, secAttacker, 1);
    rescheduleWalkEvent(em, aggDefender, 2);
    RTTR_SKIP_GFS(2);
    BOOST_TEST_REQUIRE(aggDefender.getAttacker() == nullptr);
    BOOST_TEST_REQUIRE(secAttacker.getHuntingDefender() == nullptr);
    BOOST_TEST_REQUIRE(aggDefender.isWandering());
    RTTR_EXEC_TILL(270, milBld1->getNumTroops() == 2u);
    // 3. Allied aggressor towards this bld
    // Abort attack and return home
    rescheduleWalkEvent(em, alliedAttacker, 1);
    RTTR_EXEC_TILL(1, alliedAttacker.getAttackedGoal() == nullptr);
    RTTR_EXEC_TILL(90, alliedBld->getNumTroops() == 2u);
    // 4. Hostile aggressor towards this bld
    // Continue attack and fight
    rescheduleWalkEvent(em, hostileAttacker, 1);
    BOOST_TEST_REQUIRE(hostileAttacker.getAttackedGoal() != nullptr);
    RTTR_EXEC_TILL(220, hostileAttacker.getPos() == milBld1FlagPos);
    RTTR_EXEC_TILL(50, world.getFigures(milBld1FlagPos).begin()->getGOT() == GO_Type::Fighting);
}

BOOST_FIXTURE_TEST_CASE(ConquerWithCarriersWalkingIn, AttackFixture<2>)
{
    // 1. Carrier with coin walking in the building
    // 2. Carrier with coin walking out of the building
    addSoldiers(milBld0Pos, 0, 6);
    addSoldiers(milBld1Pos, 1, Job::Private);
    const MapPoint milBld1FlagPos = milBld1->getFlagPos();

    curPlayer = 1;
    const MapPoint flagPos = world.makeMapPoint(milBld1FlagPos - Position(2, 0));
    this->BuildRoad(milBld1FlagPos, false, std::vector<Direction>(2, Direction::West));
    auto& flag = ensureNonNull(world.getSpecObj<noFlag>(flagPos));
    RoadSegment* rs = flag.getRoute(Direction::East);
    BOOST_TEST_REQUIRE(rs);
    auto& carrierIn =
      world.addFigure(flagPos, std::make_unique<nofCarrier>(CarrierType::Normal, flagPos, curPlayer, rs, &flag));
    auto& carrierOut =
      world.addFigure(flagPos, std::make_unique<nofCarrier>(CarrierType::Donkey, flagPos, curPlayer, rs, &flag));
    rs->setCarrier(0, &carrierIn);
    rs->setCarrier(1, &carrierOut);
    // Add 2 coins for the bld
    for(unsigned i = 0; i < 2; i++)
    {
        auto coin = std::make_unique<Ware>(GoodType::Coins, milBld1, &flag);
        coin->waitAtFlag(flag);
        coin->recalcRoute();
        flag.addWare(std::move(coin));
    }
    world.getPlayer(1).increaseInventoryWare(GoodType::Coins, 2);
    carrierIn.actAtFirst();
    carrierOut.actAtFirst();
    // Both picked up
    BOOST_TEST_REQUIRE(flag.getNumWares() == 0u);
    // Move carriers to flag
    for(unsigned i = 0; i < 2; i++)
    {
        rescheduleWalkEvent(em, carrierIn, 1);
        rescheduleWalkEvent(em, carrierOut, 1);
        RTTR_SKIP_GFS(1);
    }
    // And pause them
    rescheduleWalkEvent(em, carrierIn, 10000);
    // After the out-walking was in
    rescheduleWalkEvent(em, carrierOut, 1);
    RTTR_SKIP_GFS(1);
    rescheduleWalkEvent(em, carrierOut, 10000);
    BOOST_TEST_REQUIRE(carrierIn.getCurMoveDir() == Direction::NorthWest);
    BOOST_TEST_REQUIRE(carrierOut.getCurMoveDir() == Direction::SouthEast);

    // Add another for later
    MapPoint flagPosE = world.makeMapPoint(milBld1FlagPos + Position(2, 0));
    this->BuildRoad(milBld1FlagPos, false, std::vector<Direction>(2, Direction::East));
    auto& flagE = ensureNonNull(world.getSpecObj<noFlag>(flagPosE));
    RoadSegment& rsE = ensureNonNull(flagE.getRoute(Direction::West));
    auto& carrierInE =
      world.addFigure(flagPosE, std::make_unique<nofCarrier>(CarrierType::Normal, flagPosE, curPlayer, &rsE, &flagE));
    rsE.setCarrier(0, &carrierInE);
    // He also gets 1 coin
    auto coin = std::make_unique<Ware>(GoodType::Coins, milBld1, &flagE);
    coin->waitAtFlag(flagE);
    coin->recalcRoute();
    flagE.addWare(std::move(coin));
    world.getPlayer(1).increaseInventoryWare(GoodType::Coins, 1);
    carrierInE.actAtFirst();
    // Picked up
    BOOST_TEST_REQUIRE(flagE.getNumWares() == 0u);
    // And pause him
    rescheduleWalkEvent(em, carrierInE, 10000);

    curPlayer = 0;
    this->Attack(milBld1Pos, 1, true);
    BOOST_TEST_REQUIRE(milBld0->getLeavingFigures().size() == 1u);
    auto& attacker = dynamic_cast<nofAttacker&>(milBld0->getLeavingFigures().front());
    // Move him directly out
    RTTR_EXEC_TILL(70, milBld0->getLeavingFigures().empty()); //-V807
    moveObjTo(world, attacker, milBld1FlagPos);
    RTTR_EXEC_TILL(20, attacker.getPos() == milBld1FlagPos);
    // Carriers on pos or to pos get send away as soon as soldier arrives
    rescheduleWalkEvent(em, carrierIn, 1);
    rescheduleWalkEvent(em, carrierOut, 1);
    RTTR_SKIP_GFS(1);
    BOOST_TEST_REQUIRE(carrierIn.isWandering());
    BOOST_TEST_REQUIRE(carrierOut.isWandering());

    // Let east carrier walk
    rescheduleWalkEvent(em, carrierInE, 1);

    // Start fight
    const auto flagFigs = world.getFigures(milBld1FlagPos);
    RTTR_EXEC_TILL(50, flagFigs.size() == 1u && flagFigs.front().getGOT() == GO_Type::Fighting);
    // East carrier gets blocked
    BOOST_TEST_REQUIRE(!carrierInE.isMoving());

    // Door closed latest after other carriers are gone
    RTTR_EXEC_TILL(20, !milBld1->isDoorOpen());

    // Speed up fight by reducing defenders HP to 1
    auto* defender = const_cast<nofDefender*>(milBld1->getDefender());
    while(defender->getHitpoints() > 1u)
        defender->takeHit();
    RTTR_EXEC_TILL(500, milBld1->getDefender() == nullptr);
    // Defender defeated. Attacker moving in.
    BOOST_TEST_REQUIRE(attacker.isMoving());
    BOOST_TEST_REQUIRE(attacker.getCurMoveDir() == Direction::NorthWest);

    // Door opened
    BOOST_TEST_REQUIRE(milBld1->isDoorOpen());
    // Blocked carrier can walk again
    BOOST_TEST_REQUIRE(carrierInE.isMoving());
    // Capture
    RTTR_EXEC_TILL(20, milBld1->getPlayer() == 0u);
    // East Carrier should be wandering at some point
    RTTR_EXEC_TILL(20, carrierInE.isWandering());
    // Only 1 coin in the bld
    BOOST_TEST_REQUIRE(milBld1->getNumCoins() == 1u);
    // Door closed after carrier left it
    RTTR_EXEC_TILL(40, !milBld1->isDoorOpen());
}

BOOST_FIXTURE_TEST_CASE(FlagBecomesUnreachableForWaitingAttacker, AttackFixture<2>)
{
    // Setup:
    //   - Fight at building flag which is only reachable via road over water
    //   - 2nd attacker waits on other side of water, 3rd on same side
    //   - Road gets removed -> 2nd attacker cannot reach flag anymore
    //   - After fight ended 2nd attacker should go home and 3rd should be chosen to fight
    // Using 3 attackers to verify the 2nd one is skipped but remaining ones are considered
    auto* attackerBld = milBld1;
    auto* defenderBld = milBld0;
    auto plAttacker = 1;
    auto plDefender = 0;
    // Attack from right to left
    if(milBld0Pos.x < milBld1Pos.x)
    {
        using std::swap;
        swap(attackerBld, defenderBld);
        swap(plAttacker, plDefender);
    }
    const auto attackerBldPos = attackerBld->getPos();
    const auto defenderBldPos = defenderBld->getPos();
    addSoldiers(defenderBldPos, 0, 1);
    addSoldiers(attackerBldPos, 6, Job::Private);
    // Make defender building flag unreachable
    const auto terrain = this->world.getDescription().terrain.find(
      [](const TerrainDesc& t) { return !t.is(ETerrain::Walkable) && t.getBQ() != TerrainBQ::Danger; });
    const auto flagPos = defenderBld->getFlagPos();
    // At right there is 1 node of water, so not passable but soldiers can stand on other side
    /* "B"=Attacked building, "F"=Flag, "L"=Land terrain, Rest is water

      /\  /\  /\LL/
     /__\/__\/LL\/
        B
     \  /\  /\LL/\
      \/LL\/__\/LL\
           F       F
      /\  /\  /\LL/
     /__\/__\/LL\/
     --> 1 triangle right of left flag is not walkable and hence F<->F not passable without road
    */
    for(const auto dx : helpers::range(-2, 1))
    {
        for(const auto dy : helpers::range(-2, 3))
        {
            MapPoint pt = world.makeMapPoint(flagPos + Position(dx, dy));
            auto& node = world.getNodeWriteable(pt);
            node.t1 = node.t2 = terrain;
        }
    }
    {
        // RSU triangle of node right of B so flag is not reachable from above
        MapPoint pt = world.getNeighbour(defenderBldPos, Direction::East);
        auto& node = world.getNodeWriteable(pt);
        node.t1 = terrain;
    }
    {
        // RSU triangle of building so it is reachable from flag
        auto& node = world.getNodeWriteable(defenderBldPos);
        node.t1 = getLandTerrain(world.getDescription()); // RSU triangle of node right of B
    }
    GameWorldViewer gwv(plAttacker, world);
    BOOST_TEST_REQUIRE(gwv.getNumSoldiersForAttack(defenderBldPos) == 0u);
    // Road over water makes it reachable
    curPlayer = plDefender;
    const MapPoint roadFlagPos = world.makeMapPoint(flagPos + Position(2, 0));
    this->BuildRoad(flagPos, false, std::vector<Direction>(2, Direction::East));
    auto* flag = world.getSpecObj<noFlag>(roadFlagPos);
    BOOST_TEST_REQUIRE(flag);

    BOOST_TEST_REQUIRE(gwv.getNumSoldiersForAttack(defenderBldPos) >= 3u);

    curPlayer = plAttacker;
    this->Attack(defenderBldPos, 3, true);
    BOOST_TEST_REQUIRE(attackerBld->getLeavingFigures().size() == 3);
    auto& attacker1 = dynamic_cast<nofAttacker&>(attackerBld->getLeavingFigures().front());
    auto& attacker2 = dynamic_cast<nofAttacker&>(*++attackerBld->getLeavingFigures().begin());
    auto& attacker3 = dynamic_cast<nofAttacker&>(*++(++attackerBld->getLeavingFigures().begin()));
    // Let all exit and move almost to goal
    RTTR_EXEC_TILL(200, attackerBld->getLeavingFigures().empty());
    moveObjTo(world, attacker1, world.getNeighbour(flagPos, Direction::East));
    moveObjTo(world, attacker2, world.getNeighbour(roadFlagPos, Direction::East));
    moveObjTo(world, attacker3, world.getNeighbour(flagPos, Direction::West));
    RTTR_EXEC_TILL(200, !defenderBld->getLeavingFigures().empty());
    auto& defender = dynamic_cast<nofDefender&>(defenderBld->getLeavingFigures().front());
    RTTR_EXEC_TILL(200, defender.isFightingAtFlag());
    RTTR_EXEC_TILL(10, attacker2.isAttackerReady() && attacker3.isAttackerReady());
    BOOST_TEST_REQUIRE(attacker1.getState() == SoldierState::AttackingFightingVsDefender);

    // Setup done, now remove the road and verify the goal is no longer attackable
    curPlayer = plDefender;
    DestroyRoad(flagPos, Direction::East);
    BOOST_TEST_REQUIRE(gwv.getNumSoldiersForAttack(defenderBldPos) == 0u);
    // Fight ended
    RTTR_EXEC_TILL(500, !defender.isFightingAtFlag());
    // Attacker2 should go home
    BOOST_TEST(attacker2.getState() == SoldierState::WalkingHome);
    BOOST_TEST(!attacker2.getAttackedGoal());
    BOOST_TEST(!helpers::contains(defenderBld->getAggressors(), &attacker2));
    BOOST_TEST(attacker2.isMoving()); // Not stuck
    // Attacker3 should go and fight now
    BOOST_TEST(!attacker3.isAttackerReady());
    BOOST_TEST(attacker3.getAttackedGoal());
    BOOST_TEST(attacker3.isMoving()); // Not stuck
    RTTR_EXEC_TILL(100, defender.isFightingAtFlag());
    BOOST_TEST(attacker3.getState() == SoldierState::AttackingFightingVsDefender);
}

enum AttackedBldType
{
    Hq,
    MillitaryBld
};

#ifndef __INTELLISENSE__
BOOST_DATA_TEST_CASE_F(AttackFixture<2>, HandleLeavingAggDefendersOnAttack,
                       dataset::make(std::array{AttackedBldType::Hq, AttackedBldType::MillitaryBld})
                         * dataset::make(std::array{false, true}),
                       test_case, useAllDefenders)
#else
struct HandleLeavingAggDefendersOnAttack : AttackFixture<2>
{
    void test(AttackedBldType test_case, bool useAllDefenders);
};
void HandleLeavingAggDefendersOnAttack::test(AttackedBldType test_case, bool useAllDefenders)
#endif
{
    // When defender is called, leaving aggressive defenders should be canceled.
    // When there is no other soldier left to send as defender, an agressive defenders should be used as defender
    // instead.
    //
    // Reproduce issue #1907:
    // Accounting of soldiers that were about to leave but got canceled was wrong
    // Situation (happens with a single attacker already):
    //  - Attack on "military storehouse", i.e. HQ.
    //  - HQ sends aggressive defender about to leave.
    //  - Attacker reaches flag and requests defender which cancels any agressive defenders
    //  - Readding that defender cause error in accounting
    //
    // Handling for "real" military buildings and attackable warehouses (HQ) is different, so test all combinations
    std::array<nofAttacker*, 5> attackers{};
    addSoldiers(milBld0Pos, attackers.size() + 1, Job::General);
    auto& attackedHq = ensureNonNull(world.getSpecObj<nobBaseWarehouse>(hqPos[1]));
    auto& attackedBld = (test_case == AttackedBldType::Hq) ? attackedHq : static_cast<nobBaseMilitary&>(*milBld1);
    const auto attackedPlayer = attackedBld.getPlayer();
    const auto numGenerals = rttr::test::randomValue<unsigned>(1, attackers.size() / 2);
    const auto numPrivates = rttr::test::randomValue<unsigned>(1, attackers.size() / 2);
    if(test_case == AttackedBldType::Hq)
    {
        // Make sure they can be sent out (aggressive defenders are not taken from the reserve)
        attackedHq.setRealReserve(getSoldierRank(Job::General), 0);
        attackedHq.setRealReserve(getSoldierRank(Job::Private), 0);
        auto startSoldiers = PeopleCounts::make(Job::General, numGenerals);
        startSoldiers[Job::Private] = numPrivates;
        attackedHq.addToInventory(startSoldiers, true);
    } else
    {
        addSoldiers(attackedBld.getPos(), numGenerals, Job::General);
        addSoldiers(attackedBld.getPos(), numPrivates, Job::Private);
    }
    // Total soldiers should not change because none die in this test case
    // This ensures they are not double-accounted during conversion from agressive defender to defender
    // and not missed when removed from the leave queue
    const auto totalSoldiersBefore = getTotalSoldiers(attackedPlayer);
    const auto soldiersBefore = getNumSoldiers(attackedHq);
    const auto numAttackers = useAllDefenders ? attackers.size() : 1u;
    this->Attack(attackedBld.getPos(), numAttackers, true);
    BOOST_TEST_REQUIRE(milBld0->getLeavingFigures().size() == numAttackers);
    {
        int i = 0;
        for(auto& fig : milBld0->getLeavingFigures())
            attackers[i++] = &dynamic_cast<nofAttacker&>(fig);
    }
    // Let one out then call sendAggressiveDefender for all attackers.
    // This is technically wrong but we want to ensure they come from the target bld only and are still in the leave
    // queue when the first attacker calls the defender
    RTTR_EXEC_TILL(50, milBld0->getLeavingFigures().size() < numAttackers);
    for(unsigned i = 0; i < numAttackers; i++)
    {
        auto* attacker = attackers[i];
        if(attacker && !attacker->getHuntingDefender())
        {
            auto* aggDefender = attackedBld.sendAggressiveDefender(*attacker);
            if(aggDefender)
                attacker->letsFight(*aggDefender);
        }
    }
    if(useAllDefenders)
    {
        if(test_case == AttackedBldType::Hq) // HQs will be empty as all are leaving
            BOOST_TEST_REQUIRE(attackedBld.getLeavingFigures().size() == numGenerals + numPrivates);
        else // Military buildings keep at least one
            BOOST_TEST_REQUIRE(attackedBld.getLeavingFigures().size() == numGenerals + numPrivates - 1u);
    } else
        BOOST_TEST_REQUIRE(!attackedBld.getLeavingFigures().empty());
    if(test_case == AttackedBldType::Hq)
    {
        auto curSoldiers = getNumSoldiers(attackedHq);
        // Still there visually
        BOOST_TEST(curSoldiers[visual] == soldiersBefore[visual]);
        // but not real
        BOOST_TEST(curSoldiers[real] != soldiersBefore[real]);
        if(useAllDefenders)
            BOOST_TEST(calcSum(curSoldiers[real]) == 0u);
        else
            BOOST_TEST(calcSum(curSoldiers[real]) == calcSum(soldiersBefore[real]) - numAttackers);
    }
    BOOST_TEST(getTotalSoldiers(attackedPlayer) == totalSoldiersBefore);
    // Move attacker to flag to trigger defender request
    auto& attacker = *attackers.front();
    moveObjTo(world, attacker, attackedBld.getFlagPos());
    rescheduleWalkEvent(em, attacker, 1);
    RTTR_SKIP_GFS(1);
    BOOST_TEST_REQUIRE(attacker.getState() == SoldierState::AttackingWaitingForDefender);
    // All agressive defenders are cancelled
    for(const auto& fig : attackedBld.getLeavingFigures())
        BOOST_TEST(fig.getGOT() != GO_Type::NofAggressivedefender);
    BOOST_TEST_REQUIRE(!attackedBld.getLeavingFigures().empty());
    // Defender is coming
    BOOST_TEST_REQUIRE(attackedBld.getLeavingFigures().front().getGOT() == GO_Type::NofDefender);
    const auto defenderRank = getSoldierRank(attackedBld.getLeavingFigures().front().getJobType());
    if(test_case == AttackedBldType::Hq)
    {
        const auto curSoldiers = getNumSoldiers(attackedHq);
        // Only defender is missing, only from real count (until actually left)
        BOOST_TEST(calcSum(curSoldiers[real]) == calcSum(soldiersBefore[real]) - 1);
        BOOST_TEST(curSoldiers[visual] == soldiersBefore[visual]);
    }
    BOOST_TEST(getTotalSoldiers(attackedPlayer) == totalSoldiersBefore);
    RTTR_EXEC_TILL(70, attackedBld.getLeavingFigures().empty());
    if(test_case == AttackedBldType::Hq)
    {
        auto soldiersExpected = soldiersBefore;
        soldiersExpected[visual][defenderRank] = --soldiersExpected[real][defenderRank]; // Defender is out
        BOOST_TEST(getNumSoldiers(attackedHq) == soldiersExpected);
    }
    BOOST_TEST(getTotalSoldiers(attackedPlayer) == totalSoldiersBefore);
}

BOOST_FIXTURE_TEST_CASE(LeavingSoldierUsedAsDefender, AttackFixture<2>)
{
    // When the only soldier in a HQ is currently leaving (to a building) it is used as defender
    // Counts need to be correct at all times
    addSoldiers(milBld0Pos, 2, Job::General);
    auto& attackedHq = ensureNonNull(world.getSpecObj<nobBaseWarehouse>(hqPos[1]));
    const auto soldierJob = rttr::test::randomElement(SOLDIER_JOBS);
    attackedHq.setRealReserve(getSoldierRank(soldierJob), 0);
    attackedHq.addToInventory(PeopleCounts::make(soldierJob, 1), true);
    const auto attackedPlayer = attackedHq.getPlayer();
    const auto totalSoldiersBefore = getTotalSoldiers(attackedPlayer);
    const auto soldiersBefore = getNumSoldiers(attackedHq);

    this->Attack(attackedHq.getPos(), 1, true);
    BOOST_TEST_REQUIRE(milBld0->getLeavingFigures().size() == 1);
    auto& attacker = dynamic_cast<nofAttacker&>(milBld0->getLeavingFigures().front());
    RTTR_EXEC_TILL(50, milBld0->getLeavingFigures().empty());
    curPlayer = attackedPlayer;
    buildRoadForBlds(milBld1Pos, hqPos[1]);
    curPlayer = 0;
    BOOST_TEST_REQUIRE(!attackedHq.getLeavingFigures().empty());
    {
        const auto& leavingSoldier = dynamic_cast<nofSoldier&>(attackedHq.getLeavingFigures().front());
        BOOST_TEST(leavingSoldier.getGoal() == milBld1);
    }
    BOOST_TEST(getTotalSoldiers(attackedPlayer) == totalSoldiersBefore);
    const auto soldiersNow = getNumSoldiers(attackedHq);
    BOOST_TEST(soldiersNow[visual] == soldiersBefore[visual]);
    BOOST_TEST(calcSum(soldiersNow[real]) == 0u);
    moveObjTo(world, attacker, attackedHq.getFlagPos());
    rescheduleWalkEvent(em, attacker, 1);
    RTTR_SKIP_GFS(1);
    BOOST_TEST_REQUIRE(attackedHq.getLeavingFigures().size() == 1);
    BOOST_TEST(attackedHq.getLeavingFigures().front().getGOT() == GO_Type::NofDefender);
    BOOST_TEST(getTotalSoldiers(attackedPlayer) == totalSoldiersBefore);
    BOOST_TEST(getNumSoldiers(attackedHq) == soldiersNow);
    RTTR_EXEC_TILL(70, attackedHq.getLeavingFigures().empty());
    BOOST_TEST(getTotalSoldiers(attackedPlayer) == totalSoldiersBefore);
    const auto soldiersAfter = getNumSoldiers(attackedHq);
    BOOST_TEST(calcSum(soldiersAfter[visual]) == 0u);
    BOOST_TEST(calcSum(soldiersAfter[real]) == 0u);
}

using DestroyRoadsOnConquerFixture = AttackFixture<2, 24>;
BOOST_FIXTURE_TEST_CASE(DestroyRoadsOnConquer, DestroyRoadsOnConquerFixture)
{
    MapPoint leftBldPos = world.makeMapPoint(milBld1Pos + Position(-5, 2));
    noBuilding* leftBld =
      BuildingFactory::createBuilding(world, BuildingType::Barracks, leftBldPos, 1, Nation::Babylonians);
    BOOST_TEST_REQUIRE(leftBld);
    MapPoint rightBldPos = world.makeMapPoint(milBld1Pos + Position(5, 2));
    noBuilding* rightBld =
      BuildingFactory::createBuilding(world, BuildingType::Barracks, rightBldPos, 1, Nation::Babylonians);
    BOOST_TEST_REQUIRE(rightBld);

    addSoldiers(leftBldPos, 1, Job::Private);
    addSoldiers(rightBldPos, 1, Job::Private);
    addSoldiers(milBld1Pos, 1, Job::Private);
    addSoldiers(milBld0Pos, 6, Job::General);

    curPlayer = 1;
    // Build 2 roads to attack building to test that destroying them does not cause a bug
    // Bld -> Flag
    buildRoadForBlds(milBld1Pos, world.makeMapPoint(milBld1Pos + Position(2, 0)));
    // Flag -> Bld
    const MapPoint flagPt = world.makeMapPoint(milBld1Pos - Position(2, 0));
    this->SetFlag(flagPt);
    buildRoadForBlds(world.getNeighbour(flagPt, Direction::NorthWest), milBld1Pos);
    // Build a long road connecting left&right w/o any flag in between (See #863)
    buildRoadForBlds(leftBldPos, rightBldPos);
    BOOST_TEST_REQUIRE(leftBld->getFlag()->getRoute(Direction::East)->getLength() >= 10u);

    curPlayer = 0;
    this->Attack(milBld1Pos, 5, true);
    RTTR_EXEC_TILL(2000, milBld1->getPlayer() == curPlayer);
    std::array<MapPoint, 3> bldPts = {{leftBldPos, rightBldPos, milBld1Pos}};
    for(const MapPoint& bldPt : bldPts)
    {
        MapPoint flagPt = world.getNeighbour(bldPt, Direction::SouthEast);
        for(Direction i : helpers::EnumRange<Direction>{})
        {
            // No routes except the main road
            if(i != Direction::NorthWest)
            {
                BOOST_TEST_REQUIRE(!world.getSpecObj<noRoadNode>(flagPt)->getRoute(i));
                BOOST_TEST_REQUIRE(world.getPointRoad(flagPt, i) == PointRoad::None);
            }
        }
    }
}

// Prepare a free fight between an attacker from the left and a (aggressive) defender from the right
// After preparation both are just about to meet
struct FreeFightFixture : AttackFixture<2>
{
    nobMilitary &attackerBld, &attackedBld;
    const MapPoint attackedBldPos, fightSpot;
    nofAttacker* attacker_;
    nofAggressiveDefender* defender_;

    FreeFightFixture()
        : attackerBld(*milBld0), attackedBld(*milBld1), attackedBldPos(milBld1Pos),
          fightSpot(attackedBldPos - MapPoint(3, 0))
    {
        addSoldiers(milBld0Pos, 6, Job::Private);
        addSoldiers(milBld1Pos, 6, Job::Private);
        this->Attack(attackedBldPos, 1, true);
        auto& attacker = dynamic_cast<nofAttacker&>(attackerBld.getLeavingFigures().front());
        attacker_ = &attacker;
        RTTR_EXEC_TILL(70, attackerBld.getLeavingFigures().empty()); //-V807

        nofAggressiveDefender& defender = ensureNonNull(attackedBld.sendAggressiveDefender(attacker));
        defender_ = &defender;
        RTTR_EXEC_TILL(70, attackedBld.getLeavingFigures().empty()); //-V807
        attacker.letsFight(defender);

        // In a distance of 2 they find each other and meet half way
        const MapPoint fightSpot = attackedBldPos - MapPoint(3, 0);
        // attacker building is left -> place attacker left of fight spot
        BOOST_TEST_REQUIRE(attackerBld.getPos().x < attackedBldPos.x);
        moveObjTo(world, attacker, fightSpot - MapPoint(1, 0));
        moveObjTo(world, defender, fightSpot + MapPoint(1, 0));
        rescheduleWalkEvent(em, attacker, 1);
        RTTR_SKIP_GFS(1);
        BOOST_TEST_REQUIRE(defender.getState() == SoldierState::MeetEnemy);
        BOOST_TEST_REQUIRE(attacker.getState() == SoldierState::MeetEnemy);
    }

    /// Add blocking terrain around building of defender starting left of the specified position
    void blockDefenderBuilding(const MapCoord x)
    {
        const auto terrain =
          this->world.getDescription().terrain.find([](const TerrainDesc& t) { return !t.is(ETerrain::Walkable); });
        for(const auto y : helpers::range(this->world.getHeight()))
        {
            auto& node = this->world.getNodeWriteable(MapPoint(x, y));
            auto& rightNode = this->world.getNodeWriteable(MapPoint(attackedBldPos.x + 1, y));
            node.t1 = node.t2 = rightNode.t1 = rightNode.t2 = terrain;
        }
        const auto yTop = this->world.makeMapPoint(Position(attackedBldPos.x, attackedBldPos.y - 3)).y;
        const auto yBottom = this->world.makeMapPoint(Position(attackedBldPos.x, attackedBldPos.y + 3)).y;
        for(const auto x : helpers::range(x, attackedBldPos.x))
        {
            auto& topNode = this->world.getNodeWriteable(MapPoint(x, yTop));
            auto& bottomNode = this->world.getNodeWriteable(MapPoint(x, yBottom));
            topNode.t1 = topNode.t2 = bottomNode.t1 = bottomNode.t2 = terrain;
        }
    }
};

BOOST_FIXTURE_TEST_CASE(Attacker_Returns_When_AgressiveDefender_Aborts, FreeFightFixture)
{
    /* Setup:
     * Attacker waits for the defender.
     * The spot becomes unreachable for the defender and the attacked building becomes unreachable for the attacker.
     * In issue #1668 this happened due to a destroyed road.
     * Once the defender notices this, both should be walking home.
     */
    auto& attacker = ensureNonNull(attacker_);
    auto& defender = ensureNonNull(defender_);

    moveObjTo(world, attacker, fightSpot);
    rescheduleWalkEvent(em, attacker, 1);
    RTTR_SKIP_GFS(1);
    BOOST_TEST_REQUIRE(defender.getState() == SoldierState::MeetEnemy);
    BOOST_TEST_REQUIRE(attacker.getState() == SoldierState::WaitingForFight);
    // The next part of the test assumes they use this fight spot.
    // This doesn't need to stay true which only needs adjustments to the test.
    BOOST_TEST_REQUIRE(attacker.getPos() == fightSpot);

    blockDefenderBuilding(fightSpot.x);
    // -> defender can't reach attacker
    // -> attacker can't reach attacked building
    // Pending fight should get aborted and both go back home:
    rescheduleWalkEvent(em, defender, 1);
    RTTR_SKIP_GFS(1);
    BOOST_TEST(defender.isMoving());
    BOOST_TEST(attacker.isMoving());
    BOOST_TEST(defender.getState() != SoldierState::MeetEnemy);
    BOOST_TEST(attacker.getState() != SoldierState::WaitingForFight);

    BOOST_TEST_REQUIRE(attackedBld.getNumTroops() == 5u); // Sanity check
    RTTR_EXEC_TILL(100, attackedBld.getNumTroops() == 6u);
    BOOST_TEST_REQUIRE(attackerBld.getNumTroops() == 5u); // Sanity check
    RTTR_EXEC_TILL(100, attackerBld.getNumTroops() == 6u);
}

BOOST_FIXTURE_TEST_CASE(AgressiveDefender_Returns_When_Attacker_Aborts, FreeFightFixture)
{
    /* Setup:
     * Defender waits for attacker.
     * Something happens that the attacker can't reach the defender, e.g. a destroyed road
     * Once the attacker notices this, both should be walking home.
     */
    auto& attacker = ensureNonNull(attacker_);
    auto& defender = ensureNonNull(defender_);

    moveObjTo(world, defender, fightSpot);
    rescheduleWalkEvent(em, defender, 1);
    RTTR_SKIP_GFS(1);
    BOOST_TEST_REQUIRE(defender.getState() == SoldierState::WaitingForFight);
    BOOST_TEST_REQUIRE(attacker.getState() == SoldierState::MeetEnemy);
    // The next part of the test assumes they use this fight spot.
    // This doesn't need to stay true which only needs adjustments to the test.
    BOOST_TEST_REQUIRE(defender.getPos() == fightSpot);

    blockDefenderBuilding(fightSpot.x - 1);
    // -> attacker can't reach defender or attacked building
    // Pending fight should get aborted and both go back home:
    moveObjTo(world, attacker, fightSpot - MapPoint(1, 0));
    rescheduleWalkEvent(em, attacker, 1);
    RTTR_SKIP_GFS(1);
    BOOST_TEST(defender.isMoving());
    BOOST_TEST(attacker.isMoving());
    BOOST_TEST(defender.getState() != SoldierState::WaitingForFight);
    BOOST_TEST(attacker.getState() != SoldierState::MeetEnemy);

    BOOST_TEST_REQUIRE(attackedBld.getNumTroops() == 5u); // Sanity check
    RTTR_EXEC_TILL(100, attackedBld.getNumTroops() == 6u);
    BOOST_TEST_REQUIRE(attackerBld.getNumTroops() == 5u); // Sanity check
    RTTR_EXEC_TILL(100, attackerBld.getNumTroops() == 6u);
}

BOOST_AUTO_TEST_SUITE_END()
