// Copyright (c) 2016 - 2018 Settlers Freaks (sf-team at siedler25.org)
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
        BOOST_REQUIRE(farm);
        world.BuildRoad(0, false, world.GetNeighbour(farmPt, Direction::SouthEast),
                        std::vector<Direction>(5, Direction::West));
        RTTR_EXEC_TILL(7 * 20 + 60, farm->HasWorker());
        farmer = dynamic_cast<const nofFarmhand*>(farm->GetWorker());
        BOOST_REQUIRE(farmer);
    }
};

BOOST_FIXTURE_TEST_CASE(FarmFieldPlanting, FarmerFixture)
{
    initGameRNG();

    std::vector<MapPoint> radius1Pts = world.GetPointsInRadiusWithCenter(farmPt, 1);
    // First check points for validity
    // Cannot build directly next to farm
    for(MapPoint pt : radius1Pts)
        BOOST_REQUIRE(!farmer->IsPointAvailable(pt));
    // Can build everywhere but on road
    for(unsigned dir = 0; dir < 12; dir++)
    {
        if(dir == 11)
            BOOST_REQUIRE(!farmer->IsPointAvailable(world.GetNeighbour2(farmPt, dir)));
        else
            BOOST_REQUIRE(farmer->IsPointAvailable(world.GetNeighbour2(farmPt, dir)));
    }
    // Not on non-vital terrain
    DescIdx<TerrainDesc> tUnvital(0);
    for(; tUnvital.value < world.GetDescription().terrain.size(); tUnvital.value++)
    {
        if(!world.GetDescription().get(tUnvital).IsVital())
            break;
    }
    world.GetNodeWriteable(world.GetNeighbour2(farmPt, 3)).t1 = tUnvital;
    BOOST_REQUIRE(farmer->IsPointAvailable(world.GetNeighbour2(farmPt, 2)));
    BOOST_REQUIRE(!farmer->IsPointAvailable(world.GetNeighbour2(farmPt, 3)));
    BOOST_REQUIRE(farmer->IsPointAvailable(world.GetNeighbour2(farmPt, 4)));
    world.GetNodeWriteable(world.GetNeighbour2(farmPt, 3)).t2 = tUnvital;
    BOOST_REQUIRE(!farmer->IsPointAvailable(world.GetNeighbour2(farmPt, 3)));
    BOOST_REQUIRE(!farmer->IsPointAvailable(world.GetNeighbour2(farmPt, 4)));
    // Env obj is allowed
    world.SetNO(world.GetNeighbour2(farmPt, 5), new noEnvObject(world.GetNeighbour2(farmPt, 5), 0));
    BOOST_REQUIRE(farmer->IsPointAvailable(world.GetNeighbour2(farmPt, 5)));
    // On bld and next to it is not allowed
    world.SetBuildingSite(BuildingType::Watchtower, world.GetNeighbour2(farmPt, 6), 0);
    BOOST_REQUIRE(!farmer->IsPointAvailable(world.GetNeighbour2(farmPt, 6)));
    BOOST_REQUIRE(!farmer->IsPointAvailable(world.GetNeighbour2(farmPt, 7)));
    // On or next to grain field is not allowed
    const MapPoint grainFieldPos = world.GetNeighbour2(farmPt, 0);
    auto* grainField = new noGrainfield(grainFieldPos);
    world.SetNO(grainFieldPos, grainField);
    BOOST_REQUIRE(!farmer->IsPointAvailable(grainFieldPos));
    BOOST_REQUIRE(!farmer->IsPointAvailable(world.GetNeighbour2(farmPt, 1)));
    const MapPoint grainFieldPos2 = world.GetNeighbour(world.GetNeighbour2(farmPt, 2), Direction::NorthWest);
    world.SetNO(grainFieldPos2, new noGrainfield(grainFieldPos2));
    BOOST_REQUIRE(!farmer->IsPointAvailable(world.GetNeighbour2(farmPt, 0)));

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
        BOOST_REQUIRE(newField.isValid());
        // Let him get there
        RTTR_EXEC_TILL(60, farmer->GetPos() == newField);
        // Start sawing
        RTTR_SKIP_GFS(50);
        BOOST_REQUIRE(!world.GetSpecObj<noGrainfield>(newField));
        if(i == 0)
            world.SetBuildingSite(BuildingType::Woodcutter, newField, 0);
        else
            world.SetBuildingSite(BuildingType::Woodcutter, world.GetNeighbour(newField, Direction::SouthEast), 0);
        // Let farmer return
        RTTR_EXEC_TILL(150, !farm->is_working);
        // Aborted work
        BOOST_REQUIRE(!world.GetSpecObj<noGrainfield>(newField));
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
    BOOST_REQUIRE(world.GetNode(grainFieldPos).reserved);
    // Wait till farmer stopped working
    RTTR_EXEC_TILL(270, !farm->is_working);
    // Grainfield is gone
    BOOST_REQUIRE(!world.GetSpecObj<noGrainfield>(grainFieldPos));
}
