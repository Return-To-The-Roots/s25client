// Copyright (C) 2016 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "GamePlayer.h"
#include "buildings/nobUsual.h"
#include "factories/BuildingFactory.h"
#include "figures/nofFarmhand.h"
#include "worldFixtures/CreateEmptyWorld.h"
#include "worldFixtures/WorldFixture.h"
#include "worldFixtures/initGameRNG.hpp"
#include "nodeObjs/noEnvObject.h"
#include "nodeObjs/noGrainfield.h"
#include <boost/test/unit_test.hpp>

struct FarmerFixture : public WorldFixture<CreateEmptyWorld, 1>
{
    MapPoint farmPt;
    nobUsual* farm;
    const nofFarmhand* farmer;
    FarmerFixture()
    {
        farmPt = world.GetPlayer(0).GetHQPos() + MapPoint(5, 0);
        farm = dynamic_cast<nobUsual*>(
          BuildingFactory::CreateBuilding(world, BuildingType::Farm, farmPt, 0, Nation::Romans));
        BOOST_TEST_REQUIRE(farm);
        world.BuildRoad(0, false, world.GetNeighbour(farmPt, Direction::SouthEast),
                        std::vector<Direction>(5, Direction::West));
        RTTR_EXEC_TILL(7 * 20 + 60, farm->HasWorker());
        farmer = dynamic_cast<const nofFarmhand*>(farm->GetWorker());
        BOOST_TEST_REQUIRE(farmer);
    }
};

BOOST_FIXTURE_TEST_CASE(FarmFieldPlanting, FarmerFixture)
{
    initGameRNG();

    std::vector<MapPoint> radius1Pts = world.GetPointsInRadiusWithCenter(farmPt, 1);
    // First check points for validity
    // Cannot build directly next to farm
    for(MapPoint pt : radius1Pts)
        BOOST_TEST_REQUIRE(!farmer->IsPointAvailable(pt));
    // Can build everywhere but on road
    for(unsigned dir = 0; dir < 12; dir++)
    {
        if(dir == 11)
            BOOST_TEST_REQUIRE(!farmer->IsPointAvailable(world.GetNeighbour2(farmPt, dir)));
        else
            BOOST_TEST_REQUIRE(farmer->IsPointAvailable(world.GetNeighbour2(farmPt, dir)));
    }
    // Not on non-vital terrain
    DescIdx<TerrainDesc> tUnvital(0);
    for(; tUnvital.value < world.GetDescription().terrain.size(); tUnvital.value++)
    {
        if(!world.GetDescription().get(tUnvital).IsVital())
            break;
    }
    world.GetNodeWriteable(world.GetNeighbour2(farmPt, 3)).t1 = tUnvital;
    BOOST_TEST_REQUIRE(farmer->IsPointAvailable(world.GetNeighbour2(farmPt, 2)));
    BOOST_TEST_REQUIRE(!farmer->IsPointAvailable(world.GetNeighbour2(farmPt, 3)));
    BOOST_TEST_REQUIRE(farmer->IsPointAvailable(world.GetNeighbour2(farmPt, 4)));
    world.GetNodeWriteable(world.GetNeighbour2(farmPt, 3)).t2 = tUnvital;
    BOOST_TEST_REQUIRE(!farmer->IsPointAvailable(world.GetNeighbour2(farmPt, 3)));
    BOOST_TEST_REQUIRE(!farmer->IsPointAvailable(world.GetNeighbour2(farmPt, 4)));
    // Env obj is allowed
    world.SetNO(world.GetNeighbour2(farmPt, 5), new noEnvObject(world.GetNeighbour2(farmPt, 5), 0));
    BOOST_TEST_REQUIRE(farmer->IsPointAvailable(world.GetNeighbour2(farmPt, 5)));
    // On bld and next to it is not allowed
    world.SetBuildingSite(BuildingType::Watchtower, world.GetNeighbour2(farmPt, 6), 0);
    BOOST_TEST_REQUIRE(!farmer->IsPointAvailable(world.GetNeighbour2(farmPt, 6)));
    BOOST_TEST_REQUIRE(!farmer->IsPointAvailable(world.GetNeighbour2(farmPt, 7)));
    // On or next to grain field is not allowed
    const MapPoint grainFieldPos = world.GetNeighbour2(farmPt, 0);
    auto* grainField = new noGrainfield(grainFieldPos);
    world.SetNO(grainFieldPos, grainField);
    BOOST_TEST_REQUIRE(!farmer->IsPointAvailable(grainFieldPos));
    BOOST_TEST_REQUIRE(!farmer->IsPointAvailable(world.GetNeighbour2(farmPt, 1)));
    const MapPoint grainFieldPos2 = world.GetNeighbour(world.GetNeighbour2(farmPt, 2), Direction::NorthWest);
    world.SetNO(grainFieldPos2, new noGrainfield(grainFieldPos2));
    BOOST_TEST_REQUIRE(!farmer->IsPointAvailable(world.GetNeighbour2(farmPt, 0)));

    // Test farmer behaviour
    for(unsigned i = 0; i < 2; i++)
    {
        // Let the farmer pick a field
        RTTR_EXEC_TILL(150, farm->is_working);
        MapPoint newField = MapPoint::Invalid();
        // Find field, only these 3 remaining
        for(unsigned dir = 8; dir < 11; dir++)
        {
            if(world.GetNode(world.GetNeighbour2(farmPt, dir)).reserved)
            {
                newField = world.GetNeighbour2(farmPt, dir);
                break;
            }
        }
        BOOST_TEST_REQUIRE(newField.isValid());
        // Let him get there
        RTTR_EXEC_TILL(60, farmer->GetPos() == newField);
        // Start sawing
        RTTR_SKIP_GFS(50);
        BOOST_TEST_REQUIRE(!world.GetSpecObj<noGrainfield>(newField));
        if(i == 0)
            world.SetBuildingSite(BuildingType::Woodcutter, newField, 0);
        else
            world.SetBuildingSite(BuildingType::Woodcutter, world.GetNeighbour(newField, Direction::SouthEast), 0);
        // Let farmer return
        RTTR_EXEC_TILL(150, !farm->is_working);
        // Aborted work
        BOOST_TEST_REQUIRE(!world.GetSpecObj<noGrainfield>(newField));
        // And remove
        if(i == 0)
            world.DestroyFlag(world.GetNeighbour(newField, Direction::SouthEast), 0);
        else
            world.DestroyBuilding(world.GetNeighbour(newField, Direction::SouthEast), 0);
    }

    RTTR_EXEC_TILL(3000, grainField->IsHarvestable());
    // Wait till farmer stopped working
    RTTR_EXEC_TILL(150, !farm->is_working);
    // And started again
    RTTR_EXEC_TILL(150, farm->is_working);
    // Should pick this
    BOOST_TEST_REQUIRE(world.GetNode(grainFieldPos).reserved);
    // Wait till farmer stopped working
    RTTR_EXEC_TILL(270, !farm->is_working);
    // Grainfield is gone
    BOOST_TEST_REQUIRE(!world.GetSpecObj<noGrainfield>(grainFieldPos));
}
