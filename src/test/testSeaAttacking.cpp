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

#include "rttrDefines.h" // IWYU pragma: keep
#include "GamePlayer.h"
#include "PointOutput.h"
#include "addons/const_addons.h"
#include "buildings/nobBaseWarehouse.h"
#include "buildings/nobHarborBuilding.h"
#include "buildings/nobMilitary.h"
#include "factories/BuildingFactory.h"
#include "figures/nofAttacker.h"
#include "figures/nofPassiveSoldier.h"
#include "helpers/containerUtils.h"
#include "pathfinding/FindPathForRoad.h"
#include "world/GameWorldViewer.h"
#include "world/MapLoader.h"
#include "nodeObjs/noFlag.h"
#include "nodeObjs/noGranite.h"
#include "nodeObjs/noShip.h"
#include "gameData/SettingTypeConv.h"
#include "gameData/TerrainDesc.h"
#include "test/SeaWorldWithGCExecution.h"
#include "test/initTestHelpers.h"
#include <boost/foreach.hpp>
#include <boost/test/unit_test.hpp>
#include <iostream>

BOOST_AUTO_TEST_SUITE(SeaAttackSuite)

// Size is chosen based on current maximum attacking distances!
struct SeaAttackFixture : public SeaWorldWithGCExecution<3, 62, 64>
{
    typedef SeaWorldWithGCExecution<3, 62, 64> Parent;
    using Parent::curPlayer;
    using Parent::ggs;
    using Parent::world;

    /// Positions of the players HQ
    boost::array<MapPoint, 3> hqPos;
    /// Tested positions for military buildings
    MapPoint milBld1NearPos, milBld1FarPos, milBld2Pos;
    boost::array<MapPoint, 3> harborPos;
    /// Military buildings of players 1 and 2 (for 1, one is close and one far away from player 1)
    const nobMilitary *milBld1Near, *milBld1Far, *milBld2;
    GameWorldViewer gwv;

    SeaAttackFixture() : gwv(curPlayer, world)
    {
        // Make sure attacking is not limited by visibility
        RTTR_FOREACH_PT(MapPoint, world.GetSize())
        {
            world.SetVisibility(pt, 2, VIS_VISIBLE);
        }

        // Block diagonals with granite so no human path is possible
        for(MapCoord i = 1; i < std::min(world.GetWidth(), world.GetHeight()); i++)
        {
            MapPoint pt(i, i);
            if(world.GetNode(pt).bq != BQ_NOTHING)
            {
                world.SetNO(pt, new noGranite(GT_1, 5));
                if(pt.x + 1 < world.GetWidth())
                    world.SetNO(pt + MapPoint(1, 0), new noGranite(GT_1, 5));
                world.RecalcBQAroundPointBig(pt);
            }
            pt = MapPoint(world.GetHeight() - i - 1u, i);
            if(pt.x < world.GetWidth() && world.GetNode(pt).bq != BQ_NOTHING)
            {
                world.SetNO(pt, new noGranite(GT_1, 5));
                if(pt.x + 1 < world.GetWidth())
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
            const noBuilding* hb = BuildingFactory::CreateBuilding(world, BLD_HARBORBUILDING, harborPos[i], i, Nation(i));
            BOOST_REQUIRE(hb);
            BuildRoadForBlds(harborPos[i], hqPos[i]);
            MapPoint shipPos = world.GetCoastalPoint(world.GetHarborPointID(harborPos[i]), 1);
            shipPos = world.MakeMapPoint(Position(shipPos) + (Position(shipPos) - Position(harborPos[i])) * 8);
            BOOST_REQUIRE(shipPos.isValid());
            noShip* ship = new noShip(shipPos, i);
            world.AddFigure(shipPos, ship);
            world.GetPlayer(i).RegisterShip(ship);
        }

        // Build some military buildings

        milBld1NearPos = FindBldPos(world.GetHarborPoint(3) + MapPoint(3, 2), BQ_HOUSE, 1);
        BOOST_REQUIRE(milBld1NearPos.isValid());
        BOOST_REQUIRE_GE(world.GetBQ(milBld1NearPos, 1), BQ_HOUSE);
        milBld1Near = dynamic_cast<nobMilitary*>(BuildingFactory::CreateBuilding(world, BLD_WATCHTOWER, milBld1NearPos, 1, NAT_ROMANS));
        BOOST_REQUIRE(milBld1Near);

        milBld1FarPos = FindBldPos(world.GetHarborPoint(4) - MapPoint(1, 4), BQ_HOUSE, 1);
        BOOST_REQUIRE(milBld1FarPos.isValid());
        BOOST_REQUIRE_GE(world.GetBQ(milBld1FarPos, 1), BQ_HOUSE);
        milBld1Far = dynamic_cast<nobMilitary*>(BuildingFactory::CreateBuilding(world, BLD_WATCHTOWER, milBld1FarPos, 1, NAT_ROMANS));
        BOOST_REQUIRE(milBld1Far);

        milBld2Pos = FindBldPos(world.GetHarborPoint(6) - MapPoint(2, 2), BQ_HOUSE, 2);
        BOOST_REQUIRE(milBld2Pos.isValid());
        BOOST_REQUIRE_GE(world.GetBQ(milBld2Pos, 2), BQ_HOUSE);
        milBld2 = dynamic_cast<nobMilitary*>(BuildingFactory::CreateBuilding(world, BLD_WATCHTOWER, milBld2Pos, 2, NAT_BABYLONIANS));
        BOOST_REQUIRE(milBld2);

        // Add some soldiers (assumed by test cases!)
        AddSoldiers(milBld2Pos, 3, 3);
        AddSoldiers(milBld1FarPos, 6, 0);
        // Skip near bld so we have an empty one for testing

        SetCurPlayer(2);
        // Enemy harbors don't block
        ggs.setSelection(AddonId::SEA_ATTACK, 0);
    }

    struct HasBQ
    {
        const World& world;
        unsigned player;
        BuildingQuality bq;
        HasBQ(const World& world, unsigned player, BuildingQuality bq) : world(world), player(player), bq(bq) {}

        bool operator()(const MapPoint& pt) const { return world.GetBQ(pt, player) >= bq; }
    };

    MapPoint FindBldPos(const MapPoint& preferedPos, BuildingQuality reqBQ, unsigned player)
    {
        std::vector<MapPoint> pts = world.GetPointsInRadius<1>(preferedPos, 2, Identity<MapPoint>(), HasBQ(world, player, reqBQ), true);
        if(!pts.empty())
            return pts[0];
        return MapPoint::Invalid();
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
        gwv.ChangePlayer(playerIdx, false);
    }

    void AddSoldiersWithRank(MapPoint bldPos, unsigned numSoldiers, unsigned rank)
    {
        BOOST_REQUIRE_LE(rank, world.GetGGS().GetMaxMilitaryRank());
        nobMilitary* bld = world.GetSpecObj<nobMilitary>(bldPos);
        BOOST_REQUIRE(bld);
        const unsigned oldNumSoldiers = bld->GetNumTroops();
        for(unsigned i = 0; i < numSoldiers; i++)
        {
            nofPassiveSoldier* soldier = new nofPassiveSoldier(bldPos, bld->GetPlayer(), bld, bld, rank);
            world.GetPlayer(bld->GetPlayer()).IncreaseInventoryJob(soldier->GetJobType(), 1);
            world.AddFigure(bldPos, soldier);
            // Let him "walk" to goal -> Already reached -> Added
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

    /// Assert that attacking the given building from milBld2 fails
    void TestFailingSeaAttack(MapPoint bldPos, unsigned numSoldiersLeft = 6u)
    {
        BOOST_REQUIRE_EQUAL(milBld2->GetNumTroops(), numSoldiersLeft);
        // No available soldiers
        BOOST_REQUIRE_EQUAL(gwv.GetNumSoldiersForSeaAttack(bldPos), 0u);
        this->SeaAttack(bldPos, 1, true);
        // Same left
        BOOST_REQUIRE_EQUAL(milBld2->GetNumTroops(), numSoldiersLeft);
    }

    /// Asserts that the milBld2 contains the given amount of strong/weak soldiers
    void TestTroopCt(unsigned numStrong, unsigned numWeak)
    {
        std::vector<nofPassiveSoldier*> soldiers(milBld2->GetTroops().begin(), milBld2->GetTroops().end());
        BOOST_REQUIRE_EQUAL(soldiers.size(), numStrong + numWeak);
        for(unsigned i = 0; i < numWeak; i++)
            BOOST_REQUIRE_EQUAL(soldiers[i]->GetRank(), 0u);
        for(unsigned i = numWeak; i < soldiers.size(); i++)
            BOOST_REQUIRE_EQUAL(soldiers[i]->GetRank(), 4u);
    }
};

BOOST_FIXTURE_TEST_CASE(SeaAttackDisabled, SeaAttackFixture)
{
    AddSoldiers(milBld1NearPos, 6, 0);

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

BOOST_FIXTURE_TEST_CASE(NoHarborBlock, SeaAttackFixture)
{
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
    const noBuilding* usualBld = BuildingFactory::CreateBuilding(world, BLD_WOODCUTTER, harborPos[0], 0, NAT_ROMANS);
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
    BOOST_REQUIRE_EQUAL(milBld2->GetNumTroops(), 5u);

    this->SeaAttack(hqPos[1], 1, true);
    BOOST_REQUIRE_EQUAL(milBld2->GetNumTroops(), 4u);

    this->SeaAttack(milBld1NearPos, 1, true);
    BOOST_REQUIRE_EQUAL(milBld2->GetNumTroops(), 3u);

    this->SeaAttack(harborPos[1], 2, true);
    BOOST_REQUIRE_EQUAL(milBld2->GetNumTroops(), 1u);

    // All gone
    TestFailingSeaAttack(harborPos[1], 1u);
}

BOOST_FIXTURE_TEST_CASE(HarborsBlock, SeaAttackFixture)
{
    AddSoldiers(milBld1NearPos, 6, 0);

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
    const noBuilding* usualBld = BuildingFactory::CreateBuilding(world, BLD_WOODCUTTER, harborPos[0], 0, NAT_ROMANS);
    BOOST_REQUIRE(usualBld);
    TestFailingSeaAttack(harborPos[0]);

    // HQ Attackable as no harbor at harbor spot (but regular bld)
    BOOST_REQUIRE_EQUAL(gwv.GetNumSoldiersForSeaAttack(hqPos[0]), 5u);
    // Harbor is always attackable
    BOOST_REQUIRE_EQUAL(gwv.GetNumSoldiersForSeaAttack(harborPos[1]), 5u);
    // HQ
    this->SeaAttack(hqPos[0], 1, true);
    BOOST_REQUIRE_EQUAL(milBld2->GetNumTroops(), 5u);
    // Harbor
    this->SeaAttack(harborPos[1], 1, true);
    BOOST_REQUIRE_EQUAL(milBld2->GetNumTroops(), 4u);

    // Test for unowned harbor
    const MapPoint harborBot1 = world.GetHarborPoint(7);
    const MapPoint harborBot2 = world.GetHarborPoint(8);
    BOOST_REQUIRE_EQUAL(world.GetNode(harborBot1).owner, 0u);
    BOOST_REQUIRE_EQUAL(world.GetNode(harborBot2).owner, 0u);
    // Place military bld for player 1, first make some owned land
    const MapPoint bldPos = (harborBot1 + harborBot2) / MapCoord(2) + MapPoint(6, 0);
    // Distance: <= attack distance (+ reserve) but greater than range from military bld (current values)
    BOOST_REQUIRE_LE(world.CalcDistance(bldPos, harborBot2), 12u);
    BOOST_REQUIRE_GE(world.CalcDistance(bldPos, harborBot2), 9u);
    BOOST_REQUIRE_EQUAL(world.GetNode(bldPos).bq, BQ_CASTLE);
    std::vector<MapPoint> pts = world.GetPointsInRadius(bldPos, 3);
    pts.push_back(bldPos);
    BOOST_FOREACH(const MapPoint& pt, pts)
        world.SetOwner(pt, 1 + 1);
    const noBuilding* bld = BuildingFactory::CreateBuilding(world, BLD_BARRACKS, bldPos, 1, NAT_ROMANS);
    BOOST_REQUIRE(bld);
    AddSoldiers(bldPos, 2, 0);
    // Still unowned harbors
    BOOST_REQUIRE_EQUAL(world.GetNode(harborBot1).owner, 0u);
    BOOST_REQUIRE_EQUAL(world.GetNode(harborBot2).owner, 0u);
    // But we can attack the building
    BOOST_REQUIRE_EQUAL(gwv.GetNumSoldiersForSeaAttack(bldPos), 3u);
    this->SeaAttack(bldPos, 1, true);
    BOOST_REQUIRE_EQUAL(milBld2->GetNumTroops(), 3u);
}

BOOST_FIXTURE_TEST_CASE(AttackWithTeams, SeaAttackFixture)
{
    AddSoldiers(milBld1NearPos, 6, 0);

    // Enemy harbors block
    ggs.setSelection(AddonId::SEA_ATTACK, 1);

    // Build (later) ally harbor
    noBuilding* harborBld = BuildingFactory::CreateBuilding(world, BLD_HARBORBUILDING, harborPos[0], 1, NAT_ROMANS);
    BOOST_REQUIRE(harborBld);

    // Enemy harbor blocks
    TestFailingSeaAttack(hqPos[1]);

    world.GetPlayer(0).team = TM_TEAM2; //-V525
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
    world.SetVisibility(hqPos[0], 1, VIS_VISIBLE);

    // Attackable
    BOOST_REQUIRE_EQUAL(gwv.GetNumSoldiersForSeaAttack(hqPos[0]), 5u);
    this->SeaAttack(hqPos[0], 1, true);
    BOOST_REQUIRE_EQUAL(milBld2->GetNumTroops(), 5u);
}

BOOST_FIXTURE_TEST_CASE(AttackHarbor, SeaAttackFixture)
{
    initGameRNG();

    BuildRoadForBlds(milBld2Pos, harborPos[2]);
    BuildRoadForBlds(milBld2Pos, hqPos[2]);
    // Add 1 soldier to dest harbor so we have a defender
    nobHarborBuilding& hbDest = *world.GetSpecObj<nobHarborBuilding>(harborPos[1]);
    Inventory newGoods;
    newGoods.Add(JOB_SERGEANT, 1);
    hbDest.AddGoods(newGoods, true);
    // Don't keep him in reserve
    hbDest.SetRealReserve(JOB_SERGEANT - JOB_PRIVATE, 0);
    BOOST_REQUIRE_EQUAL(hbDest.GetNumVisualFigures(JOB_SERGEANT), 1u);
    BOOST_REQUIRE(!hbDest.GetDefender());

    const noShip& ship = *world.GetPlayer(2).GetShipByID(0);
    const nobHarborBuilding& hbSrc = *world.GetSpecObj<nobHarborBuilding>(harborPos[2]);

    // Start attack (1 strong first) -> Soldiers walk to harbor, ship does nothing yet
    this->SeaAttack(harborPos[1], 1, true);
    this->SeaAttack(harborPos[1], 4, true);
    BOOST_REQUIRE_EQUAL(milBld2->GetNumTroops(), 1u);
    BOOST_REQUIRE(ship.IsIdling());
    BOOST_REQUIRE_EQUAL(hbSrc.GetNumVisualFigures(JOB_GENERAL), 0u);

    // Let soldier walk to harbor
    unsigned distance = 2 * world.CalcDistance(milBld2Pos, harborPos[2]);
    RTTR_EXEC_TILL(distance * 20 + 30, !ship.IsIdling());
    BOOST_REQUIRE(ship.IsGoingToHarbor(hbSrc));
    BOOST_REQUIRE_EQUAL(hbSrc.GetNumVisualFigures(JOB_GENERAL), 1u);

    // Let ship arrive and count soldiers that arrived so far
    unsigned numSoldiersForAttack = 0;
    for(unsigned gf = 0; gf < 60;)
    {
        numSoldiersForAttack = hbSrc.GetNumVisualFigures(JOB_PRIVATE) + hbSrc.GetNumVisualFigures(JOB_GENERAL);
        unsigned numGfs = em.ExecuteNextEvent();
        if(numGfs == 0)
            break;
        gf += numGfs;
        if(ship.IsOnAttackMission())
            break;
    }
    BOOST_REQUIRE(ship.IsOnAttackMission());
    // Ship should have picked up all soldiers
    BOOST_REQUIRE_EQUAL(hbSrc.GetNumVisualFigures(JOB_PRIVATE), 0u);
    BOOST_REQUIRE_EQUAL(hbSrc.GetNumVisualFigures(JOB_GENERAL), 0u);
    // min. 2 soldiers, or attack success is not save. Place ship further away if required
    BOOST_REQUIRE_GE(numSoldiersForAttack, 2u);
    // Note: This might be off by one if the ship arrived in the same GF a soldier arrived. But this is unlikely
    BOOST_REQUIRE_EQUAL(ship.GetFigures().size(), numSoldiersForAttack); //-V807
    BOOST_FOREACH(const noFigure* attacker, ship.GetFigures())
    {
        BOOST_REQUIRE(!dynamic_cast<const nofAttacker&>(*attacker).IsSeaAttackCompleted());
    }

    // Ship loads, then starts driving (currently 200GF)
    RTTR_EXEC_TILL(200, ship.IsMoving());
    BOOST_REQUIRE(ship.IsOnAttackMission());
    // This should not have been changed
    BOOST_REQUIRE_EQUAL(ship.GetFigures().size(), numSoldiersForAttack);
    // Ship does NOT go to any harbor, as that one is an intermediate spot only
    BOOST_REQUIRE(!ship.IsGoingToHarbor(hbDest));
    BOOST_REQUIRE(!ship.IsGoingToHarbor(hbSrc));
    // Ship must not yet be at target harbor
    const MapPoint targetPt = world.GetCoastalPoint(hbDest.GetHarborPosID(), ship.GetSeaID());
    distance = world.CalcDistance(ship.GetPos(), targetPt);
    BOOST_REQUIRE_GT(distance, 2u);
    // Ship goes to enemy harbor coast and starts unloading
    RTTR_EXEC_TILL(distance * 20 + 20, !ship.IsMoving());
    BOOST_REQUIRE(ship.IsOnAttackMission());
    BOOST_REQUIRE_EQUAL(ship.GetPos(), targetPt);
    // This should not (yet) have been changed
    BOOST_REQUIRE_EQUAL(ship.GetFigures().size(), numSoldiersForAttack);

    // Now soldiers get out one by one
    for(unsigned curNumSoldiers = 1; curNumSoldiers <= numSoldiersForAttack; curNumSoldiers++)
    {
        for(unsigned gf = 0; gf < 40; gf++)
        {
            em.ExecuteNextGF();
            if(ship.GetFigures().size() + curNumSoldiers <= numSoldiersForAttack)
                break;
        }
        BOOST_REQUIRE_EQUAL(ship.GetFigures().size() + curNumSoldiers, numSoldiersForAttack);
    }
    // All out
    BOOST_REQUIRE_EQUAL(ship.GetFigures().size(), 0u);

    // Harbor might already be destroyed for fast fights and many attackers. Add more defenders for that case
    BOOST_REQUIRE(world.GetSpecObj<nobHarborBuilding>(harborPos[1]));
    // Soldiers go to enemy harbor and engage defender
    RTTR_EXEC_TILL(100, hbDest.GetDefender());
    // Harbor still valid
    BOOST_REQUIRE(world.GetSpecObj<nobHarborBuilding>(harborPos[1]));
    // Defender sent
    BOOST_REQUIRE(hbDest.GetDefender());

    // Eventually the harbor gets destroyed
    RTTR_EXEC_TILL(1000, world.GetNO(harborPos[1])->GetGOT() == GOT_FIRE);

    // Collect remaining attackers, so we know how many survived
    std::vector<MapPoint> pts = world.GetPointsInRadius(harborPos[1], 5);
    pts.push_back(harborPos[1]);
    std::vector<nofAttacker*> attackers;
    std::vector<unsigned> attackerIds;
    BOOST_FOREACH(const MapPoint& pt, pts)
    {
        BOOST_FOREACH(noBase* fig, world.GetFigures(pt))
        {
            nofAttacker* attacker = dynamic_cast<nofAttacker*>(fig);
            if(attacker)
            {
                BOOST_REQUIRE_EQUAL(attacker->GetPlayer(), 2u);
                attackers.push_back(attacker);
                // Save those for later, as attackers get deleted
                attackerIds.push_back(attacker->GetObjId());
            }
        }
    }
    // We must have at least 1 and at most the start count of soldiers remaining
    BOOST_REQUIRE_GE(attackers.size(), 1u);
    BOOST_REQUIRE_LE(attackers.size(), numSoldiersForAttack);

    // Ship waits till all attackers have returned and leaves then
    RTTR_EXEC_TILL(100, ship.IsMoving());
    BOOST_REQUIRE(ship.IsOnAttackMission());
    BOOST_REQUIRE(ship.IsGoingToHarbor(hbSrc));
    BOOST_REQUIRE_EQUAL(ship.GetFigures().size(), attackers.size());
    BOOST_FOREACH(const noFigure* attacker, ship.GetFigures())
    {
        BOOST_REQUIRE(dynamic_cast<const nofAttacker&>(*attacker).IsSeaAttackCompleted());
    }

    // Ship returns home and starts unloading
    RTTR_EXEC_TILL(distance * 20 + 20, !ship.IsMoving());
    BOOST_REQUIRE(ship.IsOnAttackMission());
    BOOST_REQUIRE(ship.IsGoingToHarbor(hbSrc));

    // And should finish this in currently 200GF
    RTTR_EXEC_TILL(200, ship.IsIdling());
    BOOST_REQUIRE(!ship.IsOnAttackMission());

    // Finally troops should return home (and missing ones replaced by HQ)
    RTTR_EXEC_TILL(1000, milBld2->GetNumTroops() == 6u);
    // All troups should have returned home
    std::vector<nofPassiveSoldier*> soldiers(milBld2->GetTroops().begin(), milBld2->GetTroops().end());
    unsigned numTroopsFound = 0;
    BOOST_FOREACH(const nofPassiveSoldier* soldier, soldiers)
    {
        if(helpers::contains(attackerIds, soldier->GetObjId()))
            numTroopsFound++;
    }
    BOOST_REQUIRE_EQUAL(numTroopsFound, attackerIds.size());
    // Make sure we don't have duplicate IDs (soldiers get converted active<->passive)
    for(unsigned i = 0; i < soldiers.size(); i++)
    {
        for(unsigned j = i + 1; j < soldiers.size(); j++)
            BOOST_REQUIRE_NE(soldiers[i]->GetObjId(), soldiers[j]->GetObjId());
    }

    // All soldiers should have left
    BOOST_REQUIRE_EQUAL(hbSrc.GetNumVisualFigures(JOB_PRIVATE), 0u);
    BOOST_REQUIRE_EQUAL(hbSrc.GetNumVisualFigures(JOB_GENERAL), 0u);
}

BOOST_FIXTURE_TEST_CASE(HarborBlocksSpots, SeaAttackFixture)
{
    DescIdx<TerrainDesc> tWater(0);
    for(; tWater.value < world.GetDescription().terrain.size(); tWater.value++)
    {
        if(world.GetDescription().get(tWater).kind == TerrainKind::WATER && !world.GetDescription().get(tWater).Is(ETerrain::Walkable))
            break;
    }
    DescIdx<TerrainDesc> tLand(0);
    for(; tLand.value < world.GetDescription().terrain.size(); tLand.value++)
    {
        if(world.GetDescription().get(tLand).kind == TerrainKind::LAND && world.GetDescription().get(tLand).Is(ETerrain::Walkable))
            break;
    }

    // Issue: A harbor is a castle-sized building and blocks the nodes W, NW, NE
    // If the NW node is selected as the corresponding seas coastal position, we cannot attack that harbor as the walking path would go over
    // the harbor or a blocked point if we can't walk around it Harbors is attackable by default
    BOOST_REQUIRE_EQUAL(gwv.GetNumSoldiersForSeaAttack(harborPos[1]), 5u);
    // build such a situation for player 1 (left)
    // Make everything west of harbor water
    for(MapPoint pt = harborPos[1] - MapPoint(0, 3); pt.y < harborPos[1].y + 3; pt.y++)
    {
        for(pt.x = harborPos[1].x - 3; pt.x <= harborPos[1].x; pt.x++)
        {
            if(world.CalcDistance(pt, harborPos[1]) < 2)
                continue;
            world.GetNodeWriteable(pt).t1 = tWater;
            world.GetNodeWriteable(pt).t2 = tWater;
        }
    }
    const MapPoint ptW = world.GetNeighbour(harborPos[1], Direction::WEST);
    // Make west point a non-coastal point by adding land to its sea points
    const MapPoint seaPtW = world.GetNeighbour(ptW, Direction::WEST);
    const MapPoint seaPtW2 = world.GetNeighbour(ptW, Direction::NORTHWEST);
    world.GetNodeWriteable(seaPtW).t2 = tLand;
    world.GetNodeWriteable(seaPtW2).t1 = tLand;
    // Make NE pt a sea point
    const MapPoint ptNE = world.GetNeighbour(harborPos[1], Direction::NORTHEAST);
    const MapPoint seaPtN = world.GetNeighbour(ptNE, Direction::NORTHWEST);
    world.GetNodeWriteable(seaPtN).t1 = tWater;
    world.GetNodeWriteable(seaPtN).t2 = tWater;

    const MapPoint ptNW = world.GetNeighbour(harborPos[1], Direction::NORTHWEST);
    BOOST_REQUIRE(world.IsWaterPoint(world.GetNeighbour(ptNW, Direction::NORTHWEST)));
    BOOST_REQUIRE(world.IsWaterPoint(seaPtN));
    // Re-init seas/harbors
    BOOST_REQUIRE(MapLoader::InitSeasAndHarbors(world));

    // Still have our harbor
    const unsigned hbId = world.GetHarborPointID(harborPos[1]);
    BOOST_REQUIRE(hbId);
    // Should have coast at NE (NW is skipped)
    BOOST_REQUIRE_EQUAL(world.GetSeaId(hbId, Direction::WEST), 0u);
    BOOST_REQUIRE_EQUAL(world.GetSeaId(hbId, Direction::NORTHWEST), 0u);
    BOOST_REQUIRE_EQUAL(world.GetSeaId(hbId, Direction::NORTHEAST), 1u);
    BOOST_REQUIRE_EQUAL(world.GetSeaId(hbId, Direction::EAST), 0u);
    BOOST_REQUIRE_EQUAL(world.GetSeaId(hbId, Direction::SOUTHEAST), 0u);
    BOOST_REQUIRE_EQUAL(world.GetSeaId(hbId, Direction::SOUTHWEST), 0u);

    // So we should still be able to attack
    BOOST_REQUIRE_EQUAL(gwv.GetNumSoldiersForSeaAttack(harborPos[1]), 5u);
    // Start attack
    this->SeaAttack(harborPos[1], 5, true);
    // Let soldier walk to harbor
    unsigned distance = 2 * world.CalcDistance(milBld2Pos, harborPos[2]);
    RTTR_SKIP_GFS(distance * 20 + 30);
    // And ship go to harbor (+200 GFs for loading)
    const noShip& ship = *world.GetPlayer(2).GetShipByID(0);
    distance = world.CalcDistance(ship.GetPos(), harborPos[1]);
    for(unsigned gf = 0; gf < distance * 2 * 20 + 200;)
    {
        unsigned numGFs = em.ExecuteNextEvent();
        if(numGFs == 0)
            break;
        gf += numGFs;
        if(!ship.IsMoving() && world.CalcDistance(ship.GetPos(), harborPos[1]) <= 2)
            break;
    }
    BOOST_REQUIRE(ship.IsOnAttackMission());
    BOOST_REQUIRE(!ship.IsMoving());
    BOOST_REQUIRE_LE(world.CalcDistance(ship.GetPos(), harborPos[1]), 2u);
    // Harbor should be destroyed and the ship go back
    RTTR_EXEC_TILL(500, ship.IsMoving());
    BOOST_REQUIRE_EQUAL(world.GetNO(harborPos[1])->GetGOT(), GOT_FIRE);
}

BOOST_AUTO_TEST_SUITE_END()
