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

#include "buildings/nobMilitary.h"
#include "factories/BuildingFactory.h"
#include "worldFixtures/WorldWithGCExecution.h"
#include <boost/test/unit_test.hpp>
#include <stdexcept>

static std::ostream& operator<<(std::ostream& out, const FrontierDistance e)
{
    return out << static_cast<unsigned>(rttr::enum_cast(e));
}

BOOST_AUTO_TEST_SUITE(FrontierDistanceSuite)

namespace {

template<unsigned T_width, unsigned T_height>
struct FrontierWorld : public WorldWithGCExecution<2, T_width, T_height>
{
    using WorldWithGCExecution<2, T_width, T_height>::world;

    MapPoint milBld0Pos, milBld1Pos;
    nobMilitary *milBld0, *milBld1;

    FrontierWorld()
    {
        const GamePlayer& p0 = world.GetPlayer(0);
        const GamePlayer& p1 = world.GetPlayer(1);
        milBld0Pos = p0.GetHQPos() - MapPoint(0, 2);
        milBld1Pos = p1.GetHQPos() - MapPoint(0, 2);
        // Assumed by distributions and sizes
        BOOST_REQUIRE_EQUAL(milBld0Pos.y, milBld1Pos.y);
        // Destroy HQs so only blds are checked
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

DescIdx<TerrainDesc> GetWaterTerrain(const GameWorld& world)
{
    DescIdx<TerrainDesc> tWater(0);
    for(; tWater.value < world.GetDescription().terrain.size(); tWater.value++)
    {
        TerrainDesc fieldDesc = world.GetDescription().get(tWater);
        if(fieldDesc.kind == TerrainKind::Water && !fieldDesc.Is(ETerrain::Walkable))
            return tWater;
    }
    throw std::logic_error("No water"); // LCOV_EXCL_LINE
}
} // namespace

BOOST_FIXTURE_TEST_CASE(FrontierDistanceNear, FrontierWorldSmall)
{
    const DescIdx<TerrainDesc> tWater = GetWaterTerrain(world);

    for(int y = 1; y < world.GetHeight(); y++)
    {
        for(int x = 1; x < world.GetWidth(); x++)
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

        BOOST_REQUIRE_EQUAL(distance0, distance1);
        BOOST_REQUIRE_EQUAL(
          distance0,
          (i == 0 ? FrontierDistance::Near : FrontierDistance::Far)); // near if addon is inactive, otherwise inland
    }
}

BOOST_FIXTURE_TEST_CASE(FrontierDistanceNearOtherFields, FrontierWorldSmall)
{
    for(int terrain = 0; terrain < 2; terrain++)
    {
        TerrainKind searchedTerrain = terrain == 1 ? TerrainKind::Lava : TerrainKind::Snow;
        DescIdx<TerrainDesc> tUnreachable(0);
        for(; tUnreachable.value < world.GetDescription().terrain.size(); tUnreachable.value++)
        {
            TerrainDesc fieldDesc = world.GetDescription().get(tUnreachable);
            if(fieldDesc.kind == searchedTerrain && fieldDesc.Is(ETerrain::Unreachable))
                break;
        }

        for(int y = 1; y < world.GetHeight(); y++)
        {
            for(int x = 1; x < world.GetWidth(); x++)
            {
                MapPoint curPoint(x, y);
                if(curPoint == milBld0Pos || curPoint == milBld1Pos)
                {
                    continue;
                }

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

            BOOST_REQUIRE_EQUAL(distance0, distance1);
            BOOST_REQUIRE_EQUAL(
              distance0,
              (i == 0 ? FrontierDistance::Near : FrontierDistance::Far)); // near if addon is inactive, otherwise inland
        }
    }
}

BOOST_FIXTURE_TEST_CASE(FrontierDistanceMiddle, FrontierWorldMiddle)
{
    const DescIdx<TerrainDesc> tWater = GetWaterTerrain(world);

    for(int y = 1; y < world.GetHeight(); y++)
    {
        for(int x = 1; x < world.GetWidth(); x++)
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

        BOOST_REQUIRE_EQUAL(distance0, distance1);
        BOOST_REQUIRE_EQUAL(
          distance0,
          (i == 0 ? FrontierDistance::Mid : FrontierDistance::Far)); // middle if addon is inactive, otherwise inland
    }
}

BOOST_FIXTURE_TEST_CASE(FrontierDistanceFar, FrontierWorldBig)
{
    const DescIdx<TerrainDesc> tWater = GetWaterTerrain(world);

    for(int y = 1; y < world.GetHeight(); y++)
    {
        for(int x = 1; x < world.GetWidth(); x++)
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

        BOOST_REQUIRE_EQUAL(distance0, distance1);
        BOOST_REQUIRE_EQUAL(distance0, FrontierDistance::Far); // everytime inland
    }
}

BOOST_FIXTURE_TEST_CASE(FrontierDistanceIslandTest, FrontierWorldMiddle)
{
    const DescIdx<TerrainDesc> tWater = GetWaterTerrain(world);

    // Little bit, but walkable water between the 2 buildings (middle of map)
    // and around the border big water
    unsigned middle = world.GetWidth() / 2;
    for(int y = 1; y < world.GetHeight(); y++)
    {
        for(int x = 1; x < world.GetWidth(); x++)
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

        BOOST_REQUIRE_EQUAL(distance0, distance1);
        BOOST_REQUIRE_EQUAL(distance0, FrontierDistance::Mid);
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
using WorldBig = WorldWithGCExecution<2u, 60u, 60u>;
BOOST_FIXTURE_TEST_CASE(FrontierDistanceBug_815, WorldBig)
{
    this->ggs.setSelection(AddonId::FRONTIER_DISTANCE_REACHABLE, 1);

    GamePlayer& p0 = world.GetPlayer(0);
    GamePlayer& p1 = world.GetPlayer(1);

    const DescIdx<TerrainDesc> tWater = GetWaterTerrain(world);

    unsigned middle = world.GetWidth() / 2;

    for(unsigned y = 1; y < world.GetHeight(); y++)
    {
        for(unsigned x = 1; x < world.GetWidth(); x++)
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

    BOOST_REQUIRE_EQUAL(distance0, FrontierDistance::Near);
    BOOST_REQUIRE_EQUAL(distance1, FrontierDistance::Near);
}

BOOST_AUTO_TEST_SUITE_END()
