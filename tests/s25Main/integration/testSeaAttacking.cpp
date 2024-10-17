// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "GamePlayer.h"
#include "PointOutput.h"
#include "RttrForeachPt.h"
#include "addons/const_addons.h"
#include "buildings/nobBaseWarehouse.h"
#include "buildings/nobHarborBuilding.h"
#include "buildings/nobMilitary.h"
#include "factories/BuildingFactory.h"
#include "figures/nofAttacker.h"
#include "figures/nofPassiveSoldier.h"
#include "helpers/containerUtils.h"
#include "pathfinding/FindPathForRoad.h"
#include "worldFixtures/SeaWorldWithGCExecution.h"
#include "worldFixtures/initGameRNG.hpp"
#include "worldFixtures/terrainHelpers.h"
#include "world/GameWorldViewer.h"
#include "world/MapLoader.h"
#include "nodeObjs/noGranite.h"
#include "nodeObjs/noShip.h"
#include "gameTypes/GameTypesOutput.h"
#include "gameData/SettingTypeConv.h"
#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_SUITE(SeaAttackSuite)

// Size is chosen based on current maximum attacking distances!
struct SeaAttackFixture : public SeaWorldWithGCExecution<3, 62, 64>
{
    using Parent = SeaWorldWithGCExecution<3, 62, 64>;
    using Parent::curPlayer;
    using Parent::ggs;
    using Parent::world;

    /// Positions of the players HQ
    std::array<MapPoint, 3> hqPos;
    /// Tested positions for military buildings
    MapPoint milBld1NearPos, milBld1FarPos, milBld2Pos;
    std::array<MapPoint, 3> harborPos;
    /// Military buildings of players 1 and 2 (for 1, one is close and one far away from player 1)
    const nobMilitary *milBld1Near, *milBld1Far, *milBld2;
    GameWorldViewer gwv;

    SeaAttackFixture() : gwv(curPlayer, world)
    {
        // Make sure attacking is not limited by visibility
        RTTR_FOREACH_PT(MapPoint, world.GetSize())
        {
            world.SetVisibility(pt, 2, Visibility::Visible);
        }

        // Block diagonals with granite so no human path is possible
        for(MapCoord i = 1; i < std::min(world.GetWidth(), world.GetHeight()); i++)
        {
            MapPoint pt(i, i);
            if(world.GetNode(pt).bq != BuildingQuality::Nothing)
            {
                world.SetNO(pt, new noGranite(GraniteType::One, 5));
                if(pt.x + 1 < world.GetWidth())
                    world.SetNO(pt + MapPoint(1, 0), new noGranite(GraniteType::One, 5));
                world.RecalcBQAroundPointBig(pt);
            }
            pt = MapPoint(world.GetHeight() - i - 1u, i);
            if(pt.x < world.GetWidth() && world.GetNode(pt).bq != BuildingQuality::Nothing)
            {
                world.SetNO(pt, new noGranite(GraniteType::One, 5));
                if(pt.x + 1 < world.GetWidth())
                    world.SetNO(pt + MapPoint(1, 0), new noGranite(GraniteType::One, 5));
                world.RecalcBQAroundPointBig(pt);
            }
        }

        for(unsigned i = 0; i < 3; i++)
        {
            SetCurPlayer(i);
            hqPos[i] = world.GetPlayer(i).GetHQPos();
            auto* hq = world.GetSpecObj<nobBaseWarehouse>(hqPos[i]);
            Inventory goods;
            goods.Add(Job::General, 3);
            hq->AddGoods(goods, true);
            this->ChangeMilitary(MILITARY_SETTINGS_SCALE);
        }
        // Assert player positions: 0: Top, 1: Left, 2: Right
        BOOST_TEST_REQUIRE(hqPos[0].y < hqPos[1].y);
        BOOST_TEST_REQUIRE(hqPos[0].y < hqPos[2].y);
        BOOST_TEST_REQUIRE(hqPos[0].x > hqPos[1].x);
        BOOST_TEST_REQUIRE(hqPos[1].x < hqPos[2].x);

        // Harbors must be at same sea
        BOOST_TEST_REQUIRE(world.IsHarborAtSea(1, 1));
        BOOST_TEST_REQUIRE(world.IsHarborAtSea(3, 1));
        BOOST_TEST_REQUIRE(world.IsHarborAtSea(6, 1));
        harborPos[0] = world.GetHarborPoint(1);
        harborPos[1] = world.GetHarborPoint(3);
        harborPos[2] = world.GetHarborPoint(6);
        // Place harbors (Player 0 has none)
        for(unsigned i = 1; i < 3; i++)
        {
            SetCurPlayer(i);
            BOOST_TEST_REQUIRE(gwv.GetBQ(harborPos[i]) == BuildingQuality::Harbor);
            const noBuilding* hb =
              BuildingFactory::CreateBuilding(world, BuildingType::HarborBuilding, harborPos[i], i, Nation(i));
            BOOST_TEST_REQUIRE(hb);
            BuildRoadForBlds(harborPos[i], hqPos[i]);
            MapPoint shipPos = world.GetCoastalPoint(world.GetHarborPointID(harborPos[i]), 1);
            shipPos = world.MakeMapPoint(Position(shipPos) + (Position(shipPos) - Position(harborPos[i])) * 8);
            BOOST_TEST_REQUIRE(shipPos.isValid());
            auto& ship = world.AddFigure(shipPos, std::make_unique<noShip>(shipPos, i));
            world.GetPlayer(i).RegisterShip(ship);
        }

        // Build some military buildings

        milBld1NearPos = FindBldPos(world.GetHarborPoint(3) + MapPoint(3, 2), BuildingQuality::House, 1);
        BOOST_TEST_REQUIRE(milBld1NearPos.isValid());
        BOOST_TEST_REQUIRE(world.GetBQ(milBld1NearPos, 1) >= BuildingQuality::House);
        milBld1Near = dynamic_cast<nobMilitary*>(
          BuildingFactory::CreateBuilding(world, BuildingType::Watchtower, milBld1NearPos, 1, Nation::Romans));
        BOOST_TEST_REQUIRE(milBld1Near);

        milBld1FarPos = FindBldPos(world.GetHarborPoint(4) - MapPoint(1, 4), BuildingQuality::House, 1);
        BOOST_TEST_REQUIRE(milBld1FarPos.isValid());
        BOOST_TEST_REQUIRE(world.GetBQ(milBld1FarPos, 1) >= BuildingQuality::House);
        milBld1Far = dynamic_cast<nobMilitary*>(
          BuildingFactory::CreateBuilding(world, BuildingType::Watchtower, milBld1FarPos, 1, Nation::Romans));
        BOOST_TEST_REQUIRE(milBld1Far);

        milBld2Pos = FindBldPos(world.GetHarborPoint(6) - MapPoint(2, 2), BuildingQuality::House, 2);
        BOOST_TEST_REQUIRE(milBld2Pos.isValid());
        BOOST_TEST_REQUIRE(world.GetBQ(milBld2Pos, 2) >= BuildingQuality::House);
        milBld2 = dynamic_cast<nobMilitary*>(
          BuildingFactory::CreateBuilding(world, BuildingType::Watchtower, milBld2Pos, 2, Nation::Babylonians));
        BOOST_TEST_REQUIRE(milBld2);

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
        const std::vector<MapPoint> pts =
          world.GetMatchingPointsInRadius<1>(preferedPos, 2, HasBQ(world, player, reqBQ), true);
        return pts.at(0);
    }

    /// Constructs a road connecting 2 buildings and checks for success
    void BuildRoadForBlds(const MapPoint bldPosFrom, const MapPoint bldPosTo)
    {
        const MapPoint start = world.GetNeighbour(bldPosFrom, Direction::SouthEast);
        const MapPoint end = world.GetNeighbour(bldPosTo, Direction::SouthEast);
        std::vector<Direction> road = FindPathForRoad(world, start, end, false);
        BOOST_TEST_REQUIRE(!road.empty());
        this->BuildRoad(start, false, road);
        BOOST_TEST_REQUIRE(world.GetPointRoad(start, road.front()) == PointRoad::Normal);
    }

    void SetCurPlayer(unsigned playerIdx)
    {
        curPlayer = playerIdx;
        gwv.ChangePlayer(playerIdx, false);
    }

    void AddSoldiersWithRank(MapPoint bldPos, unsigned numSoldiers, unsigned rank)
    {
        BOOST_TEST_REQUIRE(rank <= world.GetGGS().GetMaxMilitaryRank());
        auto* bld = world.GetSpecObj<nobMilitary>(bldPos);
        BOOST_TEST_REQUIRE(bld);
        const unsigned oldNumSoldiers = bld->GetNumTroops();
        for(unsigned i = 0; i < numSoldiers; i++)
        {
            auto& soldier =
              world.AddFigure(bldPos, std::make_unique<nofPassiveSoldier>(bldPos, bld->GetPlayer(), bld, bld, rank));
            world.GetPlayer(bld->GetPlayer()).IncreaseInventoryJob(soldier.GetJobType(), 1);
            // Let him "walk" to goal -> Already reached -> Added and all internal states set correctly
            soldier.WalkToGoal();
            BOOST_TEST_REQUIRE(soldier.HasNoGoal());
        }
        BOOST_TEST_REQUIRE(bld->GetNumTroops() == oldNumSoldiers + numSoldiers);
    }

    void AddSoldiers(MapPoint bldPos, unsigned numWeak, unsigned numStrong)
    {
        AddSoldiersWithRank(bldPos, numWeak, 0);
        AddSoldiersWithRank(bldPos, numStrong, 4);
    }

    /// Assert that attacking the given building from milBld2 fails
    void TestFailingSeaAttack(MapPoint bldPos, unsigned numSoldiersLeft = 6u)
    {
        BOOST_TEST_REQUIRE(milBld2->GetNumTroops() == numSoldiersLeft);
        // No available soldiers
        BOOST_TEST_REQUIRE(gwv.GetNumSoldiersForSeaAttack(bldPos) == 0u);
        this->SeaAttack(bldPos, 1, true);
        // Same left
        BOOST_TEST_REQUIRE(milBld2->GetNumTroops() == numSoldiersLeft);
    }

    /// Asserts that the milBld2 contains the given amount of strong/weak soldiers
    void TestTroopCt(unsigned numStrong, unsigned numWeak) const
    {
        BOOST_TEST_REQUIRE(milBld2->GetNumTroops() == numStrong + numWeak);
        auto itTroops = milBld2->GetTroops().begin();
        for(unsigned i = 0; i < numWeak; i++, ++itTroops)
            BOOST_TEST_REQUIRE(itTroops->GetRank() == 0u);
        for(unsigned i = 0; i < numStrong; i++, ++itTroops)
            BOOST_TEST_REQUIRE(itTroops->GetRank() == 4u);
    }
};

BOOST_FIXTURE_TEST_CASE(SeaAttackDisabled, SeaAttackFixture)
{
    AddSoldiers(milBld1NearPos, 6, 0);

    // Disable Sea attack
    ggs.setSelection(AddonId::SEA_ATTACK, 2);

    // No land attack
    BOOST_TEST_REQUIRE(gwv.GetNumSoldiersForAttack(hqPos[0]) == 0u);
    BOOST_TEST_REQUIRE(gwv.GetNumSoldiersForAttack(hqPos[1]) == 0u);
    BOOST_TEST_REQUIRE(gwv.GetNumSoldiersForAttack(harborPos[0]) == 0u);
    BOOST_TEST_REQUIRE(gwv.GetNumSoldiersForAttack(harborPos[1]) == 0u);
    BOOST_TEST_REQUIRE(gwv.GetNumSoldiersForAttack(milBld1NearPos) == 0u);
    BOOST_TEST_REQUIRE(gwv.GetNumSoldiersForAttack(milBld1FarPos) == 0u);

    // No sea attack
    TestFailingSeaAttack(hqPos[0]);
    TestFailingSeaAttack(hqPos[1]);
    TestFailingSeaAttack(harborPos[1]);
    TestFailingSeaAttack(milBld1NearPos);
    TestFailingSeaAttack(milBld1FarPos);
}

BOOST_FIXTURE_TEST_CASE(SeaAttackInPeacefulMode, SeaAttackFixture)
{
    AddSoldiers(milBld1NearPos, 6, 0);

    // Set peaceful mode
    ggs.setSelection(AddonId::PEACEFULMODE, 1);

    // No land attack
    BOOST_TEST_REQUIRE(gwv.GetNumSoldiersForAttack(hqPos[0]) == 0u);
    BOOST_TEST_REQUIRE(gwv.GetNumSoldiersForAttack(hqPos[1]) == 0u);
    BOOST_TEST_REQUIRE(gwv.GetNumSoldiersForAttack(harborPos[0]) == 0u);
    BOOST_TEST_REQUIRE(gwv.GetNumSoldiersForAttack(harborPos[1]) == 0u);
    BOOST_TEST_REQUIRE(gwv.GetNumSoldiersForAttack(milBld1NearPos) == 0u);
    BOOST_TEST_REQUIRE(gwv.GetNumSoldiersForAttack(milBld1FarPos) == 0u);

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
    BOOST_TEST_REQUIRE(gwv.GetNumSoldiersForAttack(hqPos[0]) == 0u);
    BOOST_TEST_REQUIRE(gwv.GetNumSoldiersForAttack(hqPos[1]) == 0u);
    BOOST_TEST_REQUIRE(gwv.GetNumSoldiersForAttack(harborPos[0]) == 0u);
    BOOST_TEST_REQUIRE(gwv.GetNumSoldiersForAttack(harborPos[1]) == 0u);
    BOOST_TEST_REQUIRE(gwv.GetNumSoldiersForAttack(milBld1NearPos) == 0u);
    BOOST_TEST_REQUIRE(gwv.GetNumSoldiersForAttack(milBld1FarPos) == 0u);

    // No building
    TestFailingSeaAttack(harborPos[0]);

    // Non-Military building
    BOOST_TEST_REQUIRE(world.GetBQ(harborPos[0], 0) > BuildingQuality::Flag);
    const noBuilding* usualBld =
      BuildingFactory::CreateBuilding(world, BuildingType::Woodcutter, harborPos[0], 0, Nation::Romans);
    BOOST_TEST_REQUIRE(usualBld);
    TestFailingSeaAttack(harborPos[0]);

    // Bld to far
    TestFailingSeaAttack(milBld1FarPos);

    // Bld not occupied
    TestFailingSeaAttack(milBld1NearPos);
    // Occupy now
    AddSoldiers(milBld1NearPos, 6, 0);

    // Rest is attackable
    BOOST_TEST_REQUIRE(gwv.GetNumSoldiersForSeaAttack(hqPos[0]) == 5u);
    BOOST_TEST_REQUIRE(gwv.GetNumSoldiersForSeaAttack(hqPos[1]) == 5u);
    BOOST_TEST_REQUIRE(gwv.GetNumSoldiersForSeaAttack(milBld1NearPos) == 5u);
    BOOST_TEST_REQUIRE(gwv.GetNumSoldiersForSeaAttack(harborPos[1]) == 5u);
    this->SeaAttack(hqPos[0], 1, true);
    BOOST_TEST_REQUIRE(milBld2->GetNumTroops() == 5u);

    this->SeaAttack(hqPos[1], 1, true);
    BOOST_TEST_REQUIRE(milBld2->GetNumTroops() == 4u);

    this->SeaAttack(milBld1NearPos, 1, true);
    BOOST_TEST_REQUIRE(milBld2->GetNumTroops() == 3u);

    this->SeaAttack(harborPos[1], 2, true);
    BOOST_TEST_REQUIRE(milBld2->GetNumTroops() == 1u);

    // All gone
    TestFailingSeaAttack(harborPos[1], 1u);
}

BOOST_FIXTURE_TEST_CASE(HarborsBlock, SeaAttackFixture)
{
    AddSoldiers(milBld1NearPos, 6, 0);

    // Enemy harbors block
    ggs.setSelection(AddonId::SEA_ATTACK, 1);
    // No land attack
    BOOST_TEST_REQUIRE(gwv.GetNumSoldiersForAttack(hqPos[0]) == 0u);
    BOOST_TEST_REQUIRE(gwv.GetNumSoldiersForAttack(hqPos[1]) == 0u);
    BOOST_TEST_REQUIRE(gwv.GetNumSoldiersForAttack(harborPos[0]) == 0u);
    BOOST_TEST_REQUIRE(gwv.GetNumSoldiersForAttack(harborPos[1]) == 0u);
    BOOST_TEST_REQUIRE(gwv.GetNumSoldiersForAttack(milBld1NearPos) == 0u);
    BOOST_TEST_REQUIRE(gwv.GetNumSoldiersForAttack(milBld1FarPos) == 0u);

    // Harbor exists -> Can't attack
    TestFailingSeaAttack(hqPos[1]);
    TestFailingSeaAttack(milBld1NearPos);
    TestFailingSeaAttack(milBld1FarPos);

    // No building
    TestFailingSeaAttack(harborPos[0]);
    // Non-Military building
    BOOST_TEST_REQUIRE(world.GetBQ(harborPos[0], 0) > BuildingQuality::Flag);
    const noBuilding* usualBld =
      BuildingFactory::CreateBuilding(world, BuildingType::Woodcutter, harborPos[0], 0, Nation::Romans);
    BOOST_TEST_REQUIRE(usualBld);
    TestFailingSeaAttack(harborPos[0]);

    // HQ Attackable as no harbor at harbor spot (but regular bld)
    BOOST_TEST_REQUIRE(gwv.GetNumSoldiersForSeaAttack(hqPos[0]) == 5u);
    // Harbor is always attackable
    BOOST_TEST_REQUIRE(gwv.GetNumSoldiersForSeaAttack(harborPos[1]) == 5u);
    // HQ
    this->SeaAttack(hqPos[0], 1, true);
    BOOST_TEST_REQUIRE(milBld2->GetNumTroops() == 5u);
    // Harbor
    this->SeaAttack(harborPos[1], 1, true);
    BOOST_TEST_REQUIRE(milBld2->GetNumTroops() == 4u);

    // Test for unowned harbor
    const MapPoint harborBot1 = world.GetHarborPoint(7);
    const MapPoint harborBot2 = world.GetHarborPoint(8);
    BOOST_TEST_REQUIRE(world.GetNode(harborBot1).owner == 0u);
    BOOST_TEST_REQUIRE(world.GetNode(harborBot2).owner == 0u);
    // Place military bld for player 1, first make some owned land
    const MapPoint bldPos = (harborBot1 + harborBot2) / MapCoord(2) + MapPoint(6, 0);
    // Distance: <= attack distance (+ reserve) but greater than range from military bld (current values)
    BOOST_TEST_REQUIRE(world.CalcDistance(bldPos, harborBot2) <= 12u);
    BOOST_TEST_REQUIRE(world.CalcDistance(bldPos, harborBot2) >= 9u);
    BOOST_TEST_REQUIRE(world.GetNode(bldPos).bq == BuildingQuality::Castle);
    std::vector<MapPoint> pts = world.GetPointsInRadius(bldPos, 3);
    pts.push_back(bldPos);
    for(const MapPoint& pt : pts)
        world.SetOwner(pt, 1 + 1);
    const noBuilding* bld = BuildingFactory::CreateBuilding(world, BuildingType::Barracks, bldPos, 1, Nation::Romans);
    BOOST_TEST_REQUIRE(bld);
    AddSoldiers(bldPos, 2, 0);
    // Still unowned harbors
    BOOST_TEST_REQUIRE(world.GetNode(harborBot1).owner == 0u);
    BOOST_TEST_REQUIRE(world.GetNode(harborBot2).owner == 0u);
    // But we can attack the building
    BOOST_TEST_REQUIRE(gwv.GetNumSoldiersForSeaAttack(bldPos) == 3u);
    this->SeaAttack(bldPos, 1, true);
    BOOST_TEST_REQUIRE(milBld2->GetNumTroops() == 3u);
}

BOOST_FIXTURE_TEST_CASE(AttackWithTeams, SeaAttackFixture)
{
    AddSoldiers(milBld1NearPos, 6, 0);

    // Enemy harbors block
    ggs.setSelection(AddonId::SEA_ATTACK, 1);

    // Build (later) ally harbor
    noBuilding* harborBld =
      BuildingFactory::CreateBuilding(world, BuildingType::HarborBuilding, harborPos[0], 1, Nation::Romans);
    BOOST_TEST_REQUIRE(harborBld);

    // Enemy harbor blocks
    TestFailingSeaAttack(hqPos[1]);

    world.GetPlayer(0).team = Team::Team2; //-V525
    world.GetPlayer(1).team = Team::Team1;
    world.GetPlayer(2).team = Team::Team1;
    for(unsigned i = 0; i < 3; i++)
        world.GetPlayer(i).MakeStartPacts();

    // Can't attack ally
    TestFailingSeaAttack(harborPos[1]);
    TestFailingSeaAttack(hqPos[1]);

    // Invisible point
    world.SetVisibility(hqPos[0], 1, Visibility::FogOfWar, em.GetCurrentGF());
    world.SetVisibility(hqPos[0], 2, Visibility::FogOfWar, em.GetCurrentGF());
    TestFailingSeaAttack(hqPos[0]);
    // Visible for ally
    world.SetVisibility(hqPos[0], 1, Visibility::Visible);

    // Attackable
    BOOST_TEST_REQUIRE(gwv.GetNumSoldiersForSeaAttack(hqPos[0]) == 5u);
    this->SeaAttack(hqPos[0], 1, true);
    BOOST_TEST_REQUIRE(milBld2->GetNumTroops() == 5u);
}

BOOST_FIXTURE_TEST_CASE(AttackHarbor, SeaAttackFixture)
{
    initGameRNG();

    BuildRoadForBlds(milBld2Pos, harborPos[2]);
    BuildRoadForBlds(milBld2Pos, hqPos[2]);
    // Add 1 soldier to dest harbor so we have a defender
    nobHarborBuilding& hbDest = *world.GetSpecObj<nobHarborBuilding>(harborPos[1]);
    Inventory newGoods;
    newGoods.Add(Job::Sergeant, 1);
    hbDest.AddGoods(newGoods, true);
    // Don't keep him in reserve
    hbDest.SetRealReserve(getSoldierRank(Job::Sergeant), 0);
    BOOST_TEST_REQUIRE(hbDest.GetNumVisualFigures(Job::Sergeant) == 1u);
    BOOST_TEST_REQUIRE(!hbDest.GetDefender());

    const noShip& ship = *world.GetPlayer(2).GetShipByID(0);
    const nobHarborBuilding& hbSrc = *world.GetSpecObj<nobHarborBuilding>(harborPos[2]);

    // Start attack (1 strong first) -> Soldiers walk to harbor, ship does nothing yet
    this->SeaAttack(harborPos[1], 1, true);
    this->SeaAttack(harborPos[1], 4, true);
    BOOST_TEST_REQUIRE(milBld2->GetNumTroops() == 1u);
    BOOST_TEST_REQUIRE(ship.IsIdling());
    BOOST_TEST_REQUIRE(hbSrc.GetNumVisualFigures(Job::General) == 0u);

    // Let soldier walk to harbor
    unsigned distance = 2 * world.CalcDistance(milBld2Pos, harborPos[2]);
    RTTR_EXEC_TILL(distance * 20 + 30, !ship.IsIdling());
    BOOST_TEST_REQUIRE(ship.IsGoingToHarbor(hbSrc));
    BOOST_TEST_REQUIRE(hbSrc.GetNumVisualFigures(Job::General) == 1u);

    // Let ship arrive and count soldiers that arrived so far
    unsigned numSoldiersForAttack = 0;
    for(unsigned gf = 0; gf < 60;)
    {
        numSoldiersForAttack = hbSrc.GetNumVisualFigures(Job::Private) + hbSrc.GetNumVisualFigures(Job::General);
        unsigned numGfs = em.ExecuteNextEvent();
        RTTR_Assert(numGfs > 0u);
        gf += numGfs;
        if(ship.IsOnAttackMission())
            break;
    }
    BOOST_TEST_REQUIRE(ship.IsOnAttackMission());
    // Ship should have picked up all soldiers
    BOOST_TEST_REQUIRE(hbSrc.GetNumVisualFigures(Job::Private) == 0u);
    BOOST_TEST_REQUIRE(hbSrc.GetNumVisualFigures(Job::General) == 0u);
    // min. 2 soldiers, or attack success is not save. Place ship further away if required
    BOOST_TEST_REQUIRE(numSoldiersForAttack >= 2u);
    // Note: This might be off by one if the ship arrived in the same GF a soldier arrived. But this is unlikely
    BOOST_TEST_REQUIRE(ship.GetFigures().size() == numSoldiersForAttack); //-V807
    for(const noFigure& attacker : ship.GetFigures())
    {
        BOOST_TEST_REQUIRE(!dynamic_cast<const nofAttacker&>(attacker).IsSeaAttackCompleted());
    }

    // Ship loads, then starts driving (currently 200GF)
    RTTR_EXEC_TILL(200, ship.IsMoving());
    BOOST_TEST_REQUIRE(ship.IsOnAttackMission());
    // This should not have been changed
    BOOST_TEST_REQUIRE(ship.GetFigures().size() == numSoldiersForAttack);
    // Ship does NOT go to any harbor, as that one is an intermediate spot only
    BOOST_TEST_REQUIRE(!ship.IsGoingToHarbor(hbDest));
    BOOST_TEST_REQUIRE(!ship.IsGoingToHarbor(hbSrc));
    // Ship must not yet be at target harbor
    const MapPoint targetPt = world.GetCoastalPoint(hbDest.GetHarborPosID(), ship.GetSeaID());
    distance = world.CalcDistance(ship.GetPos(), targetPt);
    BOOST_TEST_REQUIRE(distance > 2u);
    // Ship goes to enemy harbor coast and starts unloading
    RTTR_EXEC_TILL(distance * 20 + 20, !ship.IsMoving());
    BOOST_TEST_REQUIRE(ship.IsOnAttackMission());
    BOOST_TEST_REQUIRE(ship.GetPos() == targetPt);
    // This should not (yet) have been changed
    BOOST_TEST_REQUIRE(ship.GetFigures().size() == numSoldiersForAttack);

    // Now soldiers get out one by one
    for(unsigned curNumSoldiers = 1; curNumSoldiers <= numSoldiersForAttack; curNumSoldiers++)
    {
        for(unsigned gf = 0; gf < 40; gf++)
        {
            em.ExecuteNextGF();
            if(ship.GetFigures().size() + curNumSoldiers <= numSoldiersForAttack)
                break;
        }
        BOOST_TEST_REQUIRE(ship.GetFigures().size() + curNumSoldiers == numSoldiersForAttack);
    }
    // All out
    BOOST_TEST_REQUIRE(ship.GetFigures().size() == 0u);

    // Harbor might already be destroyed for fast fights and many attackers. Add more defenders for that case
    BOOST_TEST_REQUIRE(world.GetSpecObj<nobHarborBuilding>(harborPos[1]));
    // Soldiers go to enemy harbor and engage defender
    RTTR_EXEC_TILL(100, hbDest.GetDefender());
    // Harbor still valid
    BOOST_TEST_REQUIRE(world.GetSpecObj<nobHarborBuilding>(harborPos[1]));
    // Defender sent
    BOOST_TEST_REQUIRE(hbDest.GetDefender());

    // Eventually the harbor gets destroyed
    RTTR_EXEC_TILL(1000, world.GetNO(harborPos[1])->GetGOT() == GO_Type::Fire);

    // Collect remaining attackers, so we know how many survived
    std::vector<MapPoint> pts = world.GetPointsInRadius(harborPos[1], 5);
    pts.push_back(harborPos[1]);
    std::vector<unsigned> attackerIds;
    for(const MapPoint& pt : pts)
    {
        for(const noBase& fig : world.GetFigures(pt))
        {
            const auto* attacker = dynamic_cast<const nofAttacker*>(&fig);
            if(attacker)
            {
                BOOST_TEST_REQUIRE(attacker->GetPlayer() == 2u);
                // Save those for later, as attackers get deleted
                attackerIds.push_back(attacker->GetObjId());
            }
        }
    }
    // We must have at least 1 and at most the start count of soldiers remaining
    BOOST_TEST_REQUIRE(attackerIds.size() >= 1u);
    BOOST_TEST_REQUIRE(attackerIds.size() <= numSoldiersForAttack);

    // Ship waits till all attackers have returned and leaves then
    RTTR_EXEC_TILL(100, ship.IsMoving());
    BOOST_TEST_REQUIRE(ship.IsOnAttackMission());
    BOOST_TEST_REQUIRE(ship.IsGoingToHarbor(hbSrc));
    BOOST_TEST_REQUIRE(ship.GetFigures().size() == attackerIds.size());
    for(const noFigure& attacker : ship.GetFigures())
    {
        BOOST_TEST_REQUIRE(dynamic_cast<const nofAttacker&>(attacker).IsSeaAttackCompleted());
    }

    // Ship returns home and starts unloading
    RTTR_EXEC_TILL(distance * 20 + 20, !ship.IsMoving());
    BOOST_TEST_REQUIRE(ship.IsOnAttackMission());
    BOOST_TEST_REQUIRE(ship.IsGoingToHarbor(hbSrc));

    // And should finish this in currently 200GF
    RTTR_EXEC_TILL(200, ship.IsIdling());
    BOOST_TEST_REQUIRE(!ship.IsOnAttackMission());

    // Finally troops should return home (and missing ones replaced by HQ)
    RTTR_EXEC_TILL(1000, milBld2->GetNumTroops() == 6u);
    // All troups should have returned home
    unsigned numTroopsFound = 0;
    for(const nofPassiveSoldier& soldier : milBld2->GetTroops())
    {
        if(helpers::contains(attackerIds, soldier.GetObjId()))
            numTroopsFound++;
    }
    BOOST_TEST_REQUIRE(numTroopsFound == attackerIds.size());
    // Make sure we don't have duplicate IDs (soldiers get converted active<->passive)
    helpers::makeUnique(attackerIds);
    BOOST_TEST_REQUIRE(attackerIds.size() == numTroopsFound);

    // All soldiers should have left
    BOOST_TEST_REQUIRE(hbSrc.GetNumVisualFigures(Job::Private) == 0u);
    BOOST_TEST_REQUIRE(hbSrc.GetNumVisualFigures(Job::General) == 0u);
}

BOOST_FIXTURE_TEST_CASE(HarborBlocksSpots, SeaAttackFixture)
{
    const auto tWater = GetWaterTerrain(world.GetDescription());
    const auto tLand = GetLandTerrain(world.GetDescription());

    // Issue: A harbor is a castle-sized building and blocks the nodes W, NW, NE
    // If the NW node is selected as the corresponding seas coastal position, we cannot attack that harbor as the
    // walking path would go over the harbor or a blocked point if we can't walk around it Harbors is attackable by
    // default
    BOOST_TEST_REQUIRE(gwv.GetNumSoldiersForSeaAttack(harborPos[1]) == 5u);
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
    const MapPoint ptW = world.GetNeighbour(harborPos[1], Direction::West);
    // Make west point a non-coastal point by adding land to its sea points
    const MapPoint seaPtW = world.GetNeighbour(ptW, Direction::West);
    const MapPoint seaPtW2 = world.GetNeighbour(ptW, Direction::NorthWest);
    world.GetNodeWriteable(seaPtW).t2 = tLand;
    world.GetNodeWriteable(seaPtW2).t1 = tLand;
    // Make NE pt a sea point
    const MapPoint ptNE = world.GetNeighbour(harborPos[1], Direction::NorthEast);
    const MapPoint seaPtN = world.GetNeighbour(ptNE, Direction::NorthWest);
    world.GetNodeWriteable(seaPtN).t1 = tWater;
    world.GetNodeWriteable(seaPtN).t2 = tWater;

    const MapPoint ptNW = world.GetNeighbour(harborPos[1], Direction::NorthWest);
    BOOST_TEST_REQUIRE(world.IsWaterPoint(world.GetNeighbour(ptNW, Direction::NorthWest)));
    BOOST_TEST_REQUIRE(world.IsWaterPoint(seaPtN));
    // Re-init seas/harbors
    BOOST_TEST_REQUIRE(MapLoader::InitSeasAndHarbors(world));

    // Still have our harbor
    const unsigned hbId = world.GetHarborPointID(harborPos[1]);
    BOOST_TEST_REQUIRE(hbId);
    // Should have coast at NE (NW is skipped)
    BOOST_TEST_REQUIRE(world.GetSeaId(hbId, Direction::West) == 0u);
    BOOST_TEST_REQUIRE(world.GetSeaId(hbId, Direction::NorthWest) == 0u);
    BOOST_TEST_REQUIRE(world.GetSeaId(hbId, Direction::NorthEast) == 1u);
    BOOST_TEST_REQUIRE(world.GetSeaId(hbId, Direction::East) == 0u);
    BOOST_TEST_REQUIRE(world.GetSeaId(hbId, Direction::SouthEast) == 0u);
    BOOST_TEST_REQUIRE(world.GetSeaId(hbId, Direction::SouthWest) == 0u);

    // So we should still be able to attack
    BOOST_TEST_REQUIRE(gwv.GetNumSoldiersForSeaAttack(harborPos[1]) == 5u);
    // Start attack
    this->SeaAttack(harborPos[1], 5, true);
    // Let soldier walk to harbor
    unsigned distance = 2 * world.CalcDistance(milBld2Pos, harborPos[2]);
    RTTR_SKIP_GFS(distance * 20 + 30);
    // And ship go to harbor (+200 GFs for loading)
    const noShip& ship = *world.GetPlayer(2).GetShipByID(0);
    distance = world.CalcDistance(ship.GetPos(), harborPos[1]);
    RTTR_EXEC_TILL(distance * 2 * 20 + 200, (!ship.IsMoving() && world.CalcDistance(ship.GetPos(), harborPos[1]) <= 2));
    BOOST_TEST_REQUIRE(ship.IsOnAttackMission());
    // Harbor should be destroyed and the ship go back
    RTTR_EXEC_TILL(500, ship.IsMoving());
    BOOST_TEST_REQUIRE(world.GetNO(harborPos[1])->GetGOT() == GO_Type::Fire);
}

BOOST_AUTO_TEST_SUITE_END()
