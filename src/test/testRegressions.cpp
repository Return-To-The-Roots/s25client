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
#include "buildings/nobBaseWarehouse.h"
#include "GamePlayer.h"
#include "nodeObjs/noEnvObject.h"
#include "nodeObjs/noGrainfield.h"
#include "buildings/nobUsual.h"
#include "factories/BuildingFactory.h"
#include "figures/nofFarmhand.h"
#include "EventManager.h"
#include "gameData/ShieldConsts.h"
#include "RTTR_AssertError.h"
#include "test/testHelpers.h"
#include "test/PointOutput.h"
#include "test/WorldFixture.h"
#include "test/CreateEmptyWorld.h"
#include <boost/test/unit_test.hpp>
#include <boost/array.hpp>
#include <boost/foreach.hpp>

// This suite tests bugs that got fixed to avoid regressions
// So:
//      - Before you fix a bug, reproduce it here (or in a fitting suite) -> Test fails
//      - Fix the bug -> Test succeeds
BOOST_AUTO_TEST_SUITE(RegressionsSuite)

struct AddGoodsFixture: public WorldFixture<CreateEmptyWorld, 1, 10, 10>
{
    boost::array<unsigned, JOB_TYPES_COUNT> numPeople, numPeoplePlayer;
    boost::array<unsigned, WARE_TYPES_COUNT> numGoods, numGoodsPlayer;
    AddGoodsFixture()
    {
        GamePlayer& player = world.GetPlayer(0);
        // Don't keep any reserve
        for(unsigned i = 0; i <= this->ggs.GetMaxMilitaryRank(); ++i)
            player.GetFirstWH()->SetRealReserve(i, 0);
        numPeople = numPeoplePlayer = player.GetInventory().people;
        numGoods = numGoodsPlayer = player.GetInventory().goods;
    }

    /// Asserts that the expected and actual good count match for the HQ
    void testGoodsCountHQ()
    {
        nobBaseWarehouse& hq = *world.GetSpecObj<nobBaseWarehouse>(world.GetPlayer(0).GetHQPos());
        for(unsigned i = 0; i < JOB_TYPES_COUNT; i++)
        {
            BOOST_REQUIRE_EQUAL(hq.GetVisualFiguresCount(Job(i)), numPeople[i]);
            BOOST_REQUIRE_EQUAL(hq.GetRealFiguresCount(Job(i)), numPeople[i]);
        }
        for(unsigned i = 0; i < WARE_TYPES_COUNT; i++)
        {
            BOOST_REQUIRE_EQUAL(hq.GetVisualWaresCount(GoodType(i)), numGoods[i]);
            BOOST_REQUIRE_EQUAL(hq.GetRealWaresCount(GoodType(i)), numGoods[i]);
        }
    }
    /// Asserts that the expected and actual good count match for the player
    void testGoodsCountPlayer()
    {
        GamePlayer& player = world.GetPlayer(0);
        for(unsigned i = 0; i < JOB_TYPES_COUNT; i++)
            BOOST_REQUIRE_EQUAL(player.GetInventory().people[i], numPeoplePlayer[i]);
        for(unsigned i = 0; i < WARE_TYPES_COUNT; i++)
            BOOST_REQUIRE_EQUAL(player.GetInventory().goods[i], numGoodsPlayer[i]);
    }
};

BOOST_FIXTURE_TEST_CASE(AddGoods, AddGoodsFixture)
{
    GamePlayer& player = world.GetPlayer(0);
    nobBaseWarehouse& hq = *world.GetSpecObj<nobBaseWarehouse>(player.GetHQPos());
    testGoodsCountHQ();

    // Add nothing -> nothing changed
    Inventory newGoods;
    hq.AddGoods(newGoods, true);
    testGoodsCountHQ();
    testGoodsCountPlayer();

    // Add jobs
    for(unsigned i = 0; i < JOB_TYPES_COUNT; i++)
    {
        // Boat carrier gets divided upfront
        if(Job(i) == JOB_BOATCARRIER)
            continue;
        newGoods.Add(Job(i), i + 1);
        numPeople[i] += i + 1;
    }
    numPeoplePlayer = numPeople;
    hq.AddGoods(newGoods, true);
    testGoodsCountHQ();
    testGoodsCountPlayer();

    // Add only to hq but not to player
    for(unsigned i = 0; i < JOB_TYPES_COUNT; i++)
        numPeople[i] += newGoods.people[i];
    hq.AddGoods(newGoods, false);
    testGoodsCountHQ();
    testGoodsCountPlayer();

    // Add wares
    newGoods.clear();
    for(unsigned i = 0; i < WARE_TYPES_COUNT; i++)
    {
        // Only romand shields get added
        if(ConvertShields(GoodType(i)) == GD_SHIELDROMANS && GoodType(i) != GD_SHIELDROMANS)
            continue;
        newGoods.Add(GoodType(i), i + 2);
        numGoods[i] += i + 2;
    }
    numGoodsPlayer = numGoods;
    hq.AddGoods(newGoods, true);
    testGoodsCountHQ();
    testGoodsCountPlayer();

    // Add only to hq but not to player
    for(unsigned i = 0; i < WARE_TYPES_COUNT; i++)
        numGoods[i] += newGoods.goods[i];
    hq.AddGoods(newGoods, false);
    testGoodsCountHQ();
    testGoodsCountPlayer();

#if RTTR_ENABLE_ASSERTS
    RTTR_AssertEnableBreak = false;
    newGoods.clear();
    newGoods.Add(JOB_BOATCARRIER);
    BOOST_CHECK_THROW(hq.AddGoods(newGoods, false), RTTR_AssertError);
    newGoods.clear();
    newGoods.Add(GD_SHIELDAFRICANS);
    BOOST_CHECK_THROW(hq.AddGoods(newGoods, false), RTTR_AssertError);
    RTTR_AssertEnableBreak = true;
#endif
}

struct FarmerFixture: public WorldFixture<CreateEmptyWorld, 1, 20, 20>
{
    MapPoint farmPt;
    nobUsual* farm;
    const nofFarmhand* farmer;
    FarmerFixture()
    {
        farmPt = world.GetPlayer(0).GetHQPos() + MapPoint(5, 0);
        farm = dynamic_cast<nobUsual*>(BuildingFactory::CreateBuilding(world, BLD_FARM, farmPt, 0, NAT_ROMANS));
        BOOST_REQUIRE(farm);
        world.BuildRoad(0, false, world.GetNeighbour(farmPt, Direction::SOUTHEAST), std::vector<unsigned char>(5, Direction::WEST));
        for(unsigned gf = 0; gf < 7 * 20 + 60; gf++)
        {
            em.ExecuteNextGF();
            if(farm->HasWorker())
                break;
        }
        BOOST_REQUIRE(farm->HasWorker());
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
    BOOST_FOREACH(MapPoint pt, radius1Pts)
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
    world.GetNodeWriteable(world.GetNeighbour2(farmPt, 3)).t1 = TT_MOUNTAINMEADOW;
    BOOST_REQUIRE(farmer->IsPointAvailable(world.GetNeighbour2(farmPt, 2)));
    BOOST_REQUIRE(!farmer->IsPointAvailable(world.GetNeighbour2(farmPt, 3)));
    BOOST_REQUIRE(farmer->IsPointAvailable(world.GetNeighbour2(farmPt, 4)));
    world.GetNodeWriteable(world.GetNeighbour2(farmPt, 3)).t2 = TT_MOUNTAINMEADOW;
    BOOST_REQUIRE(!farmer->IsPointAvailable(world.GetNeighbour2(farmPt, 3)));
    BOOST_REQUIRE(!farmer->IsPointAvailable(world.GetNeighbour2(farmPt, 4)));
    // Env obj is allowed
    world.SetNO(world.GetNeighbour2(farmPt, 5), new noEnvObject(world.GetNeighbour2(farmPt, 5), 0));
    BOOST_REQUIRE(farmer->IsPointAvailable(world.GetNeighbour2(farmPt, 5)));
    // On bld and next to it is not allowed
    world.SetBuildingSite(BLD_WATCHTOWER, world.GetNeighbour2(farmPt, 6), 0);
    BOOST_REQUIRE(!farmer->IsPointAvailable(world.GetNeighbour2(farmPt, 6)));
    BOOST_REQUIRE(!farmer->IsPointAvailable(world.GetNeighbour2(farmPt, 7)));
    // On or next to grain field is not allowed
    const MapPoint grainFieldPos = world.GetNeighbour2(farmPt, 0);
    noGrainfield* grainField = new noGrainfield(grainFieldPos);
    world.SetNO(grainFieldPos, grainField);
    BOOST_REQUIRE(!farmer->IsPointAvailable(grainFieldPos));
    BOOST_REQUIRE(!farmer->IsPointAvailable(world.GetNeighbour2(farmPt, 1)));
    const MapPoint grainFieldPos2 = world.GetNeighbour(world.GetNeighbour2(farmPt, 2), Direction::NORTHWEST);
    world.SetNO(grainFieldPos2, new noGrainfield(grainFieldPos2));
    BOOST_REQUIRE(!farmer->IsPointAvailable(world.GetNeighbour2(farmPt, 0)));

    // Test farmer behaviour
    for(unsigned i = 0; i < 2; i++)
    {
        // Let the farmer pick a field
        for(unsigned gf = 0; gf < 150; gf++)
        {
            em.ExecuteNextGF();
            if(farm->is_working)
                break;
        }
        BOOST_REQUIRE(farm->is_working);
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
        for(unsigned gf = 0; gf < 60; gf++)
        {
            em.ExecuteNextGF();
            if(farmer->GetPos() == newField)
                break;
        }
        BOOST_REQUIRE_EQUAL(farmer->GetPos(), newField);
        // Start sawing
        for(unsigned gf = 0; gf < 50; gf++)
            em.ExecuteNextGF();
        BOOST_REQUIRE(!world.GetSpecObj<noGrainfield>(newField));
        if(i == 0)
            world.SetBuildingSite(BLD_WOODCUTTER, newField, 0);
        else
            world.SetBuildingSite(BLD_WOODCUTTER, world.GetNeighbour(newField, Direction::SOUTHEAST), 0);
        // Let farmer return
        for(unsigned gf = 0; gf < 150; gf++)
        {
            em.ExecuteNextGF();
            if(!farm->is_working)
                break;
        }
        BOOST_REQUIRE(!farm->is_working);
        // Aborted work
        BOOST_REQUIRE(!world.GetSpecObj<noGrainfield>(newField));
        // And remove
        if(i == 0)
            world.DestroyFlag(world.GetNeighbour(newField, Direction::SOUTHEAST), 0);
        else
            world.DestroyBuilding(world.GetNeighbour(newField, Direction::SOUTHEAST), 0);
    }

    for(unsigned gf = 0; gf < 3000; gf++)
    {
        em.ExecuteNextGF();
        if(grainField->IsHarvestable())
            break;
    }
    BOOST_REQUIRE(grainField->IsHarvestable());
    // Wait till farmer stopped working
    for(unsigned gf = 0; gf < 150; gf++)
    {
        if(!farm->is_working)
            break;
        em.ExecuteNextGF();
    }
    BOOST_REQUIRE(!farm->is_working);
    // And started again
    for(unsigned gf = 0; gf < 150; gf++)
    {
        if(farm->is_working)
            break;
        em.ExecuteNextGF();
    }
    BOOST_REQUIRE(farm->is_working);
    // Should pick this
    BOOST_REQUIRE(world.GetNode(grainFieldPos).reserved);
    // Wait till farmer stopped working
    for(unsigned gf = 0; gf < 270; gf++)
    {
        if(!farm->is_working)
            break;
        em.ExecuteNextGF();
    }
    BOOST_REQUIRE(!farm->is_working);
    // Grainfield is gone
    BOOST_REQUIRE(!world.GetSpecObj<noGrainfield>(grainFieldPos));
}

BOOST_AUTO_TEST_SUITE_END()
