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
        if(fieldDesc.kind == TerrainKind::WATER && !fieldDesc.Is(ETerrain::Walkable) && !fieldDesc.Is(ETerrain::Unreachable))
            break;
    }

    for(int y = 1; y < world.GetHeight(); y++)
    {
        for(int x = 1; x < world.GetWidth(); x++)
        {
            MapPoint curPoint(x, y);
            if(curPoint == milBld0Pos)
            {
                continue;
            }
            if(curPoint == p0.GetHQPos())
            {
                continue;
            }
            if(curPoint == milBld1Pos)
            {
                continue;
            }
            if(curPoint == p1.GetHQPos())
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
            if(fieldDesc.kind == searchedTerrain && (!fieldDesc.Is(ETerrain::Walkable) || fieldDesc.Is(ETerrain::Unreachable)))
                break;
        }

        for(int y = 1; y < world.GetHeight(); y++)
        {
            for(int x = 1; x < world.GetWidth(); x++)
            {
                MapPoint curPoint(x, y);
                if(curPoint == milBld0Pos)
                {
                    continue;
                }
                if(curPoint == p0.GetHQPos())
                {
                    continue;
                }
                if(curPoint == milBld1Pos)
                {
                    continue;
                }
                if(curPoint == p1.GetHQPos())
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
        if(fieldDesc.kind == TerrainKind::WATER && !fieldDesc.Is(ETerrain::Walkable) && !fieldDesc.Is(ETerrain::Unreachable))
            break;
    }

    for(int y = 1; y < world.GetHeight(); y++)
    {
        for(int x = 1; x < world.GetWidth(); x++)
        {
            MapPoint curPoint(x, y);
            if(curPoint == milBld0Pos)
            {
                continue;
            }
            if(curPoint == p0.GetHQPos())
            {
                continue;
            }
            if(curPoint == milBld1Pos)
            {
                continue;
            }
            if(curPoint == p1.GetHQPos())
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
        if(fieldDesc.kind == TerrainKind::WATER && !fieldDesc.Is(ETerrain::Walkable) && !fieldDesc.Is(ETerrain::Unreachable))
            break;
    }

    for(int y = 1; y < world.GetHeight(); y++)
    {
        for(int x = 1; x < world.GetWidth(); x++)
        {
            MapPoint curPoint(x, y);
            if(curPoint == milBld0Pos)
            {
                continue;
            }
            if(curPoint == p0.GetHQPos())
            {
                continue;
            }
            if(curPoint == milBld1Pos)
            {
                continue;
            }
            if(curPoint == p1.GetHQPos())
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

BOOST_FIXTURE_TEST_CASE(IslandTest, FrontierWorldMiddle)
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
        if(fieldDesc.kind == TerrainKind::WATER && !fieldDesc.Is(ETerrain::Walkable) && !fieldDesc.Is(ETerrain::Unreachable))
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

BOOST_AUTO_TEST_SUITE_END()
