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

#include "defines.h" // IWYU pragma: keep
#include "buildings/nobBaseWarehouse.h"
#include "buildings/nobMilitary.h"
#include "factories/BuildingFactory.h"
#include "figures/nofAttacker.h"
#include "figures/nofPassiveSoldier.h"
#include "pathfinding/FindPathForRoad.h"
#include "world/GameWorldViewer.h"
#include "nodeObjs/noFlag.h"
#include "gameData/SettingTypeConv.h"
#include "test/WorldWithGCExecution.h"
#include "test/initTestHelpers.h"
#include <boost/foreach.hpp>
#include <boost/test/unit_test.hpp>
#include <iostream>

BOOST_AUTO_TEST_SUITE(AttackSuite)

struct AttackDefaults
{
    BOOST_STATIC_CONSTEXPR unsigned width = 12;
    BOOST_STATIC_CONSTEXPR unsigned height = 20;
};

template<unsigned T_numPlayers, unsigned T_width, unsigned T_height>
struct AttackFixtureBase : public WorldWithGCExecution<T_numPlayers, T_width, T_height>
{
    /// Positions of the players HQ
    boost::array<MapPoint, T_numPlayers> hqPos;
    typedef WorldWithGCExecution<T_numPlayers, T_width, T_height> Parent;
    using Parent::world;
    using Parent::curPlayer;

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
        BOOST_REQUIRE_EQUAL(world.GetPointRoad(start, road.front()), RoadSegment::RT_NORMAL + 1);
    }

    void AddSoldiersWithRank(MapPoint bldPos, unsigned numSoldiers, unsigned rank)
    {
        BOOST_REQUIRE_LE(rank, world.GetGGS().GetMaxMilitaryRank());
        nobMilitary* bld = world.template GetSpecObj<nobMilitary>(bldPos);
        BOOST_REQUIRE(bld);
        const unsigned oldNumSoldiers = bld->GetTroopsCount();
        for(unsigned i = 0; i < numSoldiers; i++)
        {
            nofPassiveSoldier* soldier = new nofPassiveSoldier(bldPos, bld->GetPlayer(), bld, bld, rank);
            world.GetPlayer(bld->GetPlayer()).IncreaseInventoryJob(soldier->GetJobType(), 1);
            world.AddFigure(soldier, bldPos);
            // Let him "walk" to goal -> Already reached -> Added and all internal states set correctly
            soldier->WalkToGoal();
            BOOST_REQUIRE(soldier->HasNoGoal());
        }
        BOOST_REQUIRE_EQUAL(bld->GetTroopsCount(), oldNumSoldiers + numSoldiers);
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

struct AttackFixture : public AttackFixtureBase<2, AttackDefaults::width, AttackDefaults::height>
{
    /// Tested positions for military buildings
    MapPoint milBld0Pos, milBld1Pos;
    /// Military buildings of players 0 and 1
    const nobMilitary *milBld0, *milBld1;

    AttackFixture()
    {
        // Build some military buildings
        milBld0Pos = world.MakeMapPoint(hqPos[0] + Position(6, 0));
        BOOST_REQUIRE_EQUAL(world.GetBQ(milBld0Pos, 0), BQ_CASTLE);
        milBld0 = static_cast<nobMilitary*>(BuildingFactory::CreateBuilding(world, BLD_WATCHTOWER, milBld0Pos, 0, NAT_BABYLONIANS));
        BOOST_REQUIRE(milBld0);

        milBld1Pos = world.MakeMapPoint(hqPos[1] + Position(6, 0));
        BOOST_REQUIRE_EQUAL(world.GetBQ(milBld1Pos, 1), BQ_CASTLE);
        milBld1 = static_cast<nobMilitary*>(BuildingFactory::CreateBuilding(world, BLD_WATCHTOWER, milBld1Pos, 1, NAT_ROMANS));
        BOOST_REQUIRE(milBld1);
        MakeVisible(milBld0Pos);
        MakeVisible(milBld1Pos);
    }

    /// Assert that attacking the given building from attackSrc fails
    void TestFailingAttack(const GameWorldViewer& gwv, const MapPoint& bldPos, const nobMilitary& attackSrc, unsigned numSoldiersLeft = 6u)
    {
        BOOST_REQUIRE_EQUAL(attackSrc.GetTroopsCount(), numSoldiersLeft);
        // No available soldiers
        BOOST_REQUIRE_EQUAL(gwv.GetNumSoldiersForAttack(bldPos), 0u);
        this->Attack(bldPos, 1, true);
        // Same left
        BOOST_REQUIRE_EQUAL(attackSrc.GetTroopsCount(), numSoldiersLeft);
    }
};

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
    BOOST_REQUIRE_EQUAL(milBld1Near->GetTroopsCount(), 6u);
    BOOST_REQUIRE_EQUAL(milBld1Far->GetTroopsCount(), 6u);
    BOOST_REQUIRE_EQUAL(milBld0->GetTroopsCount(), 6u);

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

BOOST_FIXTURE_TEST_CASE(StartAttack, AttackFixture)
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
    std::vector<nofPassiveSoldier*> soldiers(attackSrc.GetTroops().begin(), attackSrc.GetTroops().end());
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

BOOST_FIXTURE_TEST_CASE(ConquerBld, AttackFixture)
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
    BOOST_REQUIRE_EQUAL(milBld0->GetTroopsCount(), 1u);
    BOOST_REQUIRE_EQUAL(milBld1->GetTroopsCount(), 2u);
    // Run till attackers reach bld. 1 Soldier will leave for them.
    // 1 stays inside till an attacker is at door
    // 20 GFs/node + 30 GFs for leaving
    const unsigned distance = world.CalcDistance(milBld0Pos, milBld1Pos);
    RTTR_EXEC_TILL(distance * 20 + 30, milBld1->GetTroopsCount() == 1);
    BOOST_REQUIRE_EQUAL(milBld1->GetTroopsCount() + milBld1->GetLeavingFigures().size(), 2u);
    const Inventory& attackedPlInventory = world.GetPlayer(1).GetInventory();
    const unsigned oldWeakSoldierCt = attackedPlInventory.people[JOB_PRIVATE];
    const unsigned oldStrongerSoldierCt = attackedPlInventory.people[JOB_PRIVATEFIRSTCLASS];
    const unsigned oldAttackerStrongSoldierCt = world.GetPlayer(curPlayer).GetInventory().people[JOB_GENERAL];

    // 1st soldier will walk towards attacker and will be killed
    // Once an attacker reaches the flag, the bld will send a defender
    BOOST_REQUIRE(!milBld1->GetDefender());
    RTTR_EXEC_TILL(300, milBld1->GetTroopsCount() == 0);
    BOOST_REQUIRE_EQUAL(milBld1->GetTroopsCount(), 0u);
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
    BOOST_REQUIRE_GT(milBld1->GetTroopsCount(), 1u);
    // Weak soldier must be dead
    BOOST_REQUIRE_EQUAL(attackedPlInventory.people[JOB_PRIVATE], oldWeakSoldierCt - 1);
    // Src building refill
    RTTR_EXEC_TILL(800, milBld0->GetTroopsCount() == 6u);
    // Src building got refilled
    BOOST_REQUIRE_EQUAL(milBld0->GetTroopsCount(), 6u);
    // We may have lost soldiers
    BOOST_REQUIRE_LE(world.GetPlayer(curPlayer).GetInventory().people[JOB_GENERAL], oldAttackerStrongSoldierCt);
    // The enemy may have lost his stronger soldier
    BOOST_REQUIRE_LE(attackedPlInventory.people[JOB_PRIVATEFIRSTCLASS], oldStrongerSoldierCt);
    // But only one
    BOOST_REQUIRE_GE(attackedPlInventory.people[JOB_PRIVATEFIRSTCLASS], oldStrongerSoldierCt - 1);
    // At least 2 survivors
    BOOST_REQUIRE_GT(milBld1->GetTroopsCount(), 2u);

    // Points around bld should be ours
    const std::vector<MapPoint> pts = world.GetPointsInRadius(milBld1Pos, 3);
    BOOST_FOREACH(const MapPoint& pt, pts)
    {
        BOOST_REQUIRE_EQUAL(world.GetNode(pt).owner, curPlayer + 1u);
    }
}

BOOST_AUTO_TEST_SUITE_END()
