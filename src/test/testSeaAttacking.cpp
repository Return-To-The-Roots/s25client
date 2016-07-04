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
#include "test/SeaWorldWithGCExecution.h"
#include "factories/BuildingFactory.h"
#include "pathfinding/FindPathForRoad.h"
#include "figures/nofPassiveSoldier.h"
#include "figures/nofAttacker.h"
#include "buildings/nobBaseWarehouse.h"
#include "buildings/nobMilitary.h"
#include "nodeObjs/noFlag.h"
#include "nodeObjs/noGranite.h"
#include "nodeObjs/noShip.h"
#include "GamePlayer.h"
#include "world/GameWorldViewer.h"
#include "gameData/SettingTypeConv.h"
#include <boost/test/unit_test.hpp>
#include <boost/foreach.hpp>

BOOST_AUTO_TEST_SUITE(SeaAttackSuite)

// Size is choosen based on current maximum attacking distances!
struct AttackFixture: public SeaWorldWithGCExecution
{
    typedef SeaWorldWithGCExecution Parent;
    using Parent::world;
    using Parent::curPlayer;
    using Parent::ggs;

    /// Positions of the players HQ
    boost::array<MapPoint, 3> hqPos;
    /// Tested positions for military buildings
    MapPoint milBld1NearPos, milBld1FarPos, milBld2Pos;
    boost::array<MapPoint, 3> harborPos;
    /// Military buildings of players 1 and 2 (for 1, one is close and one far away from player 1)
    const nobMilitary *milBld1Near, *milBld1Far, *milBld2;
    GameWorldViewer gwv;

    AttackFixture(): gwv(curPlayer, world)
    {
        // Make sure attacking is not limited by visibility
        RTTR_FOREACH_PT(MapPoint, world.GetWidth(), world.GetHeight())
        {
            world.SetVisibility(pt, 2, VIS_VISIBLE, this->em.GetCurrentGF());
        }

        // Block diagonals with granite so no human path is possible
        for(MapCoord i = 1; i < world.GetWidth() - 1; i++)
        {
            MapPoint pt(i, i);
            if(world.GetNode(pt).bq != BQ_NOTHING)
            {
                world.SetNO(pt, new noGranite(GT_1, 5));
                world.SetNO(pt + MapPoint(1, 0), new noGranite(GT_1, 5));
                world.RecalcBQAroundPointBig(pt);
            }
            pt = MapPoint(world.GetHeight() - i - 1u, i);
            if(world.GetNode(pt).bq != BQ_NOTHING)
            {
                world.SetNO(pt, new noGranite(GT_1, 5));
                world.SetNO(pt + MapPoint(1, 0), new noGranite(GT_1, 5));
                world.RecalcBQAroundPointBig(pt);
            }
        }

        for(unsigned i = 0; i < 3; i++)
        {
            SetCurPlayer(i);
            hqPos[i] = world.GetPlayer(i).GetHQPos();
            nobBaseWarehouse* hq = world.GetSpecObj<nobBaseWarehouse>(hqPos[i]);
            Inventory goods;
            goods.Add(JOB_GENERAL, 3);
            hq->AddGoods(goods, true);
            this->ChangeMilitary(MILITARY_SETTINGS_SCALE);
        }
        // Assert player positions: 0: Top, 1: Left, 2: Right
        BOOST_REQUIRE_LT(hqPos[0].y, hqPos[1].y);
        BOOST_REQUIRE_LT(hqPos[0].y, hqPos[2].y);
        BOOST_REQUIRE_GT(hqPos[0].x, hqPos[1].x);
        BOOST_REQUIRE_LT(hqPos[1].x, hqPos[2].x);

        // Harbors must be at same sea
        BOOST_REQUIRE(world.IsHarborAtSea(1, 1));
        BOOST_REQUIRE(world.IsHarborAtSea(3, 1));
        BOOST_REQUIRE(world.IsHarborAtSea(6, 1));
        harborPos[0] = world.GetHarborPoint(1);
        harborPos[1] = world.GetHarborPoint(3);
        harborPos[2] = world.GetHarborPoint(6);
        // Place harbors (Player 0 has none)
        for(unsigned i = 1; i < 3; i++)
        {
            SetCurPlayer(i);
            BOOST_REQUIRE_EQUAL(gwv.GetBQ(harborPos[i]), BQ_HARBOR);
            const noBuilding* hb = BuildingFactory::CreateBuilding(&world, BLD_HARBORBUILDING, harborPos[i], i, Nation(i));
            BOOST_REQUIRE(hb);
            BuildRoadForBlds(harborPos[i], hqPos[i]);
            const MapPoint shipPos = world.GetCoastalPoint(world.GetHarborPointID(harborPos[i]), 1);
            BOOST_REQUIRE(shipPos.isValid());
            noShip* ship = new noShip(shipPos, i);
            world.AddFigure(ship, shipPos);
            world.GetPlayer(i).RegisterShip(ship);
        }

        // Build some military buildings

        milBld1NearPos = world.GetHarborPoint(3) + MapPoint(3, 2);
        BOOST_REQUIRE_GE(world.GetBQ(milBld1NearPos, 1), BQ_HOUSE);
        milBld1Near = dynamic_cast<nobMilitary*>(BuildingFactory::CreateBuilding(&world, BLD_WATCHTOWER, milBld1NearPos, 1, NAT_ROMANS));
        BOOST_REQUIRE(milBld1Near);

        milBld1FarPos = world.GetHarborPoint(4) - MapPoint(1, 2);
        BOOST_REQUIRE_GE(world.GetBQ(milBld1FarPos, 1), BQ_HOUSE);
        milBld1Far = dynamic_cast<nobMilitary*>(BuildingFactory::CreateBuilding(&world, BLD_WATCHTOWER, milBld1FarPos, 1, NAT_ROMANS));
        BOOST_REQUIRE(milBld1Far);

        milBld2Pos = world.GetHarborPoint(6) - MapPoint(3, 2);
        BOOST_REQUIRE_GE(world.GetBQ(milBld2Pos, 2), BQ_HOUSE);
        milBld2 = dynamic_cast<nobMilitary*>(BuildingFactory::CreateBuilding(&world, BLD_WATCHTOWER, milBld2Pos, 2, NAT_BABYLONIANS));
        BOOST_REQUIRE(milBld2);
    }

    /// Constructs a road connecting 2 buildings and checks for success
    void BuildRoadForBlds(const MapPoint bldPosFrom, const MapPoint bldPosTo)
    {
        const MapPoint start = world.GetNeighbour(bldPosFrom, Direction::SOUTHEAST);
        const MapPoint end = world.GetNeighbour(bldPosTo, Direction::SOUTHEAST);
        std::vector<unsigned char> road = FindPathForRoad(world, start, end, world, false);
        BOOST_REQUIRE(!road.empty());
        this->BuildRoad(start, false, road);
        BOOST_REQUIRE_EQUAL(world.GetPointRoad(start, road.front()), RoadSegment::RT_NORMAL + 1);
    }

    void SetCurPlayer(unsigned playerIdx)
    {
        curPlayer = playerIdx;
        gwv.ChangePlayer(playerIdx, false);
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

    /// Assert that attacking the given building from milBld2 fails
    void TestFailingSeaAttack(MapPoint bldPos, unsigned numSoldiersLeft = 6u)
    {
        BOOST_REQUIRE_EQUAL(milBld2->GetTroopsCount(), numSoldiersLeft);
        // No availbale soldiers
        BOOST_REQUIRE_EQUAL(gwv.GetNumSoldiersForSeaAttack(bldPos), 0u);
        this->SeaAttack(bldPos, 1, true);
        // Noone left
        BOOST_REQUIRE_EQUAL(milBld2->GetTroopsCount(), numSoldiersLeft);
    }
};

BOOST_FIXTURE_TEST_CASE(SeaAttackDisabled, AttackFixture)
{
    AddSoldiers(milBld1NearPos, 6, 0);
    AddSoldiers(milBld1FarPos, 6, 0);
    AddSoldiers(milBld2Pos, 3, 3);

    SetCurPlayer(2);
    // Disable Sea attack
    ggs.setSelection(AddonId::SEA_ATTACK, 2);

    // No land attack
    BOOST_REQUIRE_EQUAL(gwv.GetNumSoldiersForAttack(hqPos[0]), 0u);
    BOOST_REQUIRE_EQUAL(gwv.GetNumSoldiersForAttack(hqPos[1]), 0u);
    BOOST_REQUIRE_EQUAL(gwv.GetNumSoldiersForAttack(harborPos[0]), 0u);
    BOOST_REQUIRE_EQUAL(gwv.GetNumSoldiersForAttack(harborPos[1]), 0u);
    BOOST_REQUIRE_EQUAL(gwv.GetNumSoldiersForAttack(milBld1NearPos), 0u);
    BOOST_REQUIRE_EQUAL(gwv.GetNumSoldiersForAttack(milBld1FarPos), 0u);

    // No sea attack
    TestFailingSeaAttack(hqPos[0]);
    TestFailingSeaAttack(hqPos[1]);
    TestFailingSeaAttack(harborPos[1]);
    TestFailingSeaAttack(milBld1NearPos);
    TestFailingSeaAttack(milBld1FarPos);
}

BOOST_FIXTURE_TEST_CASE(NoHarborBlock, AttackFixture)
{
    AddSoldiers(milBld1FarPos, 6, 0);
    AddSoldiers(milBld2Pos, 3, 3);

    SetCurPlayer(2);
    // Enemy harbors don't block
    ggs.setSelection(AddonId::SEA_ATTACK, 0);
    // No land attack
    BOOST_REQUIRE_EQUAL(gwv.GetNumSoldiersForAttack(hqPos[0]), 0u);
    BOOST_REQUIRE_EQUAL(gwv.GetNumSoldiersForAttack(hqPos[1]), 0u);
    BOOST_REQUIRE_EQUAL(gwv.GetNumSoldiersForAttack(harborPos[0]), 0u);
    BOOST_REQUIRE_EQUAL(gwv.GetNumSoldiersForAttack(harborPos[1]), 0u);
    BOOST_REQUIRE_EQUAL(gwv.GetNumSoldiersForAttack(milBld1NearPos), 0u);
    BOOST_REQUIRE_EQUAL(gwv.GetNumSoldiersForAttack(milBld1FarPos), 0u);

    // No building
    TestFailingSeaAttack(harborPos[0]);

    // Non-Military building
    BOOST_REQUIRE_GT(world.GetBQ(harborPos[0], 0), BQ_FLAG);
    const noBuilding* usualBld = BuildingFactory::CreateBuilding(&world, BLD_WOODCUTTER, harborPos[0], 0, NAT_ROMANS);
    BOOST_REQUIRE(usualBld);
    TestFailingSeaAttack(harborPos[0]);

    // Bld to far
    TestFailingSeaAttack(milBld1FarPos);

    // Bld not occupied
    TestFailingSeaAttack(milBld1NearPos);
    // Occupy now
    AddSoldiers(milBld1NearPos, 6, 0);

    // Rest is attackable
    BOOST_REQUIRE_EQUAL(gwv.GetNumSoldiersForSeaAttack(hqPos[0]), 5u);
    BOOST_REQUIRE_EQUAL(gwv.GetNumSoldiersForSeaAttack(hqPos[1]), 5u);
    BOOST_REQUIRE_EQUAL(gwv.GetNumSoldiersForSeaAttack(milBld1NearPos), 5u);
    BOOST_REQUIRE_EQUAL(gwv.GetNumSoldiersForSeaAttack(harborPos[1]), 5u);
    this->SeaAttack(hqPos[0], 1, true);
    BOOST_REQUIRE_EQUAL(milBld2->GetTroopsCount(), 5u);

    this->SeaAttack(hqPos[1], 1, true);
    BOOST_REQUIRE_EQUAL(milBld2->GetTroopsCount(), 4u);

    this->SeaAttack(milBld1NearPos, 1, true);
    BOOST_REQUIRE_EQUAL(milBld2->GetTroopsCount(), 3u);

    this->SeaAttack(harborPos[1], 2, true);
    BOOST_REQUIRE_EQUAL(milBld2->GetTroopsCount(), 1u);

    // All gone
    TestFailingSeaAttack(harborPos[1], 1u);
}

BOOST_FIXTURE_TEST_CASE(HarborsBlock, AttackFixture)
{
    AddSoldiers(milBld1NearPos, 6, 0);
    AddSoldiers(milBld1FarPos, 6, 0);
    AddSoldiers(milBld2Pos, 3, 3);

    SetCurPlayer(2);
    // Enemy harbors block
    ggs.setSelection(AddonId::SEA_ATTACK, 1);
    // No land attack
    BOOST_REQUIRE_EQUAL(gwv.GetNumSoldiersForAttack(hqPos[0]), 0u);
    BOOST_REQUIRE_EQUAL(gwv.GetNumSoldiersForAttack(hqPos[1]), 0u);
    BOOST_REQUIRE_EQUAL(gwv.GetNumSoldiersForAttack(harborPos[0]), 0u);
    BOOST_REQUIRE_EQUAL(gwv.GetNumSoldiersForAttack(harborPos[1]), 0u);
    BOOST_REQUIRE_EQUAL(gwv.GetNumSoldiersForAttack(milBld1NearPos), 0u);
    BOOST_REQUIRE_EQUAL(gwv.GetNumSoldiersForAttack(milBld1FarPos), 0u);

    // Harbor exists -> Can't attack
    TestFailingSeaAttack(hqPos[1]);
    TestFailingSeaAttack(milBld1NearPos);
    TestFailingSeaAttack(milBld1FarPos);

    // No building
    TestFailingSeaAttack(harborPos[0]);
    // Non-Military building
    BOOST_REQUIRE_GT(world.GetBQ(harborPos[0], 0), BQ_FLAG);
    const noBuilding* usualBld = BuildingFactory::CreateBuilding(&world, BLD_WOODCUTTER, harborPos[0], 0, NAT_ROMANS);
    BOOST_REQUIRE(usualBld);
    TestFailingSeaAttack(harborPos[0]);

    // HQ Attackable as no harbor at harbor spot (but regular bld)
    BOOST_REQUIRE_EQUAL(gwv.GetNumSoldiersForSeaAttack(hqPos[0]), 5u);
    // Harbor is always attackable
    BOOST_REQUIRE_EQUAL(gwv.GetNumSoldiersForSeaAttack(harborPos[1]), 5u);
    // HQ
    this->SeaAttack(hqPos[0], 1, true);
    BOOST_REQUIRE_EQUAL(milBld2->GetTroopsCount(), 5u);
    // Harbor
    this->SeaAttack(harborPos[1], 1, true);
    BOOST_REQUIRE_EQUAL(milBld2->GetTroopsCount(), 4u);

    // Test for unowned harbor
    const MapPoint harborBot1 = world.GetHarborPoint(7);
    const MapPoint harborBot2 = world.GetHarborPoint(8);
    BOOST_REQUIRE_EQUAL(world.GetNode(harborBot1).owner, 0u);
    BOOST_REQUIRE_EQUAL(world.GetNode(harborBot2).owner, 0u);
    // Place military bld for player 1, first make some owned land
    const MapPoint bldPos = (harborBot1 + harborBot2) / 2u + MapPoint(6, 0);
    // Distance: <= attack distance (+ reserve) but greater than range from military bld (current values)
    BOOST_REQUIRE_LE(world.CalcDistance(bldPos, harborBot2), 12u);
    BOOST_REQUIRE_GE(world.CalcDistance(bldPos, harborBot2), 9u);
    BOOST_REQUIRE_EQUAL(world.GetNode(bldPos).bq, BQ_CASTLE);
    std::vector<MapPoint> pts = world.GetPointsInRadius(bldPos, 3);
    pts.push_back(bldPos);
    BOOST_FOREACH(const MapPoint& pt, pts)
        world.SetOwner(pt, 1 + 1);
    const noBuilding* bld = BuildingFactory::CreateBuilding(&world, BLD_BARRACKS, bldPos, 1, NAT_ROMANS);
    AddSoldiers(bldPos, 2, 0);
    // Still unowned harbors
    BOOST_REQUIRE_EQUAL(world.GetNode(harborBot1).owner, 0u);
    BOOST_REQUIRE_EQUAL(world.GetNode(harborBot2).owner, 0u);
    // But we can attack the building
    BOOST_REQUIRE_EQUAL(gwv.GetNumSoldiersForSeaAttack(bldPos), 3u);
    this->SeaAttack(bldPos, 1, true);
    BOOST_REQUIRE_EQUAL(milBld2->GetTroopsCount(), 3u);
}

BOOST_FIXTURE_TEST_CASE(AttackWithTeams, AttackFixture)
{
    AddSoldiers(milBld1NearPos, 6, 0);
    AddSoldiers(milBld1FarPos, 6, 0);
    AddSoldiers(milBld2Pos, 3, 3);

    SetCurPlayer(2);
    // Enemy harbors block
    ggs.setSelection(AddonId::SEA_ATTACK, 1);

    // Build (later) ally harbor
    noBuilding* harborBld = BuildingFactory::CreateBuilding(&world, BLD_HARBORBUILDING, harborPos[0], 1, NAT_ROMANS);
    BOOST_REQUIRE(harborBld);

    // Enemy harbor blocks
    TestFailingSeaAttack(hqPos[1]);

    world.GetPlayer(0).team = TM_TEAM2;
    world.GetPlayer(1).team = TM_TEAM1;
    world.GetPlayer(2).team = TM_TEAM1;
    for(unsigned i = 0; i < 3; i++)
        world.GetPlayer(i).MakeStartPacts();

    // Can't attack ally
    TestFailingSeaAttack(harborPos[1]);
    TestFailingSeaAttack(hqPos[1]);

    // Invisible point
    world.SetVisibility(hqPos[0], 1, VIS_FOW, em.GetCurrentGF());
    world.SetVisibility(hqPos[0], 2, VIS_FOW, em.GetCurrentGF());
    TestFailingSeaAttack(hqPos[0]);
    // Visible for ally
    world.SetVisibility(hqPos[0], 1, VIS_VISIBLE, em.GetCurrentGF());
    
    // Attackable
    BOOST_REQUIRE_EQUAL(gwv.GetNumSoldiersForSeaAttack(hqPos[0]), 5u);
    this->SeaAttack(hqPos[0], 1, true);
}

BOOST_AUTO_TEST_SUITE_END()
