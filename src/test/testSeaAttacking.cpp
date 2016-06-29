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
};

BOOST_FIXTURE_TEST_CASE(NumSoldiersForSeaAttack, AttackFixture)
{
    AddSoldiers(milBld1NearPos, 6, 0);
    AddSoldiers(milBld1FarPos, 6, 0);
    AddSoldiers(milBld2Pos, 6, 0);

    SetCurPlayer(2);
    // No land attack
    BOOST_REQUIRE_EQUAL(gwv.GetNumSoldiersForAttack(hqPos[0]), 0u);
    BOOST_REQUIRE_EQUAL(gwv.GetNumSoldiersForAttack(hqPos[1]), 0u);
    BOOST_REQUIRE_EQUAL(gwv.GetNumSoldiersForAttack(harborPos[0]), 0u);
    BOOST_REQUIRE_EQUAL(gwv.GetNumSoldiersForAttack(harborPos[1]), 0u);
    BOOST_REQUIRE_EQUAL(gwv.GetNumSoldiersForAttack(milBld1NearPos), 0u);
    BOOST_REQUIRE_EQUAL(gwv.GetNumSoldiersForAttack(milBld1FarPos), 0u);

    // Disable Sea attack
    ggs.setSelection(AddonId::SEA_ATTACK, 2);
    BOOST_REQUIRE_EQUAL(gwv.GetNumSoldiersForSeaAttack(hqPos[0]), 0u);
    BOOST_REQUIRE_EQUAL(gwv.GetNumSoldiersForSeaAttack(hqPos[1]), 0u);
    BOOST_REQUIRE_EQUAL(gwv.GetNumSoldiersForSeaAttack(harborPos[1]), 0u);
    BOOST_REQUIRE_EQUAL(gwv.GetNumSoldiersForSeaAttack(milBld1NearPos), 0u);
    BOOST_REQUIRE_EQUAL(gwv.GetNumSoldiersForSeaAttack(milBld1FarPos), 0u);

    // Enemy harbors block
    ggs.setSelection(AddonId::SEA_ATTACK, 1);
    BOOST_REQUIRE_EQUAL(gwv.GetNumSoldiersForSeaAttack(hqPos[0]), 5u);
    // Harbor exists -> Can't attack
    BOOST_REQUIRE_EQUAL(gwv.GetNumSoldiersForSeaAttack(hqPos[1]), 0u);
    BOOST_REQUIRE_EQUAL(gwv.GetNumSoldiersForSeaAttack(milBld1NearPos), 0u);
    BOOST_REQUIRE_EQUAL(gwv.GetNumSoldiersForSeaAttack(milBld1FarPos), 0u);
    BOOST_REQUIRE_EQUAL(gwv.GetNumSoldiersForSeaAttack(harborPos[1]), 5u);

    // Enemy harbors don't block
    ggs.setSelection(AddonId::SEA_ATTACK, 0);
    BOOST_REQUIRE_EQUAL(gwv.GetNumSoldiersForSeaAttack(hqPos[0]), 5u);
    BOOST_REQUIRE_EQUAL(gwv.GetNumSoldiersForSeaAttack(hqPos[1]), 5u);
    BOOST_REQUIRE_EQUAL(gwv.GetNumSoldiersForSeaAttack(milBld1NearPos), 5u);
    BOOST_REQUIRE_EQUAL(gwv.GetNumSoldiersForSeaAttack(milBld1FarPos), 0u);
    BOOST_REQUIRE_EQUAL(gwv.GetNumSoldiersForSeaAttack(harborPos[1]), 5u);
    // And still no land attack
    BOOST_REQUIRE_EQUAL(gwv.GetNumSoldiersForAttack(hqPos[0]), 0u);
    BOOST_REQUIRE_EQUAL(gwv.GetNumSoldiersForAttack(hqPos[1]), 0u);
    BOOST_REQUIRE_EQUAL(gwv.GetNumSoldiersForAttack(harborPos[0]), 0u);
    BOOST_REQUIRE_EQUAL(gwv.GetNumSoldiersForAttack(harborPos[1]), 0u);
    BOOST_REQUIRE_EQUAL(gwv.GetNumSoldiersForAttack(milBld1NearPos), 0u);
    BOOST_REQUIRE_EQUAL(gwv.GetNumSoldiersForAttack(milBld1FarPos), 0u);

    // No building
    BOOST_REQUIRE_EQUAL(gwv.GetNumSoldiersForSeaAttack(harborPos[0]), 0u);
    // Non-Military building
    BOOST_REQUIRE_GT(world.GetBQ(harborPos[0], 0), BQ_FLAG);
    const noBuilding* usualBld = BuildingFactory::CreateBuilding(&world, BLD_WOODCUTTER, harborPos[0], 0, NAT_ROMANS);
    BOOST_REQUIRE(usualBld);
    BOOST_REQUIRE_EQUAL(gwv.GetNumSoldiersForSeaAttack(harborPos[0]), 0u);
    // But we can still attack the HQ as that is not a harbor at the harbor spot
    ggs.setSelection(AddonId::SEA_ATTACK, 1);
    BOOST_REQUIRE_EQUAL(gwv.GetNumSoldiersForSeaAttack(hqPos[0]), 5u);

    // Test teams
    ggs.setSelection(AddonId::SEA_ATTACK, 0);
    world.GetPlayer(0).team = TM_TEAM1;
    world.GetPlayer(1).team = TM_TEAM2;
    world.GetPlayer(2).team = TM_TEAM1;
    for(unsigned i = 0; i < 3; i++)
        world.GetPlayer(i).MakeStartPacts();
    BOOST_REQUIRE_EQUAL(gwv.GetNumSoldiersForSeaAttack(hqPos[0]), 0u);
    BOOST_REQUIRE_EQUAL(gwv.GetNumSoldiersForSeaAttack(hqPos[1]), 5u);
    // Invisible point
    world.SetVisibility(hqPos[1], 2, VIS_FOW, em.GetCurrentGF());
    BOOST_REQUIRE_EQUAL(gwv.GetNumSoldiersForSeaAttack(hqPos[1]), 0u);
    // Visible for ally
    world.SetVisibility(hqPos[1], 0, VIS_VISIBLE, em.GetCurrentGF());
    BOOST_REQUIRE_EQUAL(gwv.GetNumSoldiersForSeaAttack(hqPos[1]), 5u);

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
    std::vector<MapPoint> pts =  world.GetPointsInRadius(bldPos, 3);
    pts.push_back(bldPos);
    BOOST_FOREACH(const MapPoint& pt, pts)
        world.SetOwner(pt, 1 + 1);
    const noBuilding* bld = BuildingFactory::CreateBuilding(&world, BLD_BARRACKS, bldPos, 1, NAT_ROMANS);
    AddSoldiers(bldPos, 2, 0);
    // Still unowned harbors
    BOOST_REQUIRE_EQUAL(world.GetNode(harborBot1).owner, 0u);
    BOOST_REQUIRE_EQUAL(world.GetNode(harborBot2).owner, 0u);
    // But we can attack the building
    BOOST_REQUIRE_EQUAL(gwv.GetNumSoldiersForSeaAttack(bldPos), 5u);

    // Ally harbors don't block
    ggs.setSelection(AddonId::SEA_ATTACK, 1);
    // Enemy harbor is still blocking
    BOOST_REQUIRE_EQUAL(gwv.GetNumSoldiersForSeaAttack(hqPos[1]), 0u);
    world.DestroyNO(harborPos[1]);
    // Wait till fire vanishes
    for(unsigned gf = 0; gf < 5000; gf++)
    {
        em.ExecuteNextGF();
        if(!world.GetNode(harborPos[1]).obj)
            break;
    }
    BOOST_REQUIRE(!world.GetNode(harborPos[1]).obj);
    // Build ally harbor
    noBuilding* harborBld = BuildingFactory::CreateBuilding(&world, BLD_HARBORBUILDING, harborPos[1], 0, NAT_ROMANS);
    BOOST_REQUIRE(harborBld);
    // Attackable
    BOOST_REQUIRE_EQUAL(gwv.GetNumSoldiersForSeaAttack(hqPos[1]), 5u);
}

BOOST_AUTO_TEST_SUITE_END()
