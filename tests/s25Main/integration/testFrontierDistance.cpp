// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "GamePlayer.h"
#include "buildings/nobMilitary.h"
#include "factories/BuildingFactory.h"
#include "helpers/Range.h"
#include "worldFixtures/CreateEmptyWorld.h"
#include "worldFixtures/CreateSeaWorld.h"
#include "worldFixtures/WorldFixture.h"
#include "worldFixtures/terrainHelpers.h"
#include "world/MapLoader.h"
#include <boost/test/unit_test.hpp>
#include <stdexcept>

// LCOV_EXCL_START
static std::ostream& operator<<(std::ostream& out, const FrontierDistance e)
{
    return out << static_cast<unsigned>(rttr::enum_cast(e));
}
// LCOV_EXCL_STOP

BOOST_AUTO_TEST_SUITE(FrontierDistanceSuite)

namespace {

template<unsigned T_width, unsigned T_height, class T_WorldCreator = CreateEmptyWorld>
struct FrontierWorld : public WorldFixture<T_WorldCreator, 2, T_width, T_height>
{
    using WorldFixture<T_WorldCreator, 2, T_width, T_height>::world;

    MapPoint milBld0Pos, milBld1Pos;
    nobMilitary *milBld0, *milBld1;

    FrontierWorld()
    {
        const GamePlayer& p0 = world.GetPlayer(0);
        const GamePlayer& p1 = world.GetPlayer(1);
        milBld0Pos = p0.GetHQPos() - MapPoint(0, 2);
        milBld1Pos = p1.GetHQPos() - MapPoint(0, 2);
        if(std::is_same<T_WorldCreator, CreateEmptyWorld>::value)
        { // Assumed by distributions and sizes
            BOOST_TEST_REQUIRE(milBld0Pos.y == milBld1Pos.y);
        }
        // Destroy HQs so only buildings are checked
        world.DestroyNO(p0.GetHQPos());
        world.DestroyNO(p1.GetHQPos());
        milBld0 = dynamic_cast<nobMilitary*>(
          BuildingFactory::CreateBuilding(world, BuildingType::Barracks, milBld0Pos, 0, Nation::Romans));
        milBld1 = dynamic_cast<nobMilitary*>(
          BuildingFactory::CreateBuilding(world, BuildingType::Watchtower, milBld1Pos, 1, Nation::Vikings));
    }
};
using FrontierWorldSmall = FrontierWorld<34u, 20u>;
using FrontierWorldMiddle = FrontierWorld<38u, 20u>;
using FrontierWorldBig = FrontierWorld<60u, 20u>;
using FrontierWorldSea = FrontierWorld<SmallSeaWorldDefault<2>::width, SmallSeaWorldDefault<2>::height, CreateSeaWorld>;

} // namespace

BOOST_FIXTURE_TEST_CASE(FrontierDistanceNear, FrontierWorldSmall)
{
    const auto tWater = GetWaterTerrain(world.GetDescription());

    for(const auto y : helpers::range(1, +world.GetHeight()))
    {
        for(const auto x : helpers::range(1, +world.GetWidth()))
        {
            MapPoint curPoint(x, y);
            if(curPoint == milBld0Pos || curPoint == milBld1Pos)
                continue;

            MapNode& mapPoint = world.GetNodeWriteable(curPoint);
            mapPoint.t1 = tWater;
            mapPoint.t2 = tWater;
        }
    }

    for(int i = 0; i <= 1; i++)
    {
        this->ggs.setSelection(AddonId::FRONTIER_DISTANCE_REACHABLE, i); // addon is active on second run
        world.GetPlayer(0).RecalcMilitaryFlags();
        world.GetPlayer(1).RecalcMilitaryFlags();

        FrontierDistance distance0 = milBld0->GetFrontierDistance();
        FrontierDistance distance1 = milBld1->GetFrontierDistance();

        BOOST_TEST(distance0 == distance1);
        BOOST_TEST(
          distance0
          == (i == 0 ? FrontierDistance::Near : FrontierDistance::Far)); // near if addon is inactive, otherwise inland
    }
}

BOOST_FIXTURE_TEST_CASE(FrontierDistanceNearOtherFields, FrontierWorldSmall)
{
    for(int terrain = 0; terrain < 2; terrain++)
    {
        TerrainKind searchedTerrain = terrain == 1 ? TerrainKind::Lava : TerrainKind::Snow;
        const auto tUnreachable = world.GetDescription().terrain.find(
          [searchedTerrain](const TerrainDesc& t) { return t.kind == searchedTerrain && t.Is(ETerrain::Unreachable); });

        for(const auto y : helpers::range(1, +world.GetHeight()))
        {
            for(const auto x : helpers::range(1, +world.GetWidth()))
            {
                MapPoint curPoint(x, y);
                if(curPoint == milBld0Pos || curPoint == milBld1Pos)
                    continue;

                MapNode& mapPoint = world.GetNodeWriteable(curPoint);
                mapPoint.t1 = tUnreachable;
                mapPoint.t2 = tUnreachable;
            }
        }

        for(int i = 0; i <= 1; i++)
        {
            this->ggs.setSelection(AddonId::FRONTIER_DISTANCE_REACHABLE, i); // addon is active on second run
            world.GetPlayer(0).RecalcMilitaryFlags();
            world.GetPlayer(1).RecalcMilitaryFlags();

            FrontierDistance distance0 = milBld0->GetFrontierDistance();
            FrontierDistance distance1 = milBld1->GetFrontierDistance();

            BOOST_TEST(distance0 == distance1);
            BOOST_TEST(distance0
                       == (i == 0 ? FrontierDistance::Near :
                                    FrontierDistance::Far)); // near if addon is inactive, otherwise inland
        }
    }
}

BOOST_FIXTURE_TEST_CASE(FrontierDistanceMiddle, FrontierWorldMiddle)
{
    const DescIdx<TerrainDesc> tWater = GetWaterTerrain(world.GetDescription());

    for(const auto y : helpers::range(1, +world.GetHeight()))
    {
        for(const auto x : helpers::range(1, +world.GetWidth()))
        {
            MapPoint curPoint(x, y);
            if(curPoint == milBld0Pos || curPoint == milBld1Pos)
            {
                continue;
            }

            MapNode& mapPoint = world.GetNodeWriteable(curPoint);
            mapPoint.t1 = tWater;
            mapPoint.t2 = tWater;
        }
    }

    for(int i = 0; i <= 1; i++)
    {
        this->ggs.setSelection(AddonId::FRONTIER_DISTANCE_REACHABLE, i); // addon is active on second run
        world.GetPlayer(0).RecalcMilitaryFlags();
        world.GetPlayer(1).RecalcMilitaryFlags();

        FrontierDistance distance0 = milBld0->GetFrontierDistance();
        FrontierDistance distance1 = milBld1->GetFrontierDistance();

        BOOST_TEST(distance0 == distance1);
        BOOST_TEST(
          distance0
          == (i == 0 ? FrontierDistance::Mid : FrontierDistance::Far)); // middle if addon is inactive, otherwise inland
    }
}

BOOST_FIXTURE_TEST_CASE(FrontierDistanceFar, FrontierWorldBig)
{
    const DescIdx<TerrainDesc> tWater = GetWaterTerrain(world.GetDescription());

    for(const auto y : helpers::range(1, +world.GetHeight()))
    {
        for(const auto x : helpers::range(1, +world.GetWidth()))
        {
            MapPoint curPoint(x, y);
            if(curPoint == milBld0Pos || curPoint == milBld1Pos)
            {
                continue;
            }

            MapNode& node = world.GetNodeWriteable(curPoint);
            node.t1 = node.t2 = tWater;
        }
    }

    for(int i = 0; i <= 1; i++)
    {
        this->ggs.setSelection(AddonId::FRONTIER_DISTANCE_REACHABLE, i); // addon is active on second run
        world.GetPlayer(0).RecalcMilitaryFlags();
        world.GetPlayer(1).RecalcMilitaryFlags();

        FrontierDistance distance0 = milBld0->GetFrontierDistance();
        FrontierDistance distance1 = milBld1->GetFrontierDistance();

        BOOST_TEST_REQUIRE(distance0 == distance1);
        BOOST_TEST_REQUIRE(distance0 == FrontierDistance::Far); // every time inland
    }
}

BOOST_FIXTURE_TEST_CASE(FrontierDistanceHarbor, FrontierWorldSea)
{
    // With sea attacks
    this->ggs.setSelection(AddonId::SEA_ATTACK, 0);
    milBld0->LookForEnemyBuildings(milBld1);
    BOOST_TEST(milBld0->GetFrontierDistance() == FrontierDistance::Harbor);
    this->ggs.setSelection(AddonId::SEA_ATTACK, 1);
    milBld0->LookForEnemyBuildings(milBld1);
    BOOST_TEST(milBld0->GetFrontierDistance() == FrontierDistance::Harbor);
    // With sea attacks disabled
    this->ggs.setSelection(AddonId::SEA_ATTACK, 2);
    milBld0->LookForEnemyBuildings(milBld1);
    BOOST_TEST(milBld0->GetFrontierDistance() == FrontierDistance::Far);
}

BOOST_FIXTURE_TEST_CASE(FrontierDistanceIslandTest, FrontierWorldMiddle)
{
    const DescIdx<TerrainDesc> tWater = GetWaterTerrain(world.GetDescription());

    // Little bit, but walkable water between the 2 buildings (middle of map)
    // and around the border big water
    unsigned middle = world.GetWidth() / 2;
    for(const auto y : helpers::range(1, +world.GetHeight()))
    {
        for(const auto x : helpers::range(1, +world.GetWidth()))
        {
            MapPoint curPoint(x, y);

            if(curPoint.x < 5 || curPoint.x > world.GetWidth() - 5 || curPoint.y < 5
               || curPoint.y > world.GetHeight() - 5 || (curPoint.x >= middle - 1 && curPoint.x <= middle))
            {
                MapNode& mapPoint = world.GetNodeWriteable(curPoint);
                mapPoint.t1 = tWater;
                mapPoint.t2 = tWater;
            }
        }
    }

    for(int i = 0; i <= 1; i++)
    {
        this->ggs.setSelection(AddonId::FRONTIER_DISTANCE_REACHABLE, i); // addon is active on second run
        world.GetPlayer(0).RecalcMilitaryFlags();
        world.GetPlayer(1).RecalcMilitaryFlags();

        FrontierDistance distance0 = milBld0->GetFrontierDistance();
        FrontierDistance distance1 = milBld1->GetFrontierDistance();

        BOOST_TEST_REQUIRE(distance0 == distance1);
        BOOST_TEST_REQUIRE(distance0 == FrontierDistance::Mid);
    }
}

//
//  Bug #815 can be simplified to the following setup. Players HQ don't matter.
//  In general its a simple island, with a T separating the players HQs.
//  The design is used, to have both P1s military buildings within the LookForMilitaryBuilding calculation
//  and get a FRONTIER_DISTANCE_UNREACHABLE - behavior because of the terrain.
//
//  - and | represent water fields.
//
//  ---------------------------------------------
//  |                                           |
//  |         P0(WT/NEAR)     P1(WT/NEAR)       |
//  |      WILL GET BUGED                       |
//  |    ----------------------------------     |
//  |    ---------------||-----------------     |
//  |                   ||                      |
//  |                   ||    P1 (WT/FAR)       |
//  |                   ||    >40 Fields away   |
//  |                   ||                      |
//  |      P0(HQ)       ||        P1(HQ)        |
//  |                   ||                      |
//  |                   ||                      |
//  |                   ||                      |
//  ---------------------------------------------
//
using WorldBig = WorldFixture<CreateEmptyWorld, 2, 60u, 60u>;
BOOST_FIXTURE_TEST_CASE(FrontierDistanceBug_815, WorldBig)
{
    this->ggs.setSelection(AddonId::FRONTIER_DISTANCE_REACHABLE, 1);

    GamePlayer& p0 = world.GetPlayer(0);
    GamePlayer& p1 = world.GetPlayer(1);

    const DescIdx<TerrainDesc> tWater = GetWaterTerrain(world.GetDescription());

    unsigned middle = world.GetWidth() / 2;

    for(const auto y : helpers::range(1, +world.GetHeight()))
    {
        for(const auto x : helpers::range(1, +world.GetWidth()))
        {
            MapPoint curPoint(x, y);

            // get an island
            if(curPoint.x < 10 || curPoint.x > world.GetWidth() - 10 || curPoint.y < 10
               || curPoint.y > world.GetHeight() - 10)
            {
                MapNode& mapPoint = world.GetNodeWriteable(curPoint);
                mapPoint.t1 = tWater;
                mapPoint.t2 = tWater;
                continue;
            }

            // get bottleneck'ed passage on north of the island
            if((curPoint.x >= middle - 2 && curPoint.x <= middle + 2) && (curPoint.y > 20))
            {
                MapNode& mapPoint = world.GetNodeWriteable(curPoint);
                mapPoint.t1 = tWater;
                mapPoint.t2 = tWater;
                continue;
            }

            // get some water from the bottle neck to the west/east of the island
            if(curPoint.x > 12 && curPoint.x < world.GetWidth() - 12 && curPoint.y > 20 && curPoint.y < 25)
            {
                MapNode& mapPoint = world.GetNodeWriteable(curPoint);
                mapPoint.t1 = tWater;
                mapPoint.t2 = tWater;
                continue;
            }
        }
    }

    // side of p1 outside the bottle neck, this building will cause the bug
    MapPoint p1Far(middle + 5, 30);
    BuildingFactory::CreateBuilding(world, BuildingType::Watchtower, p1Far, p1.GetPlayerId(), Nation::Romans);

    // p1 s building, which should cause a frontier distance "near"
    MapPoint p1Near(middle + 5, 15);
    auto* milBld1 = dynamic_cast<nobMilitary*>(
      BuildingFactory::CreateBuilding(world, BuildingType::Watchtower, p1Near, p1.GetPlayerId(), Nation::Romans));

    // p0 s building, should be near, like p1 s but, will be far cause p1Far cant be reached (patch is longer then 40
    // units). It will override the NEAR-Distance from P1Near, when evaluating P1Far
    MapPoint p0Near(middle - 5, 15);
    auto* milBld0 = dynamic_cast<nobMilitary*>(
      BuildingFactory::CreateBuilding(world, BuildingType::Watchtower, p0Near, p0.GetPlayerId(), Nation::Romans));

    FrontierDistance distance0 = milBld0->GetFrontierDistance();
    FrontierDistance distance1 = milBld1->GetFrontierDistance();

    BOOST_TEST_REQUIRE(distance0 == FrontierDistance::Near);
    BOOST_TEST_REQUIRE(distance1 == FrontierDistance::Near);
}

BOOST_AUTO_TEST_SUITE_END()
