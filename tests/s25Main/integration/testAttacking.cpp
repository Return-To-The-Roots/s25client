// Copyright (C) 2005 - 2024 Settlers Freaks (sf-team at siedler25.org)
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
#include "world/GameWorldViewer.h"
#include "nodeObjs/noFlag.h"
#include "gameTypes/GameTypesOutput.h"
#include "gameData/SettingTypeConv.h"
#include <rttr/test/testHelpers.hpp>
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
    BOOST_TEST_FAIL("Event not found"); // LCOV_EXCL_LINE
}

/// Move the object next to the given point. The next walk event will make it reach that point
// LCOV_EXCL_START
void moveObjTo(GameWorldBase& world, noFigure& obj, const MapPoint& pos)
{
    std::unique_ptr<noFigure> ownedObj;
    if(world.HasFigureAt(obj.GetPos(), obj))
        ownedObj = world.RemoveFigure(obj.GetPos(), obj);
    else
        ownedObj = world.RemoveFigure(world.GetNeighbour(obj.GetPos(), obj.GetCurMoveDir() + 3u), obj);
    obj.SetPos(world.GetNeighbour(pos, Direction::West));
    world.AddFigure(obj.GetPos(), std::move(ownedObj));
    if(obj.IsMoving())
        obj.FaceDir(Direction::East);
    else
        obj.StartWalking(Direction::East);
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
        goods.Add(Job::General, 3);
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
        const MapPoint start = world.GetNeighbour(bldPosFrom, Direction::SouthEast);
        const MapPoint end = world.GetNeighbour(bldPosTo, Direction::SouthEast);
        std::vector<Direction> road = FindPathForRoad(world, start, end, false);
        BOOST_TEST_REQUIRE(!road.empty());
        this->BuildRoad(start, false, road);
        BOOST_TEST_REQUIRE(world.GetPointRoad(start, road.front()) == PointRoad::Normal);
    }

    void AddSoldiersWithRank(MapPoint bldPos, unsigned numSoldiers, unsigned rank)
    {
        BOOST_TEST_REQUIRE(rank <= world.GetGGS().GetMaxMilitaryRank());
        auto* bld = world.template GetSpecObj<nobMilitary>(bldPos);
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
        BOOST_TEST_REQUIRE(hqPos[0].x < hqPos[1].x);
        BOOST_TEST_REQUIRE(hqPos[0].y < hqPos[2].y);
        BOOST_TEST_REQUIRE(hqPos[1].y < hqPos[2].y);

        // Build some military buildings
        milBld0Pos = hqPos[0] + MapPoint(7, 0);
        BOOST_TEST_REQUIRE(world.GetBQ(milBld0Pos, 0) == BuildingQuality::Castle);
        milBld0 = dynamic_cast<nobMilitary*>(
          BuildingFactory::CreateBuilding(world, BuildingType::Watchtower, milBld0Pos, 0, Nation::Babylonians));
        BOOST_TEST_REQUIRE(milBld0);

        milBld1NearPos = hqPos[1] - MapPoint(7, 0);
        BOOST_TEST_REQUIRE(world.GetBQ(milBld1NearPos, 1) == BuildingQuality::Castle);
        milBld1Near = dynamic_cast<nobMilitary*>(
          BuildingFactory::CreateBuilding(world, BuildingType::Watchtower, milBld1NearPos, 1, Nation::Romans));
        BOOST_TEST_REQUIRE(milBld1Near);

        milBld1FarPos = hqPos[1] + MapPoint(3, 1);
        BOOST_TEST_REQUIRE(world.GetBQ(milBld1FarPos, 1) == BuildingQuality::Castle);
        milBld1Far = dynamic_cast<nobMilitary*>(
          BuildingFactory::CreateBuilding(world, BuildingType::Watchtower, milBld1FarPos, 1, Nation::Romans));
        BOOST_TEST_REQUIRE(milBld1Far);
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

template<unsigned T_numPlayers = 2, unsigned T_width = AttackDefaults::width,
         unsigned T_height = AttackDefaults::height>
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
        BOOST_TEST_REQUIRE(world.GetBQ(milBld0Pos, 0) == BuildingQuality::Castle);
        milBld0 = static_cast<nobMilitary*>(
          BuildingFactory::CreateBuilding(world, BuildingType::Watchtower, milBld0Pos, 0, Nation::Babylonians));
        BOOST_TEST_REQUIRE(milBld0);

        milBld1Pos = world.MakeMapPoint(hqPos[1] + Position(0, 6));
        BOOST_TEST_REQUIRE(world.GetBQ(milBld1Pos, 1) == BuildingQuality::Castle);
        milBld1 = static_cast<nobMilitary*>(
          BuildingFactory::CreateBuilding(world, BuildingType::Watchtower, milBld1Pos, 1, Nation::Romans));
        BOOST_TEST_REQUIRE(milBld1);
        MakeVisible(milBld0Pos);
        MakeVisible(milBld1Pos);
    }

    /// Assert that attacking the given building from attackSrc fails
    void TestFailingAttack(const GameWorldViewer& gwv, const MapPoint& bldPos, const nobMilitary& attackSrc,
                           unsigned numSoldiersLeft = 6u)
    {
        BOOST_TEST_REQUIRE(attackSrc.GetNumTroops() == numSoldiersLeft);
        // No available soldiers
        BOOST_TEST_REQUIRE(gwv.GetNumSoldiersForAttack(bldPos) == 0u);
        this->Attack(bldPos, 1, true);
        // Same left
        BOOST_TEST_REQUIRE(attackSrc.GetNumTroops() == numSoldiersLeft);
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
    BOOST_TEST_REQUIRE(milBld1Near->GetNumTroops() == 6u);
    BOOST_TEST_REQUIRE(milBld1Far->GetNumTroops() == 6u);
    BOOST_TEST_REQUIRE(milBld0->GetNumTroops() == 6u);

    // Player 2 has no military blds -> Can't attack
    SetCurPlayer(2);
    BOOST_TEST_REQUIRE(gwv.GetNumSoldiersForAttack(hqPos[0]) == 0u);
    BOOST_TEST_REQUIRE(gwv.GetNumSoldiersForAttack(hqPos[1]) == 0u);
    BOOST_TEST_REQUIRE(gwv.GetNumSoldiersForAttack(milBld0->GetPos()) == 0u);
    BOOST_TEST_REQUIRE(gwv.GetNumSoldiersForAttack(milBld1Near->GetPos()) == 0u);
    BOOST_TEST_REQUIRE(gwv.GetNumSoldiersForAttack(milBld1Far->GetPos()) == 0u);
    SetCurPlayer(1);
    // No self attack
    BOOST_TEST_REQUIRE(gwv.GetNumSoldiersForAttack(hqPos[1]) == 0u);
    BOOST_TEST_REQUIRE(gwv.GetNumSoldiersForAttack(milBld1Near->GetPos()) == 0u);
    BOOST_TEST_REQUIRE(gwv.GetNumSoldiersForAttack(milBld1Far->GetPos()) == 0u);
    // Attack both others
    BOOST_TEST_REQUIRE(gwv.GetNumSoldiersForAttack(hqPos[0]) == 5u);
    BOOST_TEST_REQUIRE(gwv.GetNumSoldiersForAttack(hqPos[2]) == 5u);
    // This is in the extended range of the far bld -> 2 more (with current range)
    BOOST_TEST_REQUIRE(gwv.GetNumSoldiersForAttack(milBld0->GetPos()) == 7u);
    SetCurPlayer(0);
    // No self attack
    BOOST_TEST_REQUIRE(gwv.GetNumSoldiersForAttack(hqPos[0]) == 0u);
    BOOST_TEST_REQUIRE(gwv.GetNumSoldiersForAttack(milBld0->GetPos()) == 0u);
    // Attack both others
    BOOST_TEST_REQUIRE(gwv.GetNumSoldiersForAttack(hqPos[1]) == 5u);
    BOOST_TEST_REQUIRE(gwv.GetNumSoldiersForAttack(hqPos[2]) == 5u);
    BOOST_TEST_REQUIRE(gwv.GetNumSoldiersForAttack(milBld1Near->GetPos()) == 5u);
    // Counterpart: 2 possible for far bld
    BOOST_TEST_REQUIRE(gwv.GetNumSoldiersForAttack(milBld1Far->GetPos()) == 2u);

    // Test in peaceful mode -- no attacks should be possible
    this->ggs.setSelection(AddonId::PEACEFULMODE, 1);
    SetCurPlayer(1);
    BOOST_TEST_REQUIRE(gwv.GetNumSoldiersForAttack(hqPos[0]) == 0u);
    BOOST_TEST_REQUIRE(gwv.GetNumSoldiersForAttack(hqPos[2]) == 0u);
    BOOST_TEST_REQUIRE(gwv.GetNumSoldiersForAttack(milBld0->GetPos()) == 0u);
    SetCurPlayer(0);
    BOOST_TEST_REQUIRE(gwv.GetNumSoldiersForAttack(hqPos[1]) == 0u);
    BOOST_TEST_REQUIRE(gwv.GetNumSoldiersForAttack(hqPos[2]) == 0u);
    BOOST_TEST_REQUIRE(gwv.GetNumSoldiersForAttack(milBld1Near->GetPos()) == 0u);

    // Functions related to stationed soldiers
    BOOST_TEST_REQUIRE(milBld0->HasMaxRankSoldier());
    BOOST_TEST_REQUIRE(milBld1Near->HasMaxRankSoldier());
    BOOST_TEST_REQUIRE(!milBld1Far->HasMaxRankSoldier());
    BOOST_TEST_REQUIRE(milBld1Near->GetSoldiersStrength() > milBld1Far->GetSoldiersStrength());
}

BOOST_FIXTURE_TEST_CASE(StartAttack, AttackFixture<>)
{
    initGameRNG();
    GameWorldViewer gwv(curPlayer, world);

    // Add soldiers (3 strong, 3 weak)
    AddSoldiers(milBld0Pos, 3, 3);

    const nobMilitary& attackSrc = *milBld0;
    const MapPoint usualBldPos = hqPos[1] - MapPoint(2, 3);
    BOOST_TEST_REQUIRE(world.GetBQ(usualBldPos, 1) >= BuildingQuality::Hut);
    const noBuilding* usualBld =
      BuildingFactory::CreateBuilding(world, BuildingType::Woodcutter, usualBldPos, 1, Nation::Romans);
    BOOST_TEST_REQUIRE(usualBld);
    const MapPoint storehousePos = hqPos[1] - MapPoint(2, 0);
    BOOST_TEST_REQUIRE(world.GetBQ(storehousePos, 1) >= BuildingQuality::House);
    const noBuilding* storeHouse =
      BuildingFactory::CreateBuilding(world, BuildingType::Storehouse, storehousePos, 1, Nation::Romans);
    BOOST_TEST_REQUIRE(storeHouse);
    MakeVisible(usualBldPos);
    MakeVisible(storehousePos);

    world.GetPlayer(0).team = Team::Team1;
    world.GetPlayer(1).team = Team::Team1;
    for(unsigned i = 0; i < 2; i++)
        world.GetPlayer(i).MakeStartPacts();
    // Try to attack ally -> Fail
    TestFailingAttack(gwv, milBld1Pos, attackSrc);

    world.GetPlayer(0).team = Team::Team1;
    world.GetPlayer(1).team = Team::Team2;
    for(unsigned i = 0; i < 2; i++)
        world.GetPlayer(i).MakeStartPacts();

    // Try to attack non-military bld -> Fail
    TestFailingAttack(gwv, usualBldPos, attackSrc);

    // Try to attack storehouse -> Fail
    TestFailingAttack(gwv, storehousePos, attackSrc);

    // Try to attack newly build bld -> Fail
    BOOST_TEST_REQUIRE(world.CalcVisiblityWithAllies(milBld1Pos, curPlayer) == Visibility::Visible);
    BOOST_TEST_REQUIRE(milBld1->IsNewBuilt());
    TestFailingAttack(gwv, milBld1Pos, attackSrc);

    // Add soldier
    AddSoldiers(milBld1Pos, 1, 0);
    BOOST_TEST_REQUIRE(!milBld1->IsNewBuilt());
    // Try to attack invisible bld -> Fail
    MapNode& node = world.GetNodeWriteable(milBld1Pos);
    node.fow[0].visibility = Visibility::FogOfWar;
    BOOST_TEST_REQUIRE(world.CalcVisiblityWithAllies(milBld1Pos, curPlayer) == Visibility::FogOfWar);
    TestFailingAttack(gwv, milBld1Pos, attackSrc);

    // Attack it
    node.fow[0].visibility = Visibility::Visible;
    BOOST_TEST_REQUIRE(attackSrc.GetNumTroops() == 6u);
    auto itTroops = attackSrc.GetTroops().begin();
    for(int i = 0; i < 3; i++, ++itTroops)
        BOOST_TEST_REQUIRE(itTroops->GetRank() == 0u);
    for(int i = 3; i < 6; i++, ++itTroops)
        BOOST_TEST_REQUIRE(itTroops->GetRank() == 4u);
    this->Attack(milBld1Pos, 1, true);
    // 1 strong soldier has left
    BOOST_TEST_REQUIRE(attackSrc.GetNumTroops() == 5u);
    itTroops = attackSrc.GetTroops().begin();
    for(int i = 0; i < 3; i++, ++itTroops)
        BOOST_TEST_REQUIRE(itTroops->GetRank() == 0u);
    for(int i = 3; i < 5; i++, ++itTroops)
        BOOST_TEST_REQUIRE(itTroops->GetRank() == 4u);
    // Attack with 1 weak soldier
    this->Attack(milBld1Pos, 1, false);
    // 1 weak soldier has left
    BOOST_TEST_REQUIRE(attackSrc.GetNumTroops() == 4u);
    itTroops = attackSrc.GetTroops().begin();
    for(int i = 0; i < 2; i++, ++itTroops)
        BOOST_TEST_REQUIRE(itTroops->GetRank() == 0u);
    for(int i = 2; i < 4; i++, ++itTroops)
        BOOST_TEST_REQUIRE(itTroops->GetRank() == 4u);
    // -> 2 strong, 2 weak remaining, attack with 3 weak ones -> 1 strong remaining
    this->Attack(milBld1Pos, 3, false);
    BOOST_TEST_REQUIRE(attackSrc.GetNumTroops() == 1u);
    BOOST_TEST(attackSrc.GetTroops().front().GetRank() == 4u);

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
    BOOST_TEST_REQUIRE(milBld0->GetNumTroops() == 1u);
    BOOST_TEST_REQUIRE(milBld1->GetNumTroops() == 2u);
    // Run till attackers reach bld. 1 Soldier will leave for them.
    // 1 stays inside till an attacker is at door
    // 20 GFs/node + 30 GFs for leaving
    const unsigned distance = world.CalcDistance(milBld0Pos, milBld1Pos);
    RTTR_EXEC_TILL(distance * 20 + 30, milBld1->GetNumTroops() == 1);
    BOOST_TEST_REQUIRE(milBld1->GetNumTroops() + milBld1->GetLeavingFigures().size() == 2u);
    const Inventory& attackedPlInventory = world.GetPlayer(1).GetInventory();
    const unsigned oldWeakSoldierCt = attackedPlInventory.people[Job::Private];
    const unsigned oldStrongerSoldierCt = attackedPlInventory.people[Job::PrivateFirstClass];
    const unsigned oldAttackerStrongSoldierCt = world.GetPlayer(curPlayer).GetInventory().people[Job::General];

    // 1st soldier will walk towards attacker and will be killed
    // Once an attacker reaches the flag, the bld will send a defender
    BOOST_TEST_REQUIRE(!milBld1->GetDefender());
    RTTR_EXEC_TILL(300, milBld1->GetNumTroops() == 0);
    // Defender deployed, attacker at flag
    BOOST_TEST_REQUIRE(milBld1->GetDefender());
    {
        const auto figures = world.GetFigures(milBld1->GetFlagPos());
        BOOST_TEST_REQUIRE(figures.size() == 1u);
        const auto& attacker = dynamic_cast<const nofAttacker&>(*figures.begin());
        BOOST_TEST_REQUIRE(static_cast<const nofAttacker&>(attacker).GetPlayer() == curPlayer);
    }

    // Lets fight
    RTTR_EXEC_TILL(1000, milBld1->IsBeingCaptured());
    // Let others in
    RTTR_EXEC_TILL(200, !milBld1->IsBeingCaptured());
    // Building conquered
    BOOST_TEST_REQUIRE(milBld1->GetPlayer() == curPlayer);
    // 1 soldier must be inside
    BOOST_TEST_REQUIRE(milBld1->GetNumTroops() > 1u);
    // Weak soldier must be dead
    BOOST_TEST_REQUIRE(attackedPlInventory.people[Job::Private] == oldWeakSoldierCt - 1);
    // Src building refill
    RTTR_EXEC_TILL(800, milBld0->GetNumTroops() == 6u);
    // Src building got refilled
    BOOST_TEST_REQUIRE(milBld0->GetNumTroops() == 6u);
    // We may have lost soldiers
    BOOST_TEST_REQUIRE(world.GetPlayer(curPlayer).GetInventory().people[Job::General] <= oldAttackerStrongSoldierCt);
    // The enemy may have lost his stronger soldier
    BOOST_TEST_REQUIRE(attackedPlInventory.people[Job::PrivateFirstClass] <= oldStrongerSoldierCt);
    // But only one
    BOOST_TEST_REQUIRE(attackedPlInventory.people[Job::PrivateFirstClass] >= oldStrongerSoldierCt - 1);
    // At least 2 survivors
    BOOST_TEST_REQUIRE(milBld1->GetNumTroops() > 2u);

    // Points around bld should be ours
    const std::vector<MapPoint> pts = world.GetPointsInRadius(milBld1Pos, 3);
    for(const MapPoint& pt : pts)
    {
        BOOST_TEST_REQUIRE(world.GetNode(pt).owner == curPlayer + 1u);
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
    BOOST_TEST_REQUIRE(milBld1->IsGoldDisabled());

    // Start attack -> 1
    this->Attack(milBld1Pos, 6, false);
    BOOST_TEST_REQUIRE(milBld0->GetNumTroops() == 1u);

    RTTR_EXEC_TILL(2000, milBld1->GetPlayer() == curPlayer);

    // check if coins were enabled after building was captured
    BOOST_TEST_REQUIRE(!milBld1->IsGoldDisabled());
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
    BOOST_TEST_REQUIRE(!milBld1->IsGoldDisabled());

    // Start attack -> 1
    this->Attack(milBld1Pos, 6, false);
    BOOST_TEST_REQUIRE(milBld0->GetNumTroops() == 1u);

    RTTR_EXEC_TILL(2000, milBld1->GetPlayer() == curPlayer);

    // check if coins were disabled after building was captured
    BOOST_TEST_REQUIRE(milBld1->IsGoldDisabled());
}

using AttackFixture4P = AttackFixture<4, 32, 34>;
BOOST_FIXTURE_TEST_CASE(ConquerWithMultipleWalkingIn, AttackFixture4P)
{
    initGameRNG();
    world.GetPlayer(0).team = Team::Team1; //-V525
    world.GetPlayer(1).team = Team::None;
    world.GetPlayer(2).team = Team::Team1; // Allied to 0
    world.GetPlayer(3).team = Team::Team2; // Hostile to 0
    for(unsigned i = 0; i < 4; i++)
        world.GetPlayer(i).MakeStartPacts();
    MilitarySettings milSettings = MILITARY_SETTINGS_SCALE;
    milSettings[2] = 0; // No aggressive defenders for attacker
    this->ChangeMilitary(milSettings);

    AddSoldiers(milBld0Pos, 0, 6);
    AddSoldiersWithRank(milBld1Pos, 1, 0);
    MapPoint milBld1FlagPos = world.GetNeighbour(milBld1Pos, Direction::SouthEast);

    // Scenario 1: Attack with one soldier.
    // Once enemy is defeated we walk in with another soldier of the enemy who wants to occupy its building.
    // The other soldier is faster -> we have to fight him
    this->Attack(milBld1Pos, 1, true);
    BOOST_TEST_REQUIRE(milBld0->GetLeavingFigures().size() == 1u); //-V807
    auto& attacker = dynamic_cast<nofAttacker&>(milBld0->GetLeavingFigures().front());
    // Let him come out
    RTTR_EXEC_TILL(70, milBld0->GetLeavingFigures().empty()); //-V807
    moveObjTo(world, attacker, milBld1FlagPos);
    BOOST_TEST_REQUIRE(!milBld1->IsDoorOpen());
    const auto flagFigs = world.GetFigures(milBld1FlagPos);
    RTTR_EXEC_TILL(70, flagFigs.size() == 1u && flagFigs.begin()->GetGOT() == GO_Type::Fighting); //-V807
    BOOST_TEST_REQUIRE(!milBld1->IsDoorOpen());
    // Speed up fight by reducing defenders HP to 1
    auto* defender = const_cast<nofDefender*>(milBld1->GetDefender());
    while(defender->GetHitpoints() > 1u)
        defender->TakeHit();
    RTTR_EXEC_TILL(500, milBld1->GetDefender() == nullptr);
    // Defender defeated. Attacker moving in.
    BOOST_TEST_REQUIRE(attacker.IsMoving());
    BOOST_TEST_REQUIRE(attacker.GetCurMoveDir() == Direction::NorthWest);
    // Door opened
    BOOST_TEST_REQUIRE(milBld1->IsDoorOpen());
    // New soldiers walked in
    AddSoldiersWithRank(milBld1Pos, 4, 0);
    // Let attacker walk in (try it at least)
    RTTR_EXEC_TILL(20, attacker.GetPos() == milBld1Pos);
    RTTR_EXEC_TILL(20, attacker.GetPos() == milBld1FlagPos);
    // New fight and door closed
    RTTR_EXEC_TILL(70, flagFigs.size() == 1u && flagFigs.front().GetGOT() == GO_Type::Fighting);
    BOOST_TEST_REQUIRE(!milBld1->IsDoorOpen());

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
    BOOST_TEST_REQUIRE(milBld1->GetLeavingFigures().size() == 1u);
    auto& attackerFromPl0 = dynamic_cast<nofAttacker&>(milBld1->GetLeavingFigures().front());
    // 2.
    curPlayer = 0;
    this->Attack(milBld1Pos, 1, true);
    // Move him directly out
    BOOST_TEST_REQUIRE(milBld0->GetLeavingFigures().size() == 1u);
    auto& secAttacker = dynamic_cast<nofAttacker&>(milBld0->GetLeavingFigures().front());
    RTTR_EXEC_TILL(70, milBld0->GetLeavingFigures().empty()); //-V807
    moveObjTo(world, secAttacker, world.MakeMapPoint(milBld1FlagPos - Position(15, 0)));
    nofAggressiveDefender& aggDefender = ensureNonNull(milBld1->SendAggressiveDefender(secAttacker));
    secAttacker.LetsFight(aggDefender);
    // 3.
    curPlayer = 2;
    MapPoint bldPos = hqPos[curPlayer] + MapPoint(3, 0);
    auto* alliedBld = static_cast<nobMilitary*>(
      BuildingFactory::CreateBuilding(world, BuildingType::Guardhouse, bldPos, curPlayer, Nation::Africans));
    AddSoldiersWithRank(bldPos, 2, 0);
    this->Attack(milBld1Pos, 1, false);
    BOOST_TEST_REQUIRE(alliedBld->GetLeavingFigures().size() == 1u);
    auto& alliedAttacker = dynamic_cast<nofAttacker&>(alliedBld->GetLeavingFigures().front());
    // 4.
    curPlayer = 3;
    bldPos = hqPos[curPlayer] + MapPoint(3, 0);
    auto* hostileBld = static_cast<nobMilitary*>(
      BuildingFactory::CreateBuilding(world, BuildingType::Guardhouse, bldPos, curPlayer, Nation::Africans));
    AddSoldiersWithRank(bldPos, 2, 0);
    this->Attack(milBld1Pos, 1, false);
    BOOST_TEST_REQUIRE(hostileBld->GetLeavingFigures().size() == 1u);
    auto& hostileAttacker = dynamic_cast<nofAttacker&>(hostileBld->GetLeavingFigures().front());

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
    defender = const_cast<nofDefender*>(milBld1->GetDefender());
    while(defender->GetHitpoints() > 1u)
        defender->TakeHit();
    RTTR_EXEC_TILL(500, milBld1->GetNumTroops() == 0u);
    defender = const_cast<nofDefender*>(milBld1->GetDefender());
    while(defender->GetHitpoints() > 1u)
        defender->TakeHit();
    RTTR_EXEC_TILL(500, milBld1->GetDefender() == nullptr);
    // Defender defeated. Attacker moving in.
    BOOST_TEST_REQUIRE(attacker.IsMoving());
    BOOST_TEST_REQUIRE(attacker.GetCurMoveDir() == Direction::NorthWest);
    // Door opened
    BOOST_TEST_REQUIRE(milBld1->IsDoorOpen());
    // Give him a bit of a head start
    RTTR_SKIP_GFS(1);
    // 5. Create new soldier who walks in after the attacker
    auto& newSld =
      world.AddFigure(milBld1FlagPos, std::make_unique<nofPassiveSoldier>(milBld1FlagPos, 1, milBld1, milBld1, 0));
    ;
    milBld1->GotWorker(newSld.GetJobType(), newSld);
    newSld.ActAtFirst();
    // Attacker faster -> Bld taken
    RTTR_EXEC_TILL(30, milBld1->GetPlayer() == 0u);
    // New soldier should be wandering
    RTTR_EXEC_TILL(10, newSld.IsWandering());
    // And door should be closed (latest after wandering soldier left flag)
    RTTR_EXEC_TILL(40, !milBld1->IsDoorOpen());

    // 1. Attackers from this building
    // No home -> Wander
    BOOST_TEST_REQUIRE(attackerFromPl0.HasNoHome());
    rescheduleWalkEvent(em, attackerFromPl0, 1);
    RTTR_EXEC_TILL(2, attackerFromPl0.IsWandering());
    // 2. Aggressive defenders from this building
    // No further attack (unless already fighting) and wander
    // The attacker proceeds to the building and occupies it
    rescheduleWalkEvent(em, secAttacker, 1);
    rescheduleWalkEvent(em, aggDefender, 2);
    RTTR_SKIP_GFS(2);
    BOOST_TEST_REQUIRE(aggDefender.GetAttacker() == nullptr);
    BOOST_TEST_REQUIRE(secAttacker.GetHuntingDefender() == nullptr);
    BOOST_TEST_REQUIRE(aggDefender.IsWandering());
    RTTR_EXEC_TILL(270, milBld1->GetNumTroops() == 2u);
    // 3. Allied aggressor towards this bld
    // Abort attack and return home
    rescheduleWalkEvent(em, alliedAttacker, 1);
    RTTR_EXEC_TILL(1, alliedAttacker.GetAttackedGoal() == nullptr);
    RTTR_EXEC_TILL(90, alliedBld->GetNumTroops() == 2u);
    // 4. Hostile aggressor towards this bld
    // Continue attack and fight
    rescheduleWalkEvent(em, hostileAttacker, 1);
    BOOST_TEST_REQUIRE(hostileAttacker.GetAttackedGoal() != nullptr);
    RTTR_EXEC_TILL(220, hostileAttacker.GetPos() == milBld1FlagPos);
    RTTR_EXEC_TILL(50, world.GetFigures(milBld1FlagPos).begin()->GetGOT() == GO_Type::Fighting);
}

BOOST_FIXTURE_TEST_CASE(ConquerWithCarriersWalkingIn, AttackFixture<2>)
{
    // 1. Carrier with coin walking in the building
    // 2. Carrier with coin walking out of the building
    AddSoldiers(milBld0Pos, 0, 6);
    AddSoldiersWithRank(milBld1Pos, 1, 0);
    MapPoint milBld1FlagPos = world.GetNeighbour(milBld1Pos, Direction::SouthEast);

    curPlayer = 1;
    MapPoint flagPos = world.MakeMapPoint(milBld1FlagPos - Position(2, 0));
    this->BuildRoad(milBld1FlagPos, false, std::vector<Direction>(2, Direction::West));
    auto* flag = world.GetSpecObj<noFlag>(flagPos);
    BOOST_TEST_REQUIRE(flag);
    RoadSegment* rs = flag->GetRoute(Direction::East);
    BOOST_TEST_REQUIRE(rs);
    auto& carrierIn =
      world.AddFigure(flagPos, std::make_unique<nofCarrier>(CarrierType::Normal, flagPos, curPlayer, rs, flag));
    auto& carrierOut =
      world.AddFigure(flagPos, std::make_unique<nofCarrier>(CarrierType::Donkey, flagPos, curPlayer, rs, flag));
    rs->setCarrier(0, &carrierIn);
    rs->setCarrier(1, &carrierOut);
    // Add 2 coins for the bld
    for(unsigned i = 0; i < 2; i++)
    {
        auto coin = std::make_unique<Ware>(GoodType::Coins, milBld1, flag);
        coin->WaitAtFlag(flag);
        coin->RecalcRoute();
        flag->AddWare(std::move(coin));
    }
    world.GetPlayer(1).IncreaseInventoryWare(GoodType::Coins, 2);
    carrierIn.ActAtFirst();
    carrierOut.ActAtFirst();
    // Both picked up
    BOOST_TEST_REQUIRE(flag->GetNumWares() == 0u);
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
    BOOST_TEST_REQUIRE(carrierIn.GetCurMoveDir() == Direction::NorthWest);
    BOOST_TEST_REQUIRE(carrierOut.GetCurMoveDir() == Direction::SouthEast);

    // Add another for later
    MapPoint flagPosE = world.MakeMapPoint(milBld1FlagPos + Position(2, 0));
    this->BuildRoad(milBld1FlagPos, false, std::vector<Direction>(2, Direction::East));
    auto* flagE = world.GetSpecObj<noFlag>(flagPosE);
    BOOST_TEST_REQUIRE(flagE);
    RoadSegment* rsE = flagE->GetRoute(Direction::West);
    BOOST_TEST_REQUIRE(rsE);
    auto& carrierInE =
      world.AddFigure(flagPosE, std::make_unique<nofCarrier>(CarrierType::Normal, flagPosE, curPlayer, rsE, flagE));
    rsE->setCarrier(0, &carrierInE);
    // He also gets 1 coin
    auto coin = std::make_unique<Ware>(GoodType::Coins, milBld1, flagE);
    coin->WaitAtFlag(flagE);
    coin->RecalcRoute();
    flagE->AddWare(std::move(coin));
    world.GetPlayer(1).IncreaseInventoryWare(GoodType::Coins, 1);
    carrierInE.ActAtFirst();
    // Picked up
    BOOST_TEST_REQUIRE(flagE->GetNumWares() == 0u);
    // And pause him
    rescheduleWalkEvent(em, carrierInE, 10000);

    curPlayer = 0;
    this->Attack(milBld1Pos, 1, true);
    BOOST_TEST_REQUIRE(milBld0->GetLeavingFigures().size() == 1u);
    auto& attacker = dynamic_cast<nofAttacker&>(milBld0->GetLeavingFigures().front());
    // Move him directly out
    RTTR_EXEC_TILL(70, milBld0->GetLeavingFigures().empty()); //-V807
    moveObjTo(world, attacker, milBld1FlagPos);
    RTTR_EXEC_TILL(20, attacker.GetPos() == milBld1FlagPos);
    // Carriers on pos or to pos get send away as soon as soldier arrives
    rescheduleWalkEvent(em, carrierIn, 1);
    rescheduleWalkEvent(em, carrierOut, 1);
    RTTR_SKIP_GFS(1);
    BOOST_TEST_REQUIRE(carrierIn.IsWandering());
    BOOST_TEST_REQUIRE(carrierOut.IsWandering());

    // Let east carrier walk
    rescheduleWalkEvent(em, carrierInE, 1);

    // Start fight
    const auto flagFigs = world.GetFigures(milBld1FlagPos);
    RTTR_EXEC_TILL(50, flagFigs.size() == 1u && flagFigs.front().GetGOT() == GO_Type::Fighting);
    // East carrier gets blocked
    BOOST_TEST_REQUIRE(!carrierInE.IsMoving());

    // Door closed latest after other carriers are gone
    RTTR_EXEC_TILL(20, !milBld1->IsDoorOpen());

    // Speed up fight by reducing defenders HP to 1
    auto* defender = const_cast<nofDefender*>(milBld1->GetDefender());
    while(defender->GetHitpoints() > 1u)
        defender->TakeHit();
    RTTR_EXEC_TILL(500, milBld1->GetDefender() == nullptr);
    // Defender defeated. Attacker moving in.
    BOOST_TEST_REQUIRE(attacker.IsMoving());
    BOOST_TEST_REQUIRE(attacker.GetCurMoveDir() == Direction::NorthWest);

    // Door opened
    BOOST_TEST_REQUIRE(milBld1->IsDoorOpen());
    // Blocked carrier can walk again
    BOOST_TEST_REQUIRE(carrierInE.IsMoving());
    // Capture
    RTTR_EXEC_TILL(20, milBld1->GetPlayer() == 0u);
    // East Carrier should be wandering at some point
    RTTR_EXEC_TILL(20, carrierInE.IsWandering());
    // Only 1 coin in the bld
    BOOST_TEST_REQUIRE(milBld1->GetNumCoins() == 1u);
    // Door closed after carrier left it
    RTTR_EXEC_TILL(40, !milBld1->IsDoorOpen());
}

using DestroyRoadsOnConquerFixture = AttackFixture<2, 24>;
BOOST_FIXTURE_TEST_CASE(DestroyRoadsOnConquer, DestroyRoadsOnConquerFixture)
{
    MapPoint leftBldPos = world.MakeMapPoint(milBld1Pos + Position(-5, 2));
    noBuilding* leftBld =
      BuildingFactory::CreateBuilding(world, BuildingType::Barracks, leftBldPos, 1, Nation::Babylonians);
    BOOST_TEST_REQUIRE(leftBld);
    MapPoint rightBldPos = world.MakeMapPoint(milBld1Pos + Position(5, 2));
    noBuilding* rightBld =
      BuildingFactory::CreateBuilding(world, BuildingType::Barracks, rightBldPos, 1, Nation::Babylonians);
    BOOST_TEST_REQUIRE(rightBld);

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
    BuildRoadForBlds(world.GetNeighbour(flagPt, Direction::NorthWest), milBld1Pos);
    // Build a long road connecting left&right w/o any flag in between (See #863)
    BuildRoadForBlds(leftBldPos, rightBldPos);
    BOOST_TEST_REQUIRE(leftBld->GetFlag()->GetRoute(Direction::East)->GetLength() >= 10u);

    curPlayer = 0;
    this->Attack(milBld1Pos, 5, true);
    RTTR_EXEC_TILL(2000, milBld1->GetPlayer() == curPlayer);
    std::array<MapPoint, 3> bldPts = {{leftBldPos, rightBldPos, milBld1Pos}};
    for(const MapPoint& bldPt : bldPts)
    {
        MapPoint flagPt = world.GetNeighbour(bldPt, Direction::SouthEast);
        for(Direction i : helpers::EnumRange<Direction>{})
        {
            // No routes except the main road
            if(i != Direction::NorthWest)
            {
                BOOST_TEST_REQUIRE(!world.GetSpecObj<noRoadNode>(flagPt)->GetRoute(i));
                BOOST_TEST_REQUIRE(world.GetPointRoad(flagPt, i) == PointRoad::None);
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
        AddSoldiersWithRank(milBld0Pos, 6, 0);
        AddSoldiersWithRank(milBld1Pos, 6, 0);
        this->Attack(attackedBldPos, 1, true);
        auto& attacker = dynamic_cast<nofAttacker&>(attackerBld.GetLeavingFigures().front());
        attacker_ = &attacker;
        RTTR_EXEC_TILL(70, attackerBld.GetLeavingFigures().empty()); //-V807

        nofAggressiveDefender& defender = ensureNonNull(attackedBld.SendAggressiveDefender(attacker));
        defender_ = &defender;
        RTTR_EXEC_TILL(70, attackedBld.GetLeavingFigures().empty()); //-V807
        attacker.LetsFight(defender);

        // In a distance of 2 they find each other and meet half way
        const MapPoint fightSpot = attackedBldPos - MapPoint(3, 0);
        // attacker building is left -> place attacker left of fight spot
        BOOST_TEST_REQUIRE(attackerBld.GetPos().x < attackedBldPos.x);
        moveObjTo(world, attacker, fightSpot - MapPoint(1, 0));
        moveObjTo(world, defender, fightSpot + MapPoint(1, 0));
        rescheduleWalkEvent(em, attacker, 1);
        RTTR_SKIP_GFS(1);
        BOOST_TEST_REQUIRE((defender.GetState() == nofActiveSoldier::SoldierState::MeetEnemy));
        BOOST_TEST_REQUIRE((attacker.GetState() == nofActiveSoldier::SoldierState::MeetEnemy));
    }

    /// Add blocking terrain around building of defender starting left of the specified position
    void blockDefenderBuilding(const MapCoord x)
    {
        const auto terrain =
          this->world.GetDescription().terrain.find([](const TerrainDesc& t) { return !t.Is(ETerrain::Walkable); });
        for(const auto y : helpers::range(this->world.GetHeight()))
        {
            auto& node = this->world.GetNodeWriteable(MapPoint(x, y));
            auto& rightNode = this->world.GetNodeWriteable(MapPoint(attackedBldPos.x + 1, y));
            node.t1 = node.t2 = rightNode.t1 = rightNode.t2 = terrain;
        }
        const auto yTop = this->world.MakeMapPoint(Position(attackedBldPos.x, attackedBldPos.y - 3)).y;
        const auto yBottom = this->world.MakeMapPoint(Position(attackedBldPos.x, attackedBldPos.y + 3)).y;
        for(const auto x : helpers::range(x, attackedBldPos.x))
        {
            auto& topNode = this->world.GetNodeWriteable(MapPoint(x, yTop));
            auto& bottomNode = this->world.GetNodeWriteable(MapPoint(x, yBottom));
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
    BOOST_TEST_REQUIRE((defender.GetState() == nofActiveSoldier::SoldierState::MeetEnemy));
    BOOST_TEST_REQUIRE((attacker.GetState() == nofActiveSoldier::SoldierState::WaitingForFight));
    // The next part of the test assumes they use this fight spot.
    // This doesn't need to stay true which only needs adjustments to the test.
    BOOST_TEST_REQUIRE(attacker.GetPos() == fightSpot);

    blockDefenderBuilding(fightSpot.x);
    // -> defender can't reach attacker
    // -> attacker can't reach attacked building
    // Pending fight should get aborted and both go back home:
    rescheduleWalkEvent(em, defender, 1);
    RTTR_SKIP_GFS(1);
    BOOST_TEST(defender.IsMoving());
    BOOST_TEST(attacker.IsMoving());
    BOOST_TEST((defender.GetState() != nofActiveSoldier::SoldierState::MeetEnemy));
    BOOST_TEST((attacker.GetState() != nofActiveSoldier::SoldierState::WaitingForFight));

    BOOST_TEST_REQUIRE(attackedBld.GetNumTroops() == 5u); // Sanity check
    RTTR_EXEC_TILL(100, attackedBld.GetNumTroops() == 6u);
    BOOST_TEST_REQUIRE(attackerBld.GetNumTroops() == 5u); // Sanity check
    RTTR_EXEC_TILL(100, attackerBld.GetNumTroops() == 6u);
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
    BOOST_TEST_REQUIRE((defender.GetState() == nofActiveSoldier::SoldierState::WaitingForFight));
    BOOST_TEST_REQUIRE((attacker.GetState() == nofActiveSoldier::SoldierState::MeetEnemy));
    // The next part of the test assumes they use this fight spot.
    // This doesn't need to stay true which only needs adjustments to the test.
    BOOST_TEST_REQUIRE(defender.GetPos() == fightSpot);

    blockDefenderBuilding(fightSpot.x - 1);
    // -> attacker can't reach defender or attacked building
    // Pending fight should get aborted and both go back home:
    moveObjTo(world, attacker, fightSpot - MapPoint(1, 0));
    rescheduleWalkEvent(em, attacker, 1);
    RTTR_SKIP_GFS(1);
    BOOST_TEST(defender.IsMoving());
    BOOST_TEST(attacker.IsMoving());
    BOOST_TEST((defender.GetState() != nofActiveSoldier::SoldierState::WaitingForFight));
    BOOST_TEST((attacker.GetState() != nofActiveSoldier::SoldierState::MeetEnemy));

    BOOST_TEST_REQUIRE(attackedBld.GetNumTroops() == 5u); // Sanity check
    RTTR_EXEC_TILL(100, attackedBld.GetNumTroops() == 6u);
    BOOST_TEST_REQUIRE(attackerBld.GetNumTroops() == 5u); // Sanity check
    RTTR_EXEC_TILL(100, attackerBld.GetNumTroops() == 6u);
}

BOOST_AUTO_TEST_SUITE_END()
