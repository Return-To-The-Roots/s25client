// Copyright (C) 2005 - 2026 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "GamePlayer.h"
#include "buildings/nobUsual.h"
#include "factories/BuildingFactory.h"
#include "figures/nofFarmhand.h"
#include "figures/nofForester.h"
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
        addStartResources();
        farmPt = world.getPlayer(0).getHQPos() + MapPoint(5, 0);
        farm = dynamic_cast<nobUsual*>(
          BuildingFactory::CreateBuilding(world, BuildingType::Farm, farmPt, 0, Nation::Romans));
        BOOST_TEST_REQUIRE(farm);
        world.BuildRoad(0, false, world.getNeighbour(farmPt, Direction::SouthEast),
                        std::vector<Direction>(5, Direction::West));
        RTTR_EXEC_TILL(7 * 20 + 60, farm->hasWorker());
        farmer = dynamic_cast<const nofFarmhand*>(farm->getWorker());
        BOOST_TEST_REQUIRE(farmer);
        initGameRNG();
    }
};

BOOST_FIXTURE_TEST_CASE(ForesterAvoidsPotentialFarmFieldSpots, FarmerFixture)
{
    const auto isPointAvailable = [](const nofFarmhand& worker, const MapPoint pt) {
        return worker.getPointQuality(pt) != nofFarmhand::PointQuality::NotPossible;
    };

    const MapPoint fieldPt = world.getNeighbour2(farmPt, 0);
    BOOST_TEST_REQUIRE(isPointAvailable(*farmer, fieldPt));

    const MapPoint foresterPt = world.MakeMapPoint(world.getPlayer(0).getHQPos() - Position(5, 0));
    auto* foresterBuilding = dynamic_cast<nobUsual*>(
      BuildingFactory::CreateBuilding(world, BuildingType::Forester, foresterPt, 0, Nation::Romans));
    BOOST_TEST_REQUIRE(foresterBuilding);

    nofForester foresterWorker(world.getNeighbour(foresterPt, Direction::SouthEast), 0, foresterBuilding);

    BOOST_TEST_REQUIRE(isPointAvailable(foresterWorker, fieldPt));

    ggs.setSelection(AddonId::FORESTER_FARM_FIELD_AVOIDANCE, 1);
    BOOST_TEST_REQUIRE(!isPointAvailable(foresterWorker, fieldPt));
}

BOOST_FIXTURE_TEST_CASE(FarmFieldPlanting, FarmerFixture)
{
    const auto isPointAvailable = [farmer = this->farmer](const MapPoint pt) {
        return farmer->getPointQuality(pt) != nofFarmhand::PointQuality::NotPossible;
    };

    std::vector<MapPoint> radius1Pts = world.GetPointsInRadiusWithCenter(farmPt, 1);
    // First check points for validity
    // Cannot build directly next to farm
    for(MapPoint pt : radius1Pts)
        BOOST_TEST_REQUIRE(!isPointAvailable(pt));
    // Can build everywhere but on road
    for(unsigned dir = 0; dir < 12; dir++)
    {
        if(dir == 11)
            BOOST_TEST_REQUIRE(!isPointAvailable(world.getNeighbour2(farmPt, dir)));
        else
            BOOST_TEST_REQUIRE(isPointAvailable(world.getNeighbour2(farmPt, dir)));
    }
    // Not on non-vital terrain
    const auto tUnvital = world.getDescription().terrain.find([](const TerrainDesc& t) { return !t.isVital(); });
    world.getNodeWriteable(world.getNeighbour2(farmPt, 3)).t1 = tUnvital;
    BOOST_TEST_REQUIRE(isPointAvailable(world.getNeighbour2(farmPt, 2)));
    BOOST_TEST_REQUIRE(!isPointAvailable(world.getNeighbour2(farmPt, 3)));
    BOOST_TEST_REQUIRE(isPointAvailable(world.getNeighbour2(farmPt, 4)));
    world.getNodeWriteable(world.getNeighbour2(farmPt, 3)).t2 = tUnvital;
    BOOST_TEST_REQUIRE(!isPointAvailable(world.getNeighbour2(farmPt, 3)));
    BOOST_TEST_REQUIRE(!isPointAvailable(world.getNeighbour2(farmPt, 4)));
    // Env obj is allowed
    world.setNO(world.getNeighbour2(farmPt, 5), new noEnvObject(world.getNeighbour2(farmPt, 5), 0));
    BOOST_TEST_REQUIRE(isPointAvailable(world.getNeighbour2(farmPt, 5)));
    // On bld and next to it is not allowed
    world.SetBuildingSite(BuildingType::Watchtower, world.getNeighbour2(farmPt, 6), 0);
    BOOST_TEST_REQUIRE(!isPointAvailable(world.getNeighbour2(farmPt, 6)));
    BOOST_TEST_REQUIRE(!isPointAvailable(world.getNeighbour2(farmPt, 7)));
    // On or next to grain field is not allowed
    const MapPoint grainFieldPos = world.getNeighbour2(farmPt, 0);
    auto* grainField = new noGrainfield(grainFieldPos);
    world.setNO(grainFieldPos, grainField);
    BOOST_TEST_REQUIRE(!isPointAvailable(grainFieldPos));
    BOOST_TEST_REQUIRE(!isPointAvailable(world.getNeighbour2(farmPt, 1)));
    const MapPoint grainFieldPos2 = world.getNeighbour(world.getNeighbour2(farmPt, 2), Direction::NorthWest);
    world.setNO(grainFieldPos2, new noGrainfield(grainFieldPos2));
    BOOST_TEST_REQUIRE(!isPointAvailable(world.getNeighbour2(farmPt, 0)));

    // Test farmer behavior
    for(unsigned i = 0; i < 2; i++)
    {
        // Let the farmer pick a field
        RTTR_EXEC_TILL(150, farm->is_working);
        MapPoint newField = MapPoint::Invalid();
        // Find field, only these 3 remaining
        for(unsigned dir = 8; dir < 11; dir++)
        {
            if(world.getNode(world.getNeighbour2(farmPt, dir)).reserved)
            {
                newField = world.getNeighbour2(farmPt, dir);
                break;
            }
        }
        BOOST_TEST_REQUIRE(newField.isValid());
        // Let him get there
        RTTR_EXEC_TILL(60, farmer->getPos() == newField);
        // Start sawing
        RTTR_SKIP_GFS(50);
        BOOST_TEST_REQUIRE(!world.getSpecObj<noGrainfield>(newField));
        if(i == 0)
            world.SetBuildingSite(BuildingType::Woodcutter, newField, 0);
        else
            world.SetBuildingSite(BuildingType::Woodcutter, world.getNeighbour(newField, Direction::SouthEast), 0);
        // Let farmer return
        RTTR_EXEC_TILL(150, !farm->is_working);
        // Aborted work
        BOOST_TEST_REQUIRE(!world.getSpecObj<noGrainfield>(newField));
        // And remove
        if(i == 0)
            world.DestroyFlag(world.getNeighbour(newField, Direction::SouthEast), 0);
        else
            world.DestroyBuilding(world.getNeighbour(newField, Direction::SouthEast), 0);
    }

    RTTR_EXEC_TILL(3000, grainField->isHarvestable());
    // Wait till farmer stopped working
    RTTR_EXEC_TILL(150, !farm->is_working);
    // And started again
    RTTR_EXEC_TILL(150, farm->is_working);
    // Should pick this
    BOOST_TEST_REQUIRE(world.getNode(grainFieldPos).reserved);
    // Wait till farmer stopped working
    RTTR_EXEC_TILL(270, !farm->is_working);
    // Grainfield is gone
    BOOST_TEST_REQUIRE(!world.getSpecObj<noGrainfield>(grainFieldPos));
}
