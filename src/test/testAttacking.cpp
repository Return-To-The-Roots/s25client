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
    BOOST_STATIC_CONSTEXPR unsigned width = 58;
    BOOST_STATIC_CONSTEXPR unsigned height = 38;
};

// Size is choosen based on current maximum attacking distances!
struct AttackFixture : public WorldWithGCExecution<3, AttackDefaults::width, AttackDefaults::height>
{
    typedef WorldWithGCExecution<3, AttackDefaults::width, AttackDefaults::height> Parent;
    using Parent::world;
    using Parent::curPlayer;

    /// Positions of the players HQ
    boost::array<MapPoint, 3> hqPos;
    /// Tested positions for military buildings
    MapPoint milBld1NearPos, milBld1FarPos, milBld2Pos;
    /// Military buildings of players 1 and 2 (for 1, one is close and one far away from player 1)
    const nobMilitary *milBld1Near, *milBld1Far, *milBld2;
    GameWorldViewer gwv;

    AttackFixture() : gwv(curPlayer, world)
    {
        // Make sure attacking is not limited by visibility
        RTTR_FOREACH_PT(MapPoint, world.GetSize())
        {
            world.SetVisibility(pt, 2, VIS_VISIBLE, this->em.GetCurrentGF());
        }

        for(unsigned i = 0; i < 3; i++)
        {
            curPlayer = i;
            hqPos[i] = world.GetPlayer(i).GetHQPos();
            nobBaseWarehouse* hq = world.GetSpecObj<nobBaseWarehouse>(hqPos[i]);
            Inventory goods;
            goods.Add(JOB_GENERAL, 3);
            hq->AddGoods(goods, true);
            this->ChangeMilitary(MILITARY_SETTINGS_SCALE);
        }
        // Assert player positions: 0: Top-Left, 1: Top-Right, 2: Bottom-Left
        BOOST_REQUIRE_LT(hqPos[0].x, hqPos[1].x);
        BOOST_REQUIRE_LT(hqPos[0].y, hqPos[2].y);
        BOOST_REQUIRE_LT(hqPos[1].y, hqPos[2].y);

        // Build some military buildings

        milBld1NearPos = hqPos[1] - MapPoint(8, 0);
        BOOST_REQUIRE_EQUAL(world.GetBQ(milBld1NearPos, 1), BQ_CASTLE);
        milBld1Near = dynamic_cast<nobMilitary*>(BuildingFactory::CreateBuilding(world, BLD_WATCHTOWER, milBld1NearPos, 1, NAT_ROMANS));
        BOOST_REQUIRE(milBld1Near);

        milBld1FarPos = hqPos[1] + MapPoint(5, 4);
        BOOST_REQUIRE_EQUAL(world.GetBQ(milBld1FarPos, 1), BQ_CASTLE);
        milBld1Far = dynamic_cast<nobMilitary*>(BuildingFactory::CreateBuilding(world, BLD_WATCHTOWER, milBld1FarPos, 1, NAT_ROMANS));
        BOOST_REQUIRE(milBld1Far);

        milBld2Pos = hqPos[2] - MapPoint(0, 6);
        BOOST_REQUIRE_EQUAL(world.GetBQ(milBld2Pos, 2), BQ_CASTLE);
        milBld2 = dynamic_cast<nobMilitary*>(BuildingFactory::CreateBuilding(world, BLD_WATCHTOWER, milBld2Pos, 2, NAT_BABYLONIANS));
        BOOST_REQUIRE(milBld2);
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

    void SetCurPlayer(unsigned playerIdx)
    {
        curPlayer = playerIdx;
        gwv.ChangePlayer(playerIdx);
    }

    void AddSoldiersWithRank(MapPoint bldPos, unsigned numSoldiers, unsigned rank)
    {
        BOOST_REQUIRE_LE(rank, world.GetGGS().GetMaxMilitaryRank());
        nobMilitary* bld = world.GetSpecObj<nobMilitary>(bldPos);
        BOOST_REQUIRE(bld);
        const unsigned oldNumSoldiers = bld->GetTroopsCount();
        for(unsigned i = 0; i < numSoldiers; i++)
        {
            nofPassiveSoldier* soldier = new nofPassiveSoldier(bldPos, bld->GetPlayer(), bld, bld, rank);
            world.GetPlayer(bld->GetPlayer()).IncreaseInventoryJob(soldier->GetJobType(), 1);
            world.AddFigure(soldier, bldPos);
            // Let him "walk" to goal -> Already reached -> Added
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

    /// Assert that attacking the given building from attackSrc fails
    void TestFailingAttack(MapPoint bldPos, const nobMilitary& attackSrc, unsigned numSoldiersLeft = 6u)
    {
        BOOST_REQUIRE_EQUAL(attackSrc.GetTroopsCount(), numSoldiersLeft);
        // No availbale soldiers
        BOOST_REQUIRE_EQUAL(gwv.GetNumSoldiersForAttack(bldPos), 0u);
        this->Attack(bldPos, 1, true);
        // Noone left
        BOOST_REQUIRE_EQUAL(attackSrc.GetTroopsCount(), numSoldiersLeft);
    }
};

BOOST_FIXTURE_TEST_CASE(NumSoldiersForAttack, AttackFixture)
{
    initGameRNG();

    // Connect buildings
    curPlayer = 1;
    BuildRoadForBlds(hqPos[1], milBld1Near->GetPos());
    BuildRoadForBlds(hqPos[1], milBld1Far->GetPos());
    curPlayer = 2;
    BuildRoadForBlds(hqPos[2], milBld2->GetPos());
    // Let soldiers get into blds. (6 soldiers, 6 fields distance, 20GFs per field, 30GFs for leaving HQ)
    unsigned numGFs = 6 * (6 * 20 + 30);
    // And 8 fields distance for far bld
    numGFs += 6 * (8 * 20 + 30);
    for(unsigned gf = 0; gf < numGFs; gf++)
        this->em.ExecuteNextGF();
    BOOST_REQUIRE_EQUAL(milBld1Near->GetTroopsCount(), 6u);
    BOOST_REQUIRE_EQUAL(milBld1Far->GetTroopsCount(), 6u);
    BOOST_REQUIRE_EQUAL(milBld2->GetTroopsCount(), 6u);

    // Player 0 has no military blds -> Can't attack
    SetCurPlayer(0);
    BOOST_REQUIRE_EQUAL(gwv.GetNumSoldiersForAttack(hqPos[1]), 0u);
    BOOST_REQUIRE_EQUAL(gwv.GetNumSoldiersForAttack(hqPos[2]), 0u);
    BOOST_REQUIRE_EQUAL(gwv.GetNumSoldiersForAttack(milBld1Near->GetPos()), 0u);
    BOOST_REQUIRE_EQUAL(gwv.GetNumSoldiersForAttack(milBld1Far->GetPos()), 0u);
    BOOST_REQUIRE_EQUAL(gwv.GetNumSoldiersForAttack(milBld2->GetPos()), 0u);
    SetCurPlayer(1);
    // No self attack
    BOOST_REQUIRE_EQUAL(gwv.GetNumSoldiersForAttack(hqPos[1]), 0u);
    BOOST_REQUIRE_EQUAL(gwv.GetNumSoldiersForAttack(milBld1Near->GetPos()), 0u);
    BOOST_REQUIRE_EQUAL(gwv.GetNumSoldiersForAttack(milBld1Far->GetPos()), 0u);
    // Attack both others
    BOOST_REQUIRE_EQUAL(gwv.GetNumSoldiersForAttack(hqPos[0]), 5u);
    BOOST_REQUIRE_EQUAL(gwv.GetNumSoldiersForAttack(hqPos[2]), 5u);
    // This is in the extended range of the far bld -> 2 more (with current range)
    BOOST_REQUIRE_EQUAL(gwv.GetNumSoldiersForAttack(milBld2->GetPos()), 7u);
    SetCurPlayer(2);
    // No self attack
    BOOST_REQUIRE_EQUAL(gwv.GetNumSoldiersForAttack(hqPos[2]), 0u);
    BOOST_REQUIRE_EQUAL(gwv.GetNumSoldiersForAttack(milBld2->GetPos()), 0u);
    // Attack both others
    BOOST_REQUIRE_EQUAL(gwv.GetNumSoldiersForAttack(hqPos[0]), 5u);
    BOOST_REQUIRE_EQUAL(gwv.GetNumSoldiersForAttack(hqPos[1]), 5u);
    BOOST_REQUIRE_EQUAL(gwv.GetNumSoldiersForAttack(milBld1Near->GetPos()), 5u);
    // Counterpart: 2 possible for far bld
    BOOST_REQUIRE_EQUAL(gwv.GetNumSoldiersForAttack(milBld1Far->GetPos()), 2u);

    // Functions related to stationed soldiers
    BOOST_REQUIRE(milBld1Near->HasMaxRankSoldier());
    BOOST_REQUIRE(!milBld1Far->HasMaxRankSoldier());
    BOOST_REQUIRE(milBld2->HasMaxRankSoldier());
    BOOST_REQUIRE_GT(milBld1Near->GetSoldiersStrength(), milBld1Far->GetSoldiersStrength());
}

BOOST_FIXTURE_TEST_CASE(StartAttack, AttackFixture)
{
    initGameRNG();

    world.GetPlayer(0).team = TM_TEAM1;
    world.GetPlayer(1).team = TM_TEAM2;
    world.GetPlayer(2).team = TM_TEAM1;
    for(unsigned i = 0; i < 3; i++)
        world.GetPlayer(i).MakeStartPacts();
    // Add soldiers to far and player 2 bld (3 strong, 3 weak)
    AddSoldiers(milBld1FarPos, 3, 3);
    AddSoldiers(milBld2Pos, 3, 3);

    const nobMilitary& attackSrc = *milBld2;
    const MapPoint usualBldPos = hqPos[1] + MapPoint(2, 0);
    BOOST_REQUIRE_GE(world.GetBQ(usualBldPos, 1), BQ_HUT);
    const noBuilding* usualBld = BuildingFactory::CreateBuilding(world, BLD_WOODCUTTER, usualBldPos, 1, NAT_ROMANS);
    BOOST_REQUIRE(usualBld);
    const MapPoint storehousePos = hqPos[1] - MapPoint(4, 0);
    BOOST_REQUIRE_GE(world.GetBQ(storehousePos, 1), BQ_HOUSE);
    const noBuilding* storeHouse = BuildingFactory::CreateBuilding(world, BLD_STOREHOUSE, storehousePos, 1, NAT_ROMANS);
    BOOST_REQUIRE(storeHouse);

    SetCurPlayer(attackSrc.GetPlayer());

    // Try to attack non-military bld -> Fail
    TestFailingAttack(usualBldPos, attackSrc);

    // Try to attack storehouse -> Fail
    TestFailingAttack(storehousePos, attackSrc);

    // Try to attack ally -> Fail
    TestFailingAttack(hqPos[0], attackSrc);

    // Try to attack newly build bld -> Fail
    BOOST_REQUIRE_EQUAL(world.CalcVisiblityWithAllies(milBld1NearPos, curPlayer), VIS_VISIBLE);
    BOOST_REQUIRE(milBld1Near->IsNewBuilt());
    TestFailingAttack(milBld1NearPos, attackSrc);

    // Add soldier
    AddSoldiers(milBld1NearPos, 1, 0);
    BOOST_REQUIRE(!milBld1Near->IsNewBuilt());
    // Try to attack invisible bld -> Fail
    MapNode& node = world.GetNodeWriteable(milBld1NearPos);
    node.fow[0].visibility = VIS_FOW;
    node.fow[2].visibility = VIS_FOW;
    BOOST_REQUIRE_EQUAL(world.CalcVisiblityWithAllies(milBld1NearPos, curPlayer), VIS_FOW);
    BOOST_REQUIRE_EQUAL(gwv.GetNumSoldiersForAttack(milBld1NearPos), 5u);
    this->Attack(milBld1NearPos, 1, true);
    BOOST_REQUIRE_EQUAL(attackSrc.GetTroopsCount(), 6u);

    // Attack it
    node.fow[0].visibility = VIS_VISIBLE;
    std::vector<nofPassiveSoldier*> soldiers(attackSrc.GetTroops().begin(), attackSrc.GetTroops().end());
    BOOST_REQUIRE_EQUAL(soldiers.size(), 6u);
    for(int i = 0; i < 3; i++)
        BOOST_REQUIRE_EQUAL(soldiers[i]->GetRank(), 0u);
    for(int i = 3; i < 6; i++)
        BOOST_REQUIRE_EQUAL(soldiers[i]->GetRank(), 4u);
    this->Attack(milBld1NearPos, 1, true);
    // 1 strong soldier has left
    soldiers.assign(attackSrc.GetTroops().begin(), attackSrc.GetTroops().end());
    BOOST_REQUIRE_EQUAL(soldiers.size(), 5u);
    for(int i = 0; i < 3; i++)
        BOOST_REQUIRE_EQUAL(soldiers[i]->GetRank(), 0u);
    for(int i = 3; i < 5; i++)
        BOOST_REQUIRE_EQUAL(soldiers[i]->GetRank(), 4u);
    // Attack with 1 weak soldier
    this->Attack(milBld1NearPos, 1, false);
    // 1 weak soldier has left
    soldiers.assign(attackSrc.GetTroops().begin(), attackSrc.GetTroops().end());
    BOOST_REQUIRE_EQUAL(soldiers.size(), 4u);
    for(int i = 0; i < 2; i++)
        BOOST_REQUIRE_EQUAL(soldiers[i]->GetRank(), 0u);
    for(int i = 2; i < 4; i++)
        BOOST_REQUIRE_EQUAL(soldiers[i]->GetRank(), 4u);
    // -> 2 strong, 2 weak remaining, attack with 3 weak ones -> 1 strong remaining
    this->Attack(milBld1NearPos, 3, false);
    soldiers.assign(attackSrc.GetTroops().begin(), attackSrc.GetTroops().end());
    BOOST_REQUIRE_EQUAL(soldiers.size(), 1u);
    BOOST_REQUIRE_EQUAL(soldiers[0]->GetRank(), 4u);

    // None left
    TestFailingAttack(milBld1NearPos, attackSrc, 1u);
}

BOOST_FIXTURE_TEST_CASE(ConquerBld, AttackFixture)
{
    initGameRNG();

    AddSoldiers(milBld2Pos, 1, 5);
    AddSoldiersWithRank(milBld1NearPos, 1, 0);
    AddSoldiersWithRank(milBld1NearPos, 1, 1);
    SetCurPlayer(2);
    BuildRoadForBlds(milBld2Pos, hqPos[2]);
    // Finish recruiting, carrier outhousing etc.
    for(unsigned gf = 0; gf < 400; gf++)
        em.ExecuteNextGF();
    // Start attack 2->1 (weak one first)
    this->Attack(milBld1NearPos, 1, false);
    this->Attack(milBld1NearPos, 5, false);
    BOOST_REQUIRE_EQUAL(milBld2->GetTroopsCount(), 1u);
    BOOST_REQUIRE_EQUAL(milBld1Near->GetTroopsCount(), 2u);
    // Run till attackers reach bld. 1 Soldier will leave for them.
    // 1 stays inside till an attacker is at door
    // 20 GFs/node + 30 GFs for leaving
    const unsigned distance = world.CalcDistance(milBld2Pos, milBld1NearPos);
    for(unsigned gf = 0; gf < distance * 20 + 30; gf++)
    {
        em.ExecuteNextGF();
        if(milBld1Near->GetTroopsCount() == 1)
            break;
    }
    BOOST_REQUIRE_EQUAL(milBld1Near->GetTroopsCount(), 1u);
    BOOST_REQUIRE_EQUAL(milBld1Near->GetTroopsCount() + milBld1Near->GetLeavingFigures().size(), 2u);
    const Inventory& attackedPlInventory = world.GetPlayer(1).GetInventory();
    const unsigned oldWeakSoldierCt = attackedPlInventory.people[JOB_PRIVATE];
    const unsigned oldStrongerSoldierCt = attackedPlInventory.people[JOB_PRIVATEFIRSTCLASS];
    const unsigned oldAttackerStrongSoldierCt = world.GetPlayer(2).GetInventory().people[JOB_GENERAL];

    // 1st soldier will walk towards attacker and will be killed
    // Once an attacker reaches the flag, the bld will send a defender
    BOOST_REQUIRE(!milBld1Near->GetDefender());
    for(unsigned gf = 0; gf < 300; gf++)
    {
        em.ExecuteNextGF();
        if(milBld1Near->GetTroopsCount() == 0)
            break;
    }
    BOOST_REQUIRE_EQUAL(milBld1Near->GetTroopsCount(), 0u);
    // Defender deployed, attacker at flag
    BOOST_REQUIRE(milBld1Near->GetDefender());
    const std::list<noBase*>& figures = world.GetFigures(milBld1Near->GetFlag()->GetPos());
    BOOST_REQUIRE_EQUAL(figures.size(), 1u);
    BOOST_REQUIRE(dynamic_cast<nofAttacker*>(figures.front()));
    BOOST_REQUIRE_EQUAL(static_cast<nofAttacker*>(figures.front())->GetPlayer(), 2u);

    // Lets fight
    for(unsigned gf = 0; gf < 1000; gf++)
    {
        em.ExecuteNextGF();
        if(milBld1Near->IsBeingCaptured())
            break;
    }
    BOOST_REQUIRE(milBld1Near->IsBeingCaptured());
    // Let others in
    for(unsigned gf = 0; gf < 200; gf++)
    {
        em.ExecuteNextGF();
        if(!milBld1Near->IsBeingCaptured())
            break;
    }
    // Not capturing anymore
    BOOST_REQUIRE(!milBld1Near->IsBeingCaptured());
    // Building conquered
    BOOST_REQUIRE_EQUAL(milBld1Near->GetPlayer(), 2u);
    // 1 soldier must be inside
    BOOST_REQUIRE_GT(milBld1Near->GetTroopsCount(), 1u);
    // Weak soldier must be dead
    BOOST_REQUIRE_EQUAL(attackedPlInventory.people[JOB_PRIVATE], oldWeakSoldierCt - 1);
    // Src building refill
    for(unsigned gf = 0; gf < 800; gf++)
    {
        em.ExecuteNextGF();
        if(milBld2->GetTroopsCount() == 6u)
            break;
    }
    // Src building got refilled
    BOOST_REQUIRE_EQUAL(milBld2->GetTroopsCount(), 6u);
    // We may have lost soldiers
    BOOST_REQUIRE_LE(world.GetPlayer(2).GetInventory().people[JOB_GENERAL], oldAttackerStrongSoldierCt);
    // The enemy may have lost his stronger soldier
    BOOST_REQUIRE_LE(attackedPlInventory.people[JOB_PRIVATEFIRSTCLASS], oldStrongerSoldierCt);
    // But only one
    BOOST_REQUIRE_GE(attackedPlInventory.people[JOB_PRIVATEFIRSTCLASS], oldStrongerSoldierCt - 1);
    // At least 2 survivors
    BOOST_REQUIRE_GT(milBld1Near->GetTroopsCount(), 2u);

    // Points around bld should be ours
    const std::vector<MapPoint> pts = world.GetPointsInRadius(milBld1NearPos, 3);
    BOOST_FOREACH(const MapPoint& pt, pts)
    {
        BOOST_REQUIRE_EQUAL(world.GetNode(pt).owner, 2u + 1u);
    }
}

BOOST_AUTO_TEST_SUITE_END()
