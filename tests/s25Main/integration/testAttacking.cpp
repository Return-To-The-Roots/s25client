// Copyright (c) 2016 - 2017 Settlers Freaks (sf-team at siedler25.org)
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

#include "GameEvent.h"
#include "Ware.h"
#include "buildings/nobBaseWarehouse.h"
#include "buildings/nobMilitary.h"
#include "factories/BuildingFactory.h"
#include "figures/nofAggressiveDefender.h"
#include "figures/nofAttacker.h"
#include "figures/nofCarrier.h"
#include "figures/nofDefender.h"
#include "figures/nofPassiveSoldier.h"
#include "helpers/containerUtils.h"
#include "pathfinding/FindPathForRoad.h"
#include "worldFixtures/WorldWithGCExecution.h"
#include "worldFixtures/initGameRNG.hpp"
#include "world/GameWorldViewer.h"
#include "nodeObjs/noFlag.h"
#include "gameTypes/GameTypesOutput.h"
#include "gameData/SettingTypeConv.h"
#include <boost/test/unit_test.hpp>
#include <iostream>

BOOST_AUTO_TEST_SUITE(AttackSuite)

namespace {
struct AttackDefaults
{
    static constexpr unsigned width = 20;
    static constexpr unsigned height = 12;
};

/// Reschedule the walk event of the obj to be executed in numGFs GFs
void rescheduleWalkEvent(TestEventManager& em, noMovable& obj, unsigned numGFs)
{
    std::vector<const GameEvent*> evts = em.GetObjEvents(obj);
    for(const GameEvent* ev : evts)
    {
        if(ev->id == 0)
        {
            em.RescheduleEvent(ev, em.GetCurrentGF() + numGFs);
            return;
        }
    }
    BOOST_REQUIRE(false);
}

/// Move the object next to the given point. The next walk event will make it reach that point
// LCOV_EXCL_START
void moveObjTo(GameWorldBase& world, noFigure& obj, const MapPoint& pos)
{
    if(helpers::contains(world.GetFigures(obj.GetPos()), &obj))
        world.RemoveFigure(obj.GetPos(), &obj);
    else if(helpers::contains(world.GetFigures(world.GetNeighbour(obj.GetPos(), obj.GetCurMoveDir() + 3u)), &obj))
        world.RemoveFigure(world.GetNeighbour(obj.GetPos(), obj.GetCurMoveDir() + 3u), &obj);
    obj.SetPos(world.GetNeighbour(pos, Direction::WEST));
    world.AddFigure(obj.GetPos(), &obj);
    if(obj.IsMoving())
        obj.FaceDir(Direction::EAST);
    else
        obj.StartWalking(Direction::EAST);
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
        Inventory goods;
        goods.Add(JOB_GENERAL, 3);
        for(unsigned i = 0; i < T_numPlayers; i++)
        {
            curPlayer = i;
            hqPos[i] = world.GetPlayer(i).GetHQPos();
            MakeVisible(hqPos[i]);
            world.template GetSpecObj<nobBaseWarehouse>(hqPos[i])->AddGoods(goods, true);
            this->ChangeMilitary(MILITARY_SETTINGS_SCALE);
        }
        curPlayer = 0;
    }

    void MakeVisible(const MapPoint& pt)
    {
        for(unsigned i = 0; i < T_numPlayers; i++)
            world.MakeVisibleAroundPoint(pt, 1, i);
    }

    /// Constructs a road connecting 2 buildings and checks for success
    void BuildRoadForBlds(const MapPoint bldPosFrom, const MapPoint bldPosTo)
    {
        const MapPoint start = world.GetNeighbour(bldPosFrom, Direction::SOUTHEAST);
        const MapPoint end = world.GetNeighbour(bldPosTo, Direction::SOUTHEAST);
        std::vector<Direction> road = FindPathForRoad(world, start, end, false);
        BOOST_REQUIRE(!road.empty());
        this->BuildRoad(start, false, road);
        BOOST_REQUIRE_EQUAL(world.GetPointRoad(start, road.front()), PointRoad::Normal);
    }

    void AddSoldiersWithRank(MapPoint bldPos, unsigned numSoldiers, unsigned rank)
    {
        BOOST_REQUIRE_LE(rank, world.GetGGS().GetMaxMilitaryRank());
        auto* bld = world.template GetSpecObj<nobMilitary>(bldPos);
        BOOST_REQUIRE(bld);
        const unsigned oldNumSoldiers = bld->GetNumTroops();
        for(unsigned i = 0; i < numSoldiers; i++)
        {
            auto* soldier = new nofPassiveSoldier(bldPos, bld->GetPlayer(), bld, bld, rank);
            world.GetPlayer(bld->GetPlayer()).IncreaseInventoryJob(soldier->GetJobType(), 1);
            world.AddFigure(bldPos, soldier);
            // Let him "walk" to goal -> Already reached -> Added and all internal states set correctly
            soldier->WalkToGoal();
            BOOST_REQUIRE(soldier->HasNoGoal());
        }
        BOOST_REQUIRE_EQUAL(bld->GetNumTroops(), oldNumSoldiers + numSoldiers);
    }

    void AddSoldiers(MapPoint bldPos, unsigned numWeak, unsigned numStrong)
    {
        AddSoldiersWithRank(bldPos, numWeak, 0);
        AddSoldiersWithRank(bldPos, numStrong, 4);
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
        // Assert player positions: 0: Top-Left, 1: Top-Right, 2: Bottom-Middle
        BOOST_REQUIRE_LT(hqPos[0].x, hqPos[1].x);
        BOOST_REQUIRE_LT(hqPos[0].y, hqPos[2].y);
        BOOST_REQUIRE_LT(hqPos[1].y, hqPos[2].y);

        // Build some military buildings
        milBld0Pos = hqPos[0] + MapPoint(7, 0);
        BOOST_REQUIRE_EQUAL(world.GetBQ(milBld0Pos, 0), BQ_CASTLE);
        milBld0 = dynamic_cast<nobMilitary*>(BuildingFactory::CreateBuilding(world, BLD_WATCHTOWER, milBld0Pos, 0, NAT_BABYLONIANS));
        BOOST_REQUIRE(milBld0);

        milBld1NearPos = hqPos[1] - MapPoint(7, 0);
        BOOST_REQUIRE_EQUAL(world.GetBQ(milBld1NearPos, 1), BQ_CASTLE);
        milBld1Near = dynamic_cast<nobMilitary*>(BuildingFactory::CreateBuilding(world, BLD_WATCHTOWER, milBld1NearPos, 1, NAT_ROMANS));
        BOOST_REQUIRE(milBld1Near);

        milBld1FarPos = hqPos[1] + MapPoint(3, 1);
        BOOST_REQUIRE_EQUAL(world.GetBQ(milBld1FarPos, 1), BQ_CASTLE);
        milBld1Far = dynamic_cast<nobMilitary*>(BuildingFactory::CreateBuilding(world, BLD_WATCHTOWER, milBld1FarPos, 1, NAT_ROMANS));
        BOOST_REQUIRE(milBld1Far);
        MakeVisible(milBld0Pos);
        MakeVisible(milBld1NearPos);
        MakeVisible(milBld1FarPos);
    }

    void SetCurPlayer(unsigned playerIdx)
    {
        curPlayer = playerIdx;
        gwv.ChangePlayer(playerIdx, false);
    }
};

template<unsigned T_numPlayers = 2, unsigned T_width = AttackDefaults::width, unsigned T_height = AttackDefaults::height>
struct AttackFixture : public AttackFixtureBase<T_numPlayers, T_width, T_height>
{
    using Parent = AttackFixtureBase<T_numPlayers, T_width, T_height>;
    using Parent::curPlayer;
    using Parent::hqPos;
    using Parent::MakeVisible;
    using Parent::world;

    /// Tested positions for military buildings
    MapPoint milBld0Pos, milBld1Pos;
    /// Military buildings of players 0 and 1
    nobMilitary *milBld0, *milBld1;

    AttackFixture()
    {
        // Build some military buildings far away enough for holding some area outside HQ
        milBld0Pos = world.MakeMapPoint(hqPos[0] + Position(0, 6));
        BOOST_REQUIRE_EQUAL(world.GetBQ(milBld0Pos, 0), BQ_CASTLE);
        milBld0 = static_cast<nobMilitary*>(BuildingFactory::CreateBuilding(world, BLD_WATCHTOWER, milBld0Pos, 0, NAT_BABYLONIANS));
        BOOST_REQUIRE(milBld0);

        milBld1Pos = world.MakeMapPoint(hqPos[1] + Position(0, 6));
        BOOST_REQUIRE_EQUAL(world.GetBQ(milBld1Pos, 1), BQ_CASTLE);
        milBld1 = static_cast<nobMilitary*>(BuildingFactory::CreateBuilding(world, BLD_WATCHTOWER, milBld1Pos, 1, NAT_ROMANS));
        BOOST_REQUIRE(milBld1);
        MakeVisible(milBld0Pos);
        MakeVisible(milBld1Pos);
    }

    /// Assert that attacking the given building from attackSrc fails
    void TestFailingAttack(const GameWorldViewer& gwv, const MapPoint& bldPos, const nobMilitary& attackSrc, unsigned numSoldiersLeft = 6u)
    {
        BOOST_REQUIRE_EQUAL(attackSrc.GetNumTroops(), numSoldiersLeft);
        // No available soldiers
        BOOST_REQUIRE_EQUAL(gwv.GetNumSoldiersForAttack(bldPos), 0u);
        this->Attack(bldPos, 1, true);
        // Same left
        BOOST_REQUIRE_EQUAL(attackSrc.GetNumTroops(), numSoldiersLeft);
    }
};
} // namespace

BOOST_FIXTURE_TEST_CASE(NumSoldiersForAttack, NumSoldierTestFixture)
{
    initGameRNG();

    // Connect buildings
    curPlayer = 1;
    BuildRoadForBlds(hqPos[1], milBld1Near->GetPos());
    curPlayer = 0;
    BuildRoadForBlds(hqPos[0], milBld0->GetPos());
    // Let soldiers get into blds. (6 soldiers, 7 fields distance, 20GFs per field, 30GFs for leaving HQ)
    unsigned numGFs = 6 * (7 * 20 + 30);
    RTTR_SKIP_GFS(numGFs);
    // Don't wait for them just add
    AddSoldiers(milBld1Far->GetPos(), 6, 0);
    BOOST_REQUIRE_EQUAL(milBld1Near->GetNumTroops(), 6u);
    BOOST_REQUIRE_EQUAL(milBld1Far->GetNumTroops(), 6u);
    BOOST_REQUIRE_EQUAL(milBld0->GetNumTroops(), 6u);

    // Player 2 has no military blds -> Can't attack
    SetCurPlayer(2);
    BOOST_REQUIRE_EQUAL(gwv.GetNumSoldiersForAttack(hqPos[0]), 0u);
    BOOST_REQUIRE_EQUAL(gwv.GetNumSoldiersForAttack(hqPos[1]), 0u);
    BOOST_REQUIRE_EQUAL(gwv.GetNumSoldiersForAttack(milBld0->GetPos()), 0u);
    BOOST_REQUIRE_EQUAL(gwv.GetNumSoldiersForAttack(milBld1Near->GetPos()), 0u);
    BOOST_REQUIRE_EQUAL(gwv.GetNumSoldiersForAttack(milBld1Far->GetPos()), 0u);
    SetCurPlayer(1);
    // No self attack
    BOOST_REQUIRE_EQUAL(gwv.GetNumSoldiersForAttack(hqPos[1]), 0u);
    BOOST_REQUIRE_EQUAL(gwv.GetNumSoldiersForAttack(milBld1Near->GetPos()), 0u);
    BOOST_REQUIRE_EQUAL(gwv.GetNumSoldiersForAttack(milBld1Far->GetPos()), 0u);
    // Attack both others
    BOOST_REQUIRE_EQUAL(gwv.GetNumSoldiersForAttack(hqPos[0]), 5u);
    BOOST_REQUIRE_EQUAL(gwv.GetNumSoldiersForAttack(hqPos[2]), 5u);
    // This is in the extended range of the far bld -> 2 more (with current range)
    BOOST_REQUIRE_EQUAL(gwv.GetNumSoldiersForAttack(milBld0->GetPos()), 7u);
    SetCurPlayer(0);
    // No self attack
    BOOST_REQUIRE_EQUAL(gwv.GetNumSoldiersForAttack(hqPos[0]), 0u);
    BOOST_REQUIRE_EQUAL(gwv.GetNumSoldiersForAttack(milBld0->GetPos()), 0u);
    // Attack both others
    BOOST_REQUIRE_EQUAL(gwv.GetNumSoldiersForAttack(hqPos[1]), 5u);
    BOOST_REQUIRE_EQUAL(gwv.GetNumSoldiersForAttack(hqPos[2]), 5u);
    BOOST_REQUIRE_EQUAL(gwv.GetNumSoldiersForAttack(milBld1Near->GetPos()), 5u);
    // Counterpart: 2 possible for far bld
    BOOST_REQUIRE_EQUAL(gwv.GetNumSoldiersForAttack(milBld1Far->GetPos()), 2u);

    // Functions related to stationed soldiers
    BOOST_REQUIRE(milBld0->HasMaxRankSoldier());
    BOOST_REQUIRE(milBld1Near->HasMaxRankSoldier());
    BOOST_REQUIRE(!milBld1Far->HasMaxRankSoldier());
    BOOST_REQUIRE_GT(milBld1Near->GetSoldiersStrength(), milBld1Far->GetSoldiersStrength());
}

BOOST_FIXTURE_TEST_CASE(StartAttack, AttackFixture<>)
{
    initGameRNG();
    GameWorldViewer gwv(curPlayer, world);

    // Add soldiers (3 strong, 3 weak)
    AddSoldiers(milBld0Pos, 3, 3);

    const nobMilitary& attackSrc = *milBld0;
    const MapPoint usualBldPos = hqPos[1] - MapPoint(2, 3);
    BOOST_REQUIRE_GE(world.GetBQ(usualBldPos, 1), BQ_HUT);
    const noBuilding* usualBld = BuildingFactory::CreateBuilding(world, BLD_WOODCUTTER, usualBldPos, 1, NAT_ROMANS);
    BOOST_REQUIRE(usualBld);
    const MapPoint storehousePos = hqPos[1] - MapPoint(2, 0);
    BOOST_REQUIRE_GE(world.GetBQ(storehousePos, 1), BQ_HOUSE);
    const noBuilding* storeHouse = BuildingFactory::CreateBuilding(world, BLD_STOREHOUSE, storehousePos, 1, NAT_ROMANS);
    BOOST_REQUIRE(storeHouse);
    MakeVisible(usualBldPos);
    MakeVisible(storehousePos);

    world.GetPlayer(0).team = TM_TEAM1;
    world.GetPlayer(1).team = TM_TEAM1;
    for(unsigned i = 0; i < 2; i++)
        world.GetPlayer(i).MakeStartPacts();
    // Try to attack ally -> Fail
    TestFailingAttack(gwv, milBld1Pos, attackSrc);

    world.GetPlayer(0).team = TM_TEAM1;
    world.GetPlayer(1).team = TM_TEAM2;
    for(unsigned i = 0; i < 2; i++)
        world.GetPlayer(i).MakeStartPacts();

    // Try to attack non-military bld -> Fail
    TestFailingAttack(gwv, usualBldPos, attackSrc);

    // Try to attack storehouse -> Fail
    TestFailingAttack(gwv, storehousePos, attackSrc);

    // Try to attack newly build bld -> Fail
    BOOST_REQUIRE_EQUAL(world.CalcVisiblityWithAllies(milBld1Pos, curPlayer), VIS_VISIBLE);
    BOOST_REQUIRE(milBld1->IsNewBuilt());
    TestFailingAttack(gwv, milBld1Pos, attackSrc);

    // Add soldier
    AddSoldiers(milBld1Pos, 1, 0);
    BOOST_REQUIRE(!milBld1->IsNewBuilt());
    // Try to attack invisible bld -> Fail
    MapNode& node = world.GetNodeWriteable(milBld1Pos);
    node.fow[0].visibility = VIS_FOW;
    BOOST_REQUIRE_EQUAL(world.CalcVisiblityWithAllies(milBld1Pos, curPlayer), VIS_FOW);
    TestFailingAttack(gwv, milBld1Pos, attackSrc);

    // Attack it
    node.fow[0].visibility = VIS_VISIBLE;
    std::vector<nofPassiveSoldier*> soldiers(attackSrc.GetTroops().begin(), attackSrc.GetTroops().end()); //-V807
    BOOST_REQUIRE_EQUAL(soldiers.size(), 6u);
    for(int i = 0; i < 3; i++)
        BOOST_REQUIRE_EQUAL(soldiers[i]->GetRank(), 0u);
    for(int i = 3; i < 6; i++)
        BOOST_REQUIRE_EQUAL(soldiers[i]->GetRank(), 4u);
    this->Attack(milBld1Pos, 1, true);
    // 1 strong soldier has left
    soldiers.assign(attackSrc.GetTroops().begin(), attackSrc.GetTroops().end());
    BOOST_REQUIRE_EQUAL(soldiers.size(), 5u);
    for(int i = 0; i < 3; i++)
        BOOST_REQUIRE_EQUAL(soldiers[i]->GetRank(), 0u);
    for(int i = 3; i < 5; i++)
        BOOST_REQUIRE_EQUAL(soldiers[i]->GetRank(), 4u);
    // Attack with 1 weak soldier
    this->Attack(milBld1Pos, 1, false);
    // 1 weak soldier has left
    soldiers.assign(attackSrc.GetTroops().begin(), attackSrc.GetTroops().end());
    BOOST_REQUIRE_EQUAL(soldiers.size(), 4u);
    for(int i = 0; i < 2; i++)
        BOOST_REQUIRE_EQUAL(soldiers[i]->GetRank(), 0u);
    for(int i = 2; i < 4; i++)
        BOOST_REQUIRE_EQUAL(soldiers[i]->GetRank(), 4u);
    // -> 2 strong, 2 weak remaining, attack with 3 weak ones -> 1 strong remaining
    this->Attack(milBld1Pos, 3, false);
    soldiers.assign(attackSrc.GetTroops().begin(), attackSrc.GetTroops().end());
    BOOST_REQUIRE_EQUAL(soldiers.size(), 1u);
    BOOST_REQUIRE_EQUAL(soldiers[0]->GetRank(), 4u);

    // None left
    TestFailingAttack(gwv, milBld1Pos, attackSrc, 1u);
}

BOOST_FIXTURE_TEST_CASE(ConquerBld, AttackFixture<>)
{
    initGameRNG();

    AddSoldiers(milBld0Pos, 1, 5);
    AddSoldiersWithRank(milBld1Pos, 1, 0);
    AddSoldiersWithRank(milBld1Pos, 1, 1);
    BuildRoadForBlds(milBld0Pos, hqPos[0]);

    // Finish recruiting, carrier outhousing etc.
    RTTR_SKIP_GFS(400);
    // Start attack ->1 (weak one first)
    this->Attack(milBld1Pos, 1, false);
    this->Attack(milBld1Pos, 5, false);
    BOOST_REQUIRE_EQUAL(milBld0->GetNumTroops(), 1u);
    BOOST_REQUIRE_EQUAL(milBld1->GetNumTroops(), 2u);
    // Run till attackers reach bld. 1 Soldier will leave for them.
    // 1 stays inside till an attacker is at door
    // 20 GFs/node + 30 GFs for leaving
    const unsigned distance = world.CalcDistance(milBld0Pos, milBld1Pos);
    RTTR_EXEC_TILL(distance * 20 + 30, milBld1->GetNumTroops() == 1);
    BOOST_REQUIRE_EQUAL(milBld1->GetNumTroops() + milBld1->GetLeavingFigures().size(), 2u);
    const Inventory& attackedPlInventory = world.GetPlayer(1).GetInventory();
    const unsigned oldWeakSoldierCt = attackedPlInventory.people[JOB_PRIVATE];
    const unsigned oldStrongerSoldierCt = attackedPlInventory.people[JOB_PRIVATEFIRSTCLASS];
    const unsigned oldAttackerStrongSoldierCt = world.GetPlayer(curPlayer).GetInventory().people[JOB_GENERAL];

    // 1st soldier will walk towards attacker and will be killed
    // Once an attacker reaches the flag, the bld will send a defender
    BOOST_REQUIRE(!milBld1->GetDefender());
    RTTR_EXEC_TILL(300, milBld1->GetNumTroops() == 0);
    // Defender deployed, attacker at flag
    BOOST_REQUIRE(milBld1->GetDefender());
    const std::list<noBase*>& figures = world.GetFigures(milBld1->GetFlag()->GetPos());
    BOOST_REQUIRE_EQUAL(figures.size(), 1u);
    BOOST_REQUIRE(dynamic_cast<nofAttacker*>(figures.front()));
    BOOST_REQUIRE_EQUAL(static_cast<nofAttacker*>(figures.front())->GetPlayer(), curPlayer);

    // Lets fight
    RTTR_EXEC_TILL(1000, milBld1->IsBeingCaptured());
    // Let others in
    RTTR_EXEC_TILL(200, !milBld1->IsBeingCaptured());
    // Building conquered
    BOOST_REQUIRE_EQUAL(milBld1->GetPlayer(), curPlayer);
    // 1 soldier must be inside
    BOOST_REQUIRE_GT(milBld1->GetNumTroops(), 1u);
    // Weak soldier must be dead
    BOOST_REQUIRE_EQUAL(attackedPlInventory.people[JOB_PRIVATE], oldWeakSoldierCt - 1);
    // Src building refill
    RTTR_EXEC_TILL(800, milBld0->GetNumTroops() == 6u);
    // Src building got refilled
    BOOST_REQUIRE_EQUAL(milBld0->GetNumTroops(), 6u);
    // We may have lost soldiers
    BOOST_REQUIRE_LE(world.GetPlayer(curPlayer).GetInventory().people[JOB_GENERAL], oldAttackerStrongSoldierCt);
    // The enemy may have lost his stronger soldier
    BOOST_REQUIRE_LE(attackedPlInventory.people[JOB_PRIVATEFIRSTCLASS], oldStrongerSoldierCt);
    // But only one
    BOOST_REQUIRE_GE(attackedPlInventory.people[JOB_PRIVATEFIRSTCLASS], oldStrongerSoldierCt - 1);
    // At least 2 survivors
    BOOST_REQUIRE_GT(milBld1->GetNumTroops(), 2u);

    // Points around bld should be ours
    const std::vector<MapPoint> pts = world.GetPointsInRadius(milBld1Pos, 3);
    for(const MapPoint& pt : pts)
    {
        BOOST_REQUIRE_EQUAL(world.GetNode(pt).owner, curPlayer + 1u);
    }
}

BOOST_FIXTURE_TEST_CASE(ConquerBldCoinAddonEnable, AttackFixture<>)
{
    this->ggs.setSelection(AddonId::COINS_CAPTURED_BLD, 1); // addon is active on second run

    initGameRNG();
    AddSoldiers(milBld0Pos, 1, 5);
    AddSoldiersWithRank(milBld1Pos, 1, 0);
    // Finish recruiting, carrier outhousing etc.
    RTTR_SKIP_GFS(400);

    // ensure that coins are disabled
    milBld1->SetCoinsAllowed(false);
    BOOST_REQUIRE(milBld1->IsGoldDisabled());

    // Start attack -> 1
    this->Attack(milBld1Pos, 6, false);
    BOOST_REQUIRE_EQUAL(milBld0->GetNumTroops(), 1u);

    RTTR_EXEC_TILL(2000, milBld1->GetPlayer() == curPlayer);

    // check if coins were enabled after building was captured
    BOOST_REQUIRE(!milBld1->IsGoldDisabled());
}

BOOST_FIXTURE_TEST_CASE(ConquerBldCoinAddonDisable, AttackFixture<>)
{
    this->ggs.setSelection(AddonId::COINS_CAPTURED_BLD, 2); // addon is active on second run

    initGameRNG();
    AddSoldiers(milBld0Pos, 1, 5);
    AddSoldiersWithRank(milBld1Pos, 1, 0);
    // Finish recruiting, carrier outhousing etc.
    RTTR_SKIP_GFS(400);

    // ensure that coins are enabled
    milBld1->SetCoinsAllowed(true);
    BOOST_REQUIRE(!milBld1->IsGoldDisabled());

    // Start attack -> 1
    this->Attack(milBld1Pos, 6, false);
    BOOST_REQUIRE_EQUAL(milBld0->GetNumTroops(), 1u);

    RTTR_EXEC_TILL(2000, milBld1->GetPlayer() == curPlayer);

    // check if coins were disabled after building was captured
    BOOST_REQUIRE(milBld1->IsGoldDisabled());
}

using AttackFixture4P = AttackFixture<4, 32, 34>;
BOOST_FIXTURE_TEST_CASE(ConquerWithMultipleWalkingIn, AttackFixture4P)
{
    initGameRNG();
    world.GetPlayer(0).team = TM_TEAM1; //-V525
    world.GetPlayer(1).team = TM_NOTEAM;
    world.GetPlayer(2).team = TM_TEAM1; // Allied to 0
    world.GetPlayer(3).team = TM_TEAM2; // Hostile to 0
    for(unsigned i = 0; i < 4; i++)
        world.GetPlayer(i).MakeStartPacts();
    MilitarySettings milSettings = MILITARY_SETTINGS_SCALE;
    milSettings[2] = 0; // No aggressive defenders for attacker
    this->ChangeMilitary(milSettings);

    AddSoldiers(milBld0Pos, 0, 6);
    AddSoldiersWithRank(milBld1Pos, 1, 0);
    MapPoint milBld1FlagPos = world.GetNeighbour(milBld1Pos, Direction::SOUTHEAST);

    // Scenario 1: Attack with one soldier.
    // Once enemy is defeated we walk in with another soldier of the enemy who wants to occupy its building.
    // The other soldier is faster -> we have to fight him
    this->Attack(milBld1Pos, 1, true);
    BOOST_REQUIRE_EQUAL(milBld0->GetLeavingFigures().size(), 1u); //-V807
    auto* attacker = dynamic_cast<nofAttacker*>(milBld0->GetLeavingFigures().front());
    BOOST_REQUIRE(attacker);
    // Move him directly out
    const_cast<std::list<noFigure*>&>(milBld0->GetLeavingFigures()).pop_front();
    moveObjTo(world, *attacker, milBld1FlagPos); //-V522
    BOOST_REQUIRE(!milBld1->IsDoorOpen());
    const std::list<noBase*>& flagFigs = world.GetFigures(milBld1FlagPos);
    RTTR_EXEC_TILL(70, flagFigs.size() == 1u && flagFigs.front()->GetGOT() == GOT_FIGHTING); //-V807
    BOOST_REQUIRE(!milBld1->IsDoorOpen());
    // Speed up fight by reducing defenders HP to 1
    auto* defender = const_cast<nofDefender*>(milBld1->GetDefender());
    while(defender->GetHitpoints() > 1u)
        defender->TakeHit();
    RTTR_EXEC_TILL(500, milBld1->GetDefender() == nullptr);
    // Defender defeated. Attacker moving in.
    BOOST_REQUIRE(attacker->IsMoving());
    BOOST_REQUIRE_EQUAL(attacker->GetCurMoveDir(), Direction::NORTHWEST);
    // Door opened
    BOOST_REQUIRE(milBld1->IsDoorOpen());
    // New soldiers walked in
    AddSoldiersWithRank(milBld1Pos, 4, 0);
    // Let attacker walk in (try it at least)
    RTTR_EXEC_TILL(20, attacker->GetPos() == milBld1Pos);
    RTTR_EXEC_TILL(20, attacker->GetPos() == milBld1FlagPos);
    // New fight and door closed
    RTTR_EXEC_TILL(70, flagFigs.size() == 1u && flagFigs.front()->GetGOT() == GOT_FIGHTING);
    BOOST_REQUIRE(!milBld1->IsDoorOpen());

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
    BOOST_REQUIRE_EQUAL(milBld1->GetLeavingFigures().size(), 1u);
    auto* attackerFromPl0 = dynamic_cast<nofAttacker*>(milBld1->GetLeavingFigures().front());
    BOOST_REQUIRE(attackerFromPl0);
    // 2.
    curPlayer = 0;
    this->Attack(milBld1Pos, 1, true);
    // Move him directly out
    BOOST_REQUIRE_EQUAL(milBld0->GetLeavingFigures().size(), 1u);
    auto* secAttacker = dynamic_cast<nofAttacker*>(milBld0->GetLeavingFigures().front());
    BOOST_REQUIRE(secAttacker);
    const_cast<std::list<noFigure*>&>(milBld0->GetLeavingFigures()).pop_front();
    moveObjTo(world, *secAttacker, world.MakeMapPoint(milBld1FlagPos - Position(15, 0))); //-V522
    nofAggressiveDefender* aggDefender = milBld1->SendAggressiveDefender(secAttacker);
    BOOST_REQUIRE(aggDefender);
    secAttacker->LetsFight(aggDefender);
    // 3.
    curPlayer = 2;
    MapPoint bldPos = hqPos[curPlayer] + MapPoint(3, 0);
    auto* alliedBld = static_cast<nobMilitary*>(BuildingFactory::CreateBuilding(world, BLD_GUARDHOUSE, bldPos, curPlayer, NAT_AFRICANS));
    AddSoldiersWithRank(bldPos, 2, 0);
    this->Attack(milBld1Pos, 1, false);
    BOOST_REQUIRE_EQUAL(alliedBld->GetLeavingFigures().size(), 1u);
    auto* alliedAttacker = dynamic_cast<nofAttacker*>(alliedBld->GetLeavingFigures().front());
    BOOST_REQUIRE(alliedAttacker);
    // 4.
    curPlayer = 3;
    bldPos = hqPos[curPlayer] + MapPoint(3, 0);
    auto* hostileBld = static_cast<nobMilitary*>(BuildingFactory::CreateBuilding(world, BLD_GUARDHOUSE, bldPos, curPlayer, NAT_AFRICANS));
    AddSoldiersWithRank(bldPos, 2, 0);
    this->Attack(milBld1Pos, 1, false);
    BOOST_REQUIRE_EQUAL(hostileBld->GetLeavingFigures().size(), 1u);
    auto* hostileAttacker = dynamic_cast<nofAttacker*>(hostileBld->GetLeavingFigures().front());
    BOOST_REQUIRE(hostileAttacker);

    // Make sure all other soldiers left their buildings (<=30GFs each + 20 for walking to flag and a bit further)
    RTTR_SKIP_GFS(30 + 20 + 10);
    // And suspend them to inspect them later on
    rescheduleWalkEvent(em, *attackerFromPl0, 10000); //-V522
    rescheduleWalkEvent(em, *secAttacker, 10000);
    rescheduleWalkEvent(em, *alliedAttacker, 10000);  //-V522
    rescheduleWalkEvent(em, *hostileAttacker, 10000); //-V522
    // We got 2 from milBld1
    RTTR_SKIP_GFS(30);
    rescheduleWalkEvent(em, *aggDefender, 10000);
    // Let defenders (2!) die
    defender = const_cast<nofDefender*>(milBld1->GetDefender());
    while(defender->GetHitpoints() > 1u)
        defender->TakeHit();
    RTTR_EXEC_TILL(500, milBld1->GetNumTroops() == 0u);
    defender = const_cast<nofDefender*>(milBld1->GetDefender());
    while(defender->GetHitpoints() > 1u)
        defender->TakeHit();
    RTTR_EXEC_TILL(500, milBld1->GetDefender() == nullptr);
    // Defender defeated. Attacker moving in.
    BOOST_REQUIRE(attacker->IsMoving());
    BOOST_REQUIRE_EQUAL(attacker->GetCurMoveDir(), Direction::NORTHWEST);
    // Door opened
    BOOST_REQUIRE(milBld1->IsDoorOpen());
    // Give him a bit of a head start
    RTTR_SKIP_GFS(1);
    // 5. Create new soldier who walks in after the attacker
    auto* newSld = new nofPassiveSoldier(milBld1FlagPos, 1, milBld1, milBld1, 0);
    milBld1->GotWorker(newSld->GetJobType(), newSld);
    world.AddFigure(milBld1FlagPos, newSld);
    newSld->ActAtFirst();
    // Attacker faster -> Bld taken
    RTTR_EXEC_TILL(30, milBld1->GetPlayer() == 0u);
    // New soldier should be wandering
    RTTR_EXEC_TILL(10, newSld->IsWandering());
    // And door should be closed (latest after wandering soldier left flag)
    RTTR_EXEC_TILL(40, !milBld1->IsDoorOpen());

    // 1. Attackers from this building
    // No home -> Wander
    BOOST_REQUIRE(attackerFromPl0->HasNoHome());
    rescheduleWalkEvent(em, *attackerFromPl0, 1);
    RTTR_EXEC_TILL(2, attackerFromPl0->IsWandering());
    // 2. Aggressive defenders from this building
    // No further attack (unless already fighting) and wander
    // The attacker proceeds to the building and occupies it
    rescheduleWalkEvent(em, *secAttacker, 1);
    rescheduleWalkEvent(em, *aggDefender, 2);
    RTTR_SKIP_GFS(2);
    BOOST_REQUIRE(aggDefender->GetAttacker() == nullptr);
    BOOST_REQUIRE(secAttacker->GetHuntingDefender() == nullptr);
    BOOST_REQUIRE(aggDefender->IsWandering());
    RTTR_EXEC_TILL(270, milBld1->GetNumTroops() == 2u);
    // 3. Allied aggressor towards this bld
    // Abort attack and return home
    rescheduleWalkEvent(em, *alliedAttacker, 1);
    RTTR_EXEC_TILL(1, alliedAttacker->GetAttackedGoal() == nullptr);
    RTTR_EXEC_TILL(90, alliedBld->GetNumTroops() == 2u);
    // 4. Hostile aggressor towards this bld
    // Continue attack and fight
    rescheduleWalkEvent(em, *hostileAttacker, 1);
    BOOST_REQUIRE(hostileAttacker->GetAttackedGoal() != nullptr);
    RTTR_EXEC_TILL(220, hostileAttacker->GetPos() == milBld1FlagPos);
    RTTR_EXEC_TILL(50, world.GetFigures(milBld1FlagPos).front()->GetGOT() == GOT_FIGHTING);
}

BOOST_FIXTURE_TEST_CASE(ConquerWithCarriersWalkingIn, AttackFixture<2>)
{
    // 1. Carrier with coin walking in the building
    // 2. Carrier with coin walking out of the building
    AddSoldiers(milBld0Pos, 0, 6);
    AddSoldiersWithRank(milBld1Pos, 1, 0);
    MapPoint milBld1FlagPos = world.GetNeighbour(milBld1Pos, Direction::SOUTHEAST);

    curPlayer = 1;
    MapPoint flagPos = world.MakeMapPoint(milBld1FlagPos - Position(2, 0));
    this->BuildRoad(milBld1FlagPos, false, std::vector<Direction>(2, Direction::WEST));
    auto* flag = world.GetSpecObj<noFlag>(flagPos);
    BOOST_REQUIRE(flag);
    RoadSegment* rs = flag->GetRoute(Direction::EAST);
    BOOST_REQUIRE(rs);
    auto* carrierIn = new nofCarrier(nofCarrier::CT_NORMAL, flagPos, curPlayer, rs, flag);
    auto* carrierOut = new nofCarrier(nofCarrier::CT_DONKEY, flagPos, curPlayer, rs, flag);
    world.AddFigure(flagPos, carrierIn);
    world.AddFigure(flagPos, carrierOut);
    rs->setCarrier(0, carrierIn);
    rs->setCarrier(1, carrierOut);
    // Add 2 coins for the bld
    for(unsigned i = 0; i < 2; i++)
    {
        auto* coin = new Ware(GD_COINS, milBld1, flag);
        coin->WaitAtFlag(flag);
        coin->RecalcRoute();
        flag->AddWare(coin);
    }
    world.GetPlayer(1).IncreaseInventoryWare(GD_COINS, 2);
    carrierIn->ActAtFirst();
    carrierOut->ActAtFirst();
    // Both picked up
    BOOST_REQUIRE_EQUAL(flag->GetNumWares(), 0u);
    // Move carriers to flag
    for(unsigned i = 0; i < 2; i++)
    {
        rescheduleWalkEvent(em, *carrierIn, 1);
        rescheduleWalkEvent(em, *carrierOut, 1);
        RTTR_SKIP_GFS(1);
    }
    // And pause them
    rescheduleWalkEvent(em, *carrierIn, 10000);
    // After the out-walking was in
    rescheduleWalkEvent(em, *carrierOut, 1);
    RTTR_SKIP_GFS(1);
    rescheduleWalkEvent(em, *carrierOut, 10000);
    BOOST_REQUIRE_EQUAL(carrierIn->GetCurMoveDir(), Direction::NORTHWEST);
    BOOST_REQUIRE_EQUAL(carrierOut->GetCurMoveDir(), Direction::SOUTHEAST);

    // Add another for later
    MapPoint flagPosE = world.MakeMapPoint(milBld1FlagPos + Position(2, 0));
    this->BuildRoad(milBld1FlagPos, false, std::vector<Direction>(2, Direction::EAST));
    auto* flagE = world.GetSpecObj<noFlag>(flagPosE);
    BOOST_REQUIRE(flagE);
    RoadSegment* rsE = flagE->GetRoute(Direction::WEST);
    BOOST_REQUIRE(rsE);
    auto* carrierInE = new nofCarrier(nofCarrier::CT_NORMAL, flagPosE, curPlayer, rsE, flagE);
    world.AddFigure(flagPosE, carrierInE);
    rsE->setCarrier(0, carrierInE);
    // He also gets 1 coin
    auto* coin = new Ware(GD_COINS, milBld1, flagE);
    coin->WaitAtFlag(flagE);
    coin->RecalcRoute();
    flagE->AddWare(coin);
    world.GetPlayer(1).IncreaseInventoryWare(GD_COINS, 1);
    carrierInE->ActAtFirst();
    // Picked up
    BOOST_REQUIRE_EQUAL(flagE->GetNumWares(), 0u);
    // And pause him
    rescheduleWalkEvent(em, *carrierInE, 10000);

    curPlayer = 0;
    this->Attack(milBld1Pos, 1, true);
    BOOST_REQUIRE_EQUAL(milBld0->GetLeavingFigures().size(), 1u);
    auto* attacker = dynamic_cast<nofAttacker*>(milBld0->GetLeavingFigures().front());
    BOOST_REQUIRE(attacker);
    // Move him directly out
    const_cast<std::list<noFigure*>&>(milBld0->GetLeavingFigures()).pop_front();
    moveObjTo(world, *attacker, milBld1FlagPos); //-V522
    const std::list<noBase*>& flagFigs = world.GetFigures(milBld1FlagPos);
    RTTR_EXEC_TILL(20, attacker->GetPos() == milBld1FlagPos);
    // Carriers on pos or to pos get send away as soon as soldier arrives
    rescheduleWalkEvent(em, *carrierIn, 1);
    rescheduleWalkEvent(em, *carrierOut, 1);
    RTTR_SKIP_GFS(1);
    BOOST_REQUIRE(carrierIn->IsWandering());
    BOOST_REQUIRE(carrierOut->IsWandering());

    // Let east carrier walk
    rescheduleWalkEvent(em, *carrierInE, 1);

    // Start fight
    RTTR_EXEC_TILL(50, flagFigs.size() == 1u && flagFigs.front()->GetGOT() == GOT_FIGHTING);
    // East carrier gets blocked
    BOOST_REQUIRE(!carrierInE->IsMoving());

    // Door closed latest after other carriers are gone
    RTTR_EXEC_TILL(20, !milBld1->IsDoorOpen());

    // Speed up fight by reducing defenders HP to 1
    auto* defender = const_cast<nofDefender*>(milBld1->GetDefender());
    while(defender->GetHitpoints() > 1u)
        defender->TakeHit();
    RTTR_EXEC_TILL(500, milBld1->GetDefender() == nullptr);
    // Defender defeated. Attacker moving in.
    BOOST_REQUIRE(attacker->IsMoving());
    BOOST_REQUIRE_EQUAL(attacker->GetCurMoveDir(), Direction::NORTHWEST);

    // Door opened
    BOOST_REQUIRE(milBld1->IsDoorOpen());
    // Blocked carrier can walk again
    BOOST_REQUIRE(carrierInE->IsMoving());
    // Capture
    RTTR_EXEC_TILL(20, milBld1->GetPlayer() == 0u);
    // East Carrier should be wandering at some point
    RTTR_EXEC_TILL(20, carrierInE->IsWandering());
    // Only 1 coin in the bld
    BOOST_REQUIRE_EQUAL(milBld1->GetNumCoins(), 1u);
    // Door closed after carrier left it
    RTTR_EXEC_TILL(40, !milBld1->IsDoorOpen());
}

using DestroyRoadsOnConquerFixture = AttackFixture<2, 24>;
BOOST_FIXTURE_TEST_CASE(DestroyRoadsOnConquer, DestroyRoadsOnConquerFixture)
{
    MapPoint leftBldPos = world.MakeMapPoint(milBld1Pos + Position(-5, 2));
    noBuilding* leftBld = BuildingFactory::CreateBuilding(world, BLD_BARRACKS, leftBldPos, 1, NAT_BABYLONIANS);
    BOOST_REQUIRE(leftBld);
    MapPoint rightBldPos = world.MakeMapPoint(milBld1Pos + Position(5, 2));
    noBuilding* rightBld = BuildingFactory::CreateBuilding(world, BLD_BARRACKS, rightBldPos, 1, NAT_BABYLONIANS);
    BOOST_REQUIRE(rightBld);

    AddSoldiersWithRank(leftBldPos, 1, 0);
    AddSoldiersWithRank(rightBldPos, 1, 0);
    AddSoldiersWithRank(milBld1Pos, 1, 0);
    AddSoldiersWithRank(milBld0Pos, 6, 4);

    curPlayer = 1;
    // Build 2 roads to attack building to test that destroying them does not cause a bug
    // Bld -> Flag
    BuildRoadForBlds(milBld1Pos, world.MakeMapPoint(milBld1Pos + Position(2, 0)));
    // Flag -> Bld
    const MapPoint flagPt = world.MakeMapPoint(milBld1Pos - Position(2, 0));
    this->SetFlag(flagPt);
    BuildRoadForBlds(world.GetNeighbour(flagPt, Direction::NORTHWEST), milBld1Pos);
    // Build a long road connecting left&right w/o any flag inbetween (See #863)
    BuildRoadForBlds(leftBldPos, rightBldPos);
    BOOST_REQUIRE_GE(leftBld->GetFlag()->GetRoute(Direction::EAST)->GetLength(), 10u);

    curPlayer = 0;
    this->Attack(milBld1Pos, 5, true);
    RTTR_EXEC_TILL(2000, milBld1->GetPlayer() == curPlayer);
    std::array<MapPoint, 3> bldPts = {{leftBldPos, rightBldPos, milBld1Pos}};
    for(const MapPoint& bldPt : bldPts)
    {
        MapPoint flagPt = world.GetNeighbour(bldPt, Direction::SOUTHEAST);
        for(Direction i : helpers::EnumRange<Direction>{})
        {
            // No routes except the main road
            if(i != Direction::NORTHWEST)
            {
                BOOST_REQUIRE(!world.GetSpecObj<noRoadNode>(flagPt)->GetRoute(i));
                BOOST_REQUIRE_EQUAL(world.GetPointRoad(flagPt, i), PointRoad::None);
            }
        }
    }
}

BOOST_AUTO_TEST_SUITE_END()
