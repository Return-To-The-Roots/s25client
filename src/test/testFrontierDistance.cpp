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

#include "addons/Addon.h"
#include "buildings/nobMilitary.h"
#include "factories/BuildingFactory.h"
#include "test/WorldWithGCExecution.h"
#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_SUITE(FrontierDistance)

typedef WorldWithGCExecution<2, 20u, 20u> FrontierWorldSmall;
typedef WorldWithGCExecution<2, 38u, 38u> FrontierWorldMiddle;
typedef WorldWithGCExecution<2, 60u, 60u> FrontierWorldBig;

BOOST_FIXTURE_TEST_CASE(FrontierDistanceNear, FrontierWorldSmall)
{
    GamePlayer& p0 = world.GetPlayer(0);
    MapPoint milBld0Pos = p0.GetHQPos() - MapPoint(0, 2);
    GamePlayer& p1 = world.GetPlayer(1);
    MapPoint milBld1Pos = p1.GetHQPos() - MapPoint(0, 2);
    nobMilitary* milBld0 = dynamic_cast<nobMilitary*>(BuildingFactory::CreateBuilding(world, BLD_WATCHTOWER, milBld0Pos, 0, NAT_ROMANS));
    nobMilitary* milBld1 = dynamic_cast<nobMilitary*>(BuildingFactory::CreateBuilding(world, BLD_WATCHTOWER, milBld1Pos, 1, NAT_VIKINGS));

    DescIdx<TerrainDesc> tWater(0);
    for(; tWater.value < world.GetDescription().terrain.size(); tWater.value++)
    {
        TerrainDesc fieldDesc = world.GetDescription().get(tWater);
        if(fieldDesc.kind == TerrainKind::WATER && !fieldDesc.Is(ETerrain::Walkable))
            break;
    }

    for(int y = 1; y < world.GetHeight(); y++)
    {
        for(int x = 1; x < world.GetWidth(); x++)
        {
            MapPoint curPoint(x, y);
            if(curPoint == milBld0Pos || curPoint == p0.GetHQPos() || curPoint == milBld1Pos || curPoint == p1.GetHQPos())
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
        p0.RecalcMilitaryFlags();
        p1.RecalcMilitaryFlags();

        unsigned distance0 = milBld0->GetFrontierDistance();
        unsigned distance1 = milBld1->GetFrontierDistance();

        BOOST_REQUIRE_EQUAL(distance0, distance1);
        BOOST_REQUIRE_EQUAL(distance0 + i * 100, (i == 0 ? 3u : 0u) + i * 100); // near if addon is inactive, otherwise inland
    }
}

BOOST_FIXTURE_TEST_CASE(FrontierDistanceNearOtherFields, FrontierWorldSmall)
{
    GamePlayer& p0 = world.GetPlayer(0);
    MapPoint milBld0Pos = p0.GetHQPos() - MapPoint(0, 2);
    GamePlayer& p1 = world.GetPlayer(1);
    MapPoint milBld1Pos = p1.GetHQPos() - MapPoint(0, 2);
    nobMilitary* milBld0 = dynamic_cast<nobMilitary*>(BuildingFactory::CreateBuilding(world, BLD_WATCHTOWER, milBld0Pos, 0, NAT_ROMANS));
    nobMilitary* milBld1 = dynamic_cast<nobMilitary*>(BuildingFactory::CreateBuilding(world, BLD_WATCHTOWER, milBld1Pos, 1, NAT_VIKINGS));

    for(int terrain = 0; terrain < 2; terrain++)
    {
        TerrainKind searchedTerrain = terrain == 1 ? TerrainKind::LAVA : TerrainKind::SNOW;
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
                if(curPoint == milBld0Pos || curPoint == p0.GetHQPos() || curPoint == milBld1Pos || curPoint == p1.GetHQPos())
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
            p0.RecalcMilitaryFlags();
            p1.RecalcMilitaryFlags();

            unsigned distance0 = milBld0->GetFrontierDistance();
            unsigned distance1 = milBld1->GetFrontierDistance();

            BOOST_REQUIRE_EQUAL(distance0, distance1);
            BOOST_REQUIRE_EQUAL(distance0 + i * 100, (i == 0 ? 3u : 0u) + i * 100); // near if addon is inactive, otherwise inland
        }
    }
}

BOOST_FIXTURE_TEST_CASE(FrontierDistanceMiddle, FrontierWorldMiddle)
{
    GamePlayer& p0 = world.GetPlayer(0);
    MapPoint milBld0Pos = p0.GetHQPos() - MapPoint(0, 2);
    GamePlayer& p1 = world.GetPlayer(1);
    MapPoint milBld1Pos = p1.GetHQPos() - MapPoint(0, 2);
    nobMilitary* milBld0 = dynamic_cast<nobMilitary*>(BuildingFactory::CreateBuilding(world, BLD_WATCHTOWER, milBld0Pos, 0, NAT_ROMANS));
    nobMilitary* milBld1 = dynamic_cast<nobMilitary*>(BuildingFactory::CreateBuilding(world, BLD_WATCHTOWER, milBld1Pos, 1, NAT_VIKINGS));

    DescIdx<TerrainDesc> tWater(0);
    for(; tWater.value < world.GetDescription().terrain.size(); tWater.value++)
    {
        TerrainDesc fieldDesc = world.GetDescription().get(tWater);
        if(fieldDesc.kind == TerrainKind::WATER && !fieldDesc.Is(ETerrain::Walkable))
            break;
    }

    for(int y = 1; y < world.GetHeight(); y++)
    {
        for(int x = 1; x < world.GetWidth(); x++)
        {
            MapPoint curPoint(x, y);
            if(curPoint == milBld0Pos || curPoint == p0.GetHQPos() || curPoint == milBld1Pos || curPoint == p1.GetHQPos())
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
        p0.RecalcMilitaryFlags();
        p1.RecalcMilitaryFlags();

        unsigned distance0 = milBld0->GetFrontierDistance();
        unsigned distance1 = milBld1->GetFrontierDistance();

        BOOST_REQUIRE_EQUAL(distance0, distance1);
        BOOST_REQUIRE_EQUAL(distance0 + i * 100, (i == 0 ? 1u : 0u) + i * 100); // middle if addon is inactive, otherwise inland
    }
}

BOOST_FIXTURE_TEST_CASE(FrontierDistanceFar, FrontierWorldBig)
{
    GamePlayer& p0 = world.GetPlayer(0);
    MapPoint milBld0Pos = p0.GetHQPos() - MapPoint(0, 2);
    GamePlayer& p1 = world.GetPlayer(1);
    MapPoint milBld1Pos = p1.GetHQPos() - MapPoint(0, 2);
    nobMilitary* milBld0 = dynamic_cast<nobMilitary*>(BuildingFactory::CreateBuilding(world, BLD_WATCHTOWER, milBld0Pos, 0, NAT_ROMANS));
    nobMilitary* milBld1 = dynamic_cast<nobMilitary*>(BuildingFactory::CreateBuilding(world, BLD_WATCHTOWER, milBld1Pos, 1, NAT_VIKINGS));

    DescIdx<TerrainDesc> tWater(0);
    for(; tWater.value < world.GetDescription().terrain.size(); tWater.value++)
    {
        TerrainDesc fieldDesc = world.GetDescription().get(tWater);
        if(fieldDesc.kind == TerrainKind::WATER && !fieldDesc.Is(ETerrain::Walkable))
            break;
    }

    for(int y = 1; y < world.GetHeight(); y++)
    {
        for(int x = 1; x < world.GetWidth(); x++)
        {
            MapPoint curPoint(x, y);
            if(curPoint == milBld0Pos || curPoint == p0.GetHQPos() || curPoint == milBld1Pos || curPoint == p1.GetHQPos())
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
        p0.RecalcMilitaryFlags();
        p1.RecalcMilitaryFlags();

        unsigned distance0 = milBld0->GetFrontierDistance();
        unsigned distance1 = milBld1->GetFrontierDistance();

        BOOST_REQUIRE_EQUAL(distance0, distance1);
        BOOST_REQUIRE_EQUAL(distance0 + i * 100, 0u + i * 100); // everytime inland
    }
}

BOOST_FIXTURE_TEST_CASE(FrontierDistanceIslandTest, FrontierWorldMiddle)
{
    GamePlayer& p0 = world.GetPlayer(0);
    MapPoint milBld0Pos = p0.GetHQPos() + MapPoint(5, 0);
    GamePlayer& p1 = world.GetPlayer(1);
    MapPoint milBld1Pos = p1.GetHQPos() - MapPoint(5, 0);
    nobMilitary* milBld0 = dynamic_cast<nobMilitary*>(BuildingFactory::CreateBuilding(world, BLD_WATCHTOWER, milBld0Pos, 0, NAT_ROMANS));
    nobMilitary* milBld1 = dynamic_cast<nobMilitary*>(BuildingFactory::CreateBuilding(world, BLD_WATCHTOWER, milBld1Pos, 1, NAT_VIKINGS));

    DescIdx<TerrainDesc> tWater(0);
    for(; tWater.value < world.GetDescription().terrain.size(); tWater.value++)
    {
        TerrainDesc fieldDesc = world.GetDescription().get(tWater);
        if(fieldDesc.kind == TerrainKind::WATER && !fieldDesc.Is(ETerrain::Walkable))
            break;
    }

    unsigned middle = world.GetWidth() / 2;

    for(int y = 1; y < world.GetHeight(); y++)
    {
        for(int x = 1; x < world.GetWidth(); x++)
        {
            MapPoint curPoint(x, y);

            if(curPoint.x < 5 || curPoint.x > world.GetWidth() - 5 || curPoint.y < 5 || curPoint.y > world.GetHeight() - 5
               || (curPoint.x >= middle - 1 && curPoint.x <= middle))
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
        p0.RecalcMilitaryFlags();
        p1.RecalcMilitaryFlags();

        unsigned distance0 = milBld0->GetFrontierDistance();
        unsigned distance1 = milBld1->GetFrontierDistance();

        BOOST_REQUIRE_EQUAL(distance0, distance1);
        BOOST_REQUIRE_EQUAL(distance0 + i * 100, 3u + i * 100); // near
    }
}

BOOST_FIXTURE_TEST_CASE(FrontierDistanceBug_815, FrontierWorldBig)
{
    this->ggs.setSelection(AddonId::FRONTIER_DISTANCE_REACHABLE, 1);

    GamePlayer& p0 = world.GetPlayer(0);
    GamePlayer& p1 = world.GetPlayer(1);

    DescIdx<TerrainDesc> tWater(0);
    for(; tWater.value < world.GetDescription().terrain.size(); tWater.value++)
    {
        TerrainDesc fieldDesc = world.GetDescription().get(tWater);
        if(fieldDesc.kind == TerrainKind::WATER && !fieldDesc.Is(ETerrain::Walkable))
            break;
    }

    unsigned middle = world.GetWidth() / 2;

    for(unsigned y = 1; y < world.GetHeight(); y++)
    {
        for(unsigned x = 1; x < world.GetWidth(); x++)
        {
            MapPoint curPoint(x, y);

            // get an island
            if(curPoint.x < 10 || curPoint.x > world.GetWidth() - 10 || curPoint.y < 10 || curPoint.y > world.GetHeight() - 10)
            {
                MapNode& mapPoint = world.GetNodeWriteable(curPoint);
                mapPoint.t1 = tWater;
                mapPoint.t2 = tWater;
                continue;
            }

            // get bottleneck'ed passage on south of the island
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
    BuildingFactory::CreateBuilding(world, BLD_WATCHTOWER, p1Far, p1.GetPlayerId(), NAT_ROMANS);

    // p1 s building, which should cause a frontier distance "near"
    MapPoint p1Near(middle + 5, 15); // side of p0 in bottleneck
    nobMilitary* milBld1 =
      dynamic_cast<nobMilitary*>(BuildingFactory::CreateBuilding(world, BLD_WATCHTOWER, p1Near, p1.GetPlayerId(), NAT_ROMANS));

    // p0 s building, should be near, like p1 s but, will be far cause p1Far cant be reached (patch is longer then 40 units).
    // It will override the NEAR-Distance, while iteration.
    MapPoint p0Near(middle - 5, 15);
    nobMilitary* milBld0 =
      dynamic_cast<nobMilitary*>(BuildingFactory::CreateBuilding(world, BLD_WATCHTOWER, p0Near, p0.GetPlayerId(), NAT_ROMANS));

    unsigned distance0 = milBld0->GetFrontierDistance();
    unsigned distance1 = milBld1->GetFrontierDistance();

    BOOST_REQUIRE_EQUAL(distance0, 3u);
    BOOST_REQUIRE_EQUAL(distance1, 3u);
}

BOOST_AUTO_TEST_SUITE_END()