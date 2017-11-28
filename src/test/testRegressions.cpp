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
#include "EventManager.h"
#include "GamePlayer.h"
#include "PointOutput.h"
#include "RTTR_AssertError.h"
#include "TerrainRenderer.h"
#include "buildings/nobBaseWarehouse.h"
#include "buildings/nobUsual.h"
#include "factories/BuildingFactory.h"
#include "figures/nofFarmhand.h"
#include "helperFuncs.h"
#include "ingameWindows/iwHelp.h"
#include "nodeObjs/noEnvObject.h"
#include "nodeObjs/noGrainfield.h"
#include "gameTypes/Direction_Output.h"
#include "gameData/MapConsts.h"
#include "gameData/ShieldConsts.h"
#include "test/CreateEmptyWorld.h"
#include "test/WorldFixture.h"
#include "test/initTestHelpers.h"
#include <boost/array.hpp>
#include <boost/foreach.hpp>
#include <boost/test/unit_test.hpp>
#include <algorithm>

// This suite tests bugs that got fixed to avoid regressions
// So:
//      - Before you fix a bug, reproduce it here (or in a fitting suite) -> Test fails
//      - Fix the bug -> Test succeeds
BOOST_AUTO_TEST_SUITE(RegressionsSuite)

BOOST_AUTO_TEST_CASE(ProdOfComponents)
{
    Point<uint16_t> pt(256, 256);
    BOOST_REQUIRE_EQUAL(prodOfComponents(pt), 256u * 256u);
    Point<int16_t> ptI(256, 256);
    BOOST_REQUIRE_EQUAL(prodOfComponents(ptI), 256 * 256);
    Point<float> ptF(256.5, 256.5);
    BOOST_REQUIRE_EQUAL(prodOfComponents(ptF), 256.5f * 256.5f);
}

BOOST_AUTO_TEST_CASE(DirectionCmp)
{
    Direction east(Direction::EAST); //-V525
    Direction east2(Direction::EAST);
    Direction west(Direction::WEST);
    // All variations: Dir-Dir, Dir-Type, Type-Dir
    BOOST_REQUIRE_EQUAL(east, east2);
    BOOST_REQUIRE_EQUAL(east, Direction::EAST);
    BOOST_REQUIRE_EQUAL(Direction::EAST, east2);
    BOOST_REQUIRE_NE(east, west);
    BOOST_REQUIRE_NE(east, Direction::WEST);
    BOOST_REQUIRE_NE(Direction::WEST, east2);
}

BOOST_AUTO_TEST_CASE(DirectionIncDec)
{
    // For every direction
    for(unsigned startDir = 0; startDir < Direction::COUNT; startDir++)
    {
        // Fit back to range
        BOOST_REQUIRE_EQUAL(Direction(startDir + Direction::COUNT).toUInt(), startDir);
        // Increment
        Direction testDir(startDir);
        BOOST_REQUIRE_EQUAL(testDir++, Direction(startDir));
        BOOST_REQUIRE_EQUAL(testDir, Direction(startDir + 1));
        BOOST_REQUIRE_EQUAL(++testDir, Direction(startDir + 2));
        BOOST_REQUIRE_EQUAL(testDir, Direction(startDir + 2));
        // Decrement
        BOOST_REQUIRE_EQUAL(testDir--, Direction(startDir + 2));
        BOOST_REQUIRE_EQUAL(testDir, Direction(startDir + 1));
        BOOST_REQUIRE_EQUAL(--testDir, Direction(startDir));
        BOOST_REQUIRE_EQUAL(testDir, Direction(startDir));
        // Add/Subtract. Test using the already tested primitives
        for(unsigned diff = 1; diff < 20; diff++)
        {
            Direction resultDir = testDir + diff;
            Direction expectedDir(testDir);
            for(unsigned i = 0; i < diff; i++)
                ++expectedDir;
            BOOST_REQUIRE_EQUAL(resultDir, expectedDir);
            Direction resultDir2 = testDir;
            BOOST_REQUIRE_EQUAL(resultDir2 += diff, expectedDir);
            BOOST_REQUIRE_EQUAL(resultDir2, expectedDir);
            resultDir = testDir - diff;
            expectedDir = testDir;
            for(unsigned i = 0; i < diff; i++)
                --expectedDir;
            BOOST_REQUIRE_EQUAL(resultDir, expectedDir);
            resultDir2 = testDir;
            BOOST_REQUIRE_EQUAL(resultDir2 -= diff, expectedDir);
            BOOST_REQUIRE_EQUAL(resultDir2, expectedDir);
        }
    }
}

BOOST_AUTO_TEST_CASE(IngameWnd)
{
    initGUITests();
    iwHelp wnd(CGI_HELP, "Foo barFoo barFoo barFoo bar\n\n\n\nFoo\nFoo\nFoo\nFoo\nFoo\nFoo\nFoo\nFoo\nFoo\nFoo\nFoo\nFoo\n");
    const Extent oldSize = wnd.GetSize();
    BOOST_REQUIRE_GT(oldSize.x, 50u);
    BOOST_REQUIRE_GT(oldSize.y, 50u);
    // Window should reduce height (only)
    wnd.SetMinimized(true);
    BOOST_REQUIRE_EQUAL(wnd.GetSize().x, oldSize.x); //-V807
    BOOST_REQUIRE_GT(wnd.GetSize().y, 0u);
    BOOST_REQUIRE_LT(wnd.GetSize().y, oldSize.y);
    // And fully expand to old size
    wnd.SetMinimized(false);
    BOOST_REQUIRE_EQUAL(wnd.GetSize(), oldSize);
}

struct AddGoodsFixture : public WorldFixture<CreateEmptyWorld, 1>, public LogAccessor
{
    boost::array<unsigned, NUM_JOB_TYPES> numPeople, numPeoplePlayer;
    boost::array<unsigned, NUM_WARE_TYPES> numGoods, numGoodsPlayer;
    AddGoodsFixture()
    {
        GamePlayer& player = world.GetPlayer(0);
        // Don't keep any reserve
        for(unsigned i = 0; i <= this->ggs.GetMaxMilitaryRank(); ++i)
            player.GetFirstWH()->SetRealReserve(i, 0); //-V522
        numPeople = numPeoplePlayer = player.GetInventory().people;
        numGoods = numGoodsPlayer = player.GetInventory().goods;
    }

    /// Asserts that the expected and actual good count match for the HQ
    void testNumGoodsHQ()
    {
        nobBaseWarehouse& hq = *world.GetSpecObj<nobBaseWarehouse>(world.GetPlayer(0).GetHQPos());
        for(unsigned i = 0; i < NUM_JOB_TYPES; i++)
        {
            BOOST_REQUIRE_EQUAL(hq.GetNumVisualFigures(Job(i)), numPeople[i]);
            BOOST_REQUIRE_EQUAL(hq.GetNumRealFigures(Job(i)), numPeople[i]);
        }
        for(unsigned i = 0; i < NUM_WARE_TYPES; i++)
        {
            BOOST_REQUIRE_EQUAL(hq.GetNumVisualWares(GoodType(i)), numGoods[i]);
            BOOST_REQUIRE_EQUAL(hq.GetNumRealWares(GoodType(i)), numGoods[i]);
        }
    }
    /// Asserts that the expected and actual good count match for the player
    void testNumGoodsPlayer()
    {
        GamePlayer& player = world.GetPlayer(0);
        for(unsigned i = 0; i < NUM_JOB_TYPES; i++)
            BOOST_REQUIRE_EQUAL(player.GetInventory().people[i], numPeoplePlayer[i]);
        for(unsigned i = 0; i < NUM_WARE_TYPES; i++)
            BOOST_REQUIRE_EQUAL(player.GetInventory().goods[i], numGoodsPlayer[i]);
    }
};

BOOST_FIXTURE_TEST_CASE(AddGoods, AddGoodsFixture)
{
    GamePlayer& player = world.GetPlayer(0);
    nobBaseWarehouse& hq = *world.GetSpecObj<nobBaseWarehouse>(player.GetHQPos());
    testNumGoodsHQ();

    // Add nothing -> nothing changed
    Inventory newGoods;
    hq.AddGoods(newGoods, true);
    testNumGoodsHQ();
    testNumGoodsPlayer();

    // Add jobs
    for(unsigned i = 0; i < NUM_JOB_TYPES; i++)
    {
        // Boat carrier gets divided upfront
        if(Job(i) == JOB_BOATCARRIER)
            continue;
        newGoods.Add(Job(i), i + 1);
        numPeople[i] += i + 1;
    }
    numPeoplePlayer = numPeople;
    hq.AddGoods(newGoods, true);
    testNumGoodsHQ();
    testNumGoodsPlayer();

    // Add only to hq but not to player
    for(unsigned i = 0; i < NUM_JOB_TYPES; i++)
        numPeople[i] += newGoods.people[i];
    hq.AddGoods(newGoods, false);
    testNumGoodsHQ();
    testNumGoodsPlayer();

    // Add wares
    newGoods.clear();
    for(unsigned i = 0; i < NUM_WARE_TYPES; i++)
    {
        // Only romand shields get added
        if(ConvertShields(GoodType(i)) == GD_SHIELDROMANS && GoodType(i) != GD_SHIELDROMANS)
            continue;
        newGoods.Add(GoodType(i), i + 2);
        numGoods[i] += i + 2;
    }
    numGoodsPlayer = numGoods;
    hq.AddGoods(newGoods, true);
    testNumGoodsHQ();
    testNumGoodsPlayer();

    // Add only to hq but not to player
    for(unsigned i = 0; i < NUM_WARE_TYPES; i++)
        numGoods[i] += newGoods.goods[i];
    hq.AddGoods(newGoods, false);
    testNumGoodsHQ();
    testNumGoodsPlayer();

#if RTTR_ENABLE_ASSERTS
    RTTR_AssertEnableBreak = false;
    newGoods.clear();
    newGoods.Add(JOB_BOATCARRIER);
    RTTR_REQUIRE_ASSERT(hq.AddGoods(newGoods, false));
    newGoods.clear();
    newGoods.Add(GD_SHIELDAFRICANS);
    RTTR_REQUIRE_ASSERT(hq.AddGoods(newGoods, false));
    RTTR_AssertEnableBreak = true;
#endif
}

struct FarmerFixture : public WorldFixture<CreateEmptyWorld, 1>
{
    MapPoint farmPt;
    nobUsual* farm;
    const nofFarmhand* farmer;
    FarmerFixture()
    {
        farmPt = world.GetPlayer(0).GetHQPos() + MapPoint(5, 0);
        farm = dynamic_cast<nobUsual*>(BuildingFactory::CreateBuilding(world, BLD_FARM, farmPt, 0, NAT_ROMANS));
        BOOST_REQUIRE(farm);
        world.BuildRoad(0, false, world.GetNeighbour(farmPt, Direction::SOUTHEAST), std::vector<Direction>(5, Direction::WEST));
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
            world.SetBuildingSite(BLD_WOODCUTTER, newField, 0);
        else
            world.SetBuildingSite(BLD_WOODCUTTER, world.GetNeighbour(newField, Direction::SOUTHEAST), 0);
        // Let farmer return
        RTTR_EXEC_TILL(150, !farm->is_working);
        // Aborted work
        BOOST_REQUIRE(!world.GetSpecObj<noGrainfield>(newField));
        // And remove
        if(i == 0)
            world.DestroyFlag(world.GetNeighbour(newField, Direction::SOUTHEAST), 0);
        else
            world.DestroyBuilding(world.GetNeighbour(newField, Direction::SOUTHEAST), 0);
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

typedef WorldFixture<CreateEmptyWorld, 0> WorldFixtureEmpty0P;
boost::test_tools::predicate_result boundaryStonesMatch(GameWorldGame& world, const std::vector<BoundaryStones>& expected)
{
    world.RecalcBorderStones(Position(0, 0), Extent(world.GetSize()));
    RTTR_FOREACH_PT(MapPoint, world.GetSize())
    {
        const BoundaryStones& isValue = world.GetNode(pt).boundary_stones;
        const BoundaryStones& expectedValue = expected[world.GetIdx(pt)];
        for(unsigned i = 0; i < BoundaryStones::size(); i++)
        {
            if(isValue[i] != expectedValue[i])
            {
                boost::test_tools::predicate_result result(false);
                result.message() << isValue[i] << "!=" << expectedValue[i] << " at " << pt << "[" << i << "]";
                return result;
            }
        }
    }
    return true;
}

BOOST_FIXTURE_TEST_CASE(BorderStones, WorldFixtureEmpty0P)
{
    std::vector<MapPoint> ptsToTest;
    // Check some point in the middle and at 0,0 which causes wrapping
    ptsToTest.push_back(MapPoint(5, 5));
    ptsToTest.push_back(MapPoint(0, 0));
    BOOST_FOREACH(MapPoint middlePt, ptsToTest)
    {
        std::vector<BoundaryStones> expectedBoundaryStones(world.GetWidth() * world.GetHeight());
        // Reset owner to 0 (None) and boundary stones to nothing
        RTTR_FOREACH_PT(MapPoint, world.GetSize())
        {
            world.SetOwner(pt, 0);
            std::fill(expectedBoundaryStones[world.GetIdx(pt)].begin(), expectedBoundaryStones[world.GetIdx(pt)].end(), 0u);
        }
        // Get the minimum possible region where border stones would be placed
        const std::vector<MapPoint> radius1Pts = world.GetPointsInRadius(middlePt, 1);
        // Set only the middle pt and recalc
        world.SetOwner(middlePt, 1);
        // Only middle pt has a single boundary stone
        expectedBoundaryStones[world.GetIdx(middlePt)][0] = 1u;
        BOOST_REQUIRE(boundaryStonesMatch(world, expectedBoundaryStones));

        BOOST_FOREACH(MapPoint pt, radius1Pts)
            world.SetOwner(pt, 1);
        // Midle pt lost its stone
        expectedBoundaryStones[world.GetIdx(middlePt)][0] = 0u;
        // Each border node should have a boundary stone at the center
        BOOST_FOREACH(MapPoint pt, radius1Pts)
            expectedBoundaryStones[world.GetIdx(pt)][0] = 1u;
        expectedBoundaryStones[world.GetIdx(radius1Pts[Direction::WEST])][2] = 1u;
        expectedBoundaryStones[world.GetIdx(radius1Pts[Direction::NORTHWEST])][1] = 1u;
        expectedBoundaryStones[world.GetIdx(radius1Pts[Direction::NORTHWEST])][3] = 1u;
        expectedBoundaryStones[world.GetIdx(radius1Pts[Direction::NORTHEAST])][2] = 1u;
        expectedBoundaryStones[world.GetIdx(radius1Pts[Direction::EAST])][3] = 1u;
        // SE has no other stone
        expectedBoundaryStones[world.GetIdx(radius1Pts[Direction::SOUTHWEST])][1] = 1u;
        BOOST_REQUIRE(boundaryStonesMatch(world, expectedBoundaryStones));

        // Now obtain another node:
        const MapPoint doubleWestPt = world.GetNeighbour(radius1Pts[Direction::WEST], Direction::WEST);
        world.SetOwner(doubleWestPt, 1);
        // Still the same, but that node has 2 stones
        expectedBoundaryStones[world.GetIdx(doubleWestPt)][0] = 1u;
        expectedBoundaryStones[world.GetIdx(doubleWestPt)][1] = 1u;
        BOOST_REQUIRE(boundaryStonesMatch(world, expectedBoundaryStones));

        // Next, actually increasing our territory
        // Note: The NW node and its WEST neighbour have (temporarly) 3 neigbouring stones leading to issue #538 where
        // the half-way stone to E gets removed with prevent-blocking enabled
        const MapPoint doubleWestPt2 = world.GetNeighbour(doubleWestPt, Direction::NORTHEAST);
        const MapPoint doubleWestPt3 = world.GetNeighbour(doubleWestPt, Direction::SOUTHEAST);
        world.SetOwner(doubleWestPt2, 1);
        world.SetOwner(doubleWestPt3, 1);
        // New points get some stones: Top
        expectedBoundaryStones[world.GetIdx(doubleWestPt2)][0] = 1u;
        expectedBoundaryStones[world.GetIdx(doubleWestPt2)][1] = 1u;
        expectedBoundaryStones[world.GetIdx(doubleWestPt2)][3] = 1u;
        // Bottom
        expectedBoundaryStones[world.GetIdx(doubleWestPt3)][0] = 1u;
        expectedBoundaryStones[world.GetIdx(doubleWestPt3)][1] = 1u;
        // Middle (gets one to bottom, looses one to right, as WEST point is now no border node anymore (affects 2 more)
        expectedBoundaryStones[world.GetIdx(doubleWestPt)][2] = 1u;
        expectedBoundaryStones[world.GetIdx(doubleWestPt)][1] = 0u;
        expectedBoundaryStones[world.GetIdx(radius1Pts[Direction::WEST])][0] = 0u;
        expectedBoundaryStones[world.GetIdx(radius1Pts[Direction::WEST])][2] = 0u;
        expectedBoundaryStones[world.GetIdx(radius1Pts[Direction::NORTHWEST])][3] = 0u;
        BOOST_REQUIRE(boundaryStonesMatch(world, expectedBoundaryStones));
    }
}

BOOST_AUTO_TEST_CASE(TR_ConvertCoords)
{
    TerrainRenderer tr;
    const int w = 23;
    const int h = 32;
    tr.Init(MapExtent(w, h));

    Position offset;
    // Test border cases
    BOOST_REQUIRE_EQUAL(tr.ConvertCoords(Position(0, 0), &offset), MapPoint(0, 0));
    BOOST_REQUIRE_EQUAL(offset, Position(0, 0));
    BOOST_REQUIRE_EQUAL(tr.ConvertCoords(Position(w - 1, h - 1), &offset), MapPoint(w - 1, h - 1));
    BOOST_REQUIRE_EQUAL(offset, Position(0, 0));
    BOOST_REQUIRE_EQUAL(tr.ConvertCoords(Position(w, h - 1), &offset), MapPoint(0, h - 1));
    BOOST_REQUIRE_EQUAL(offset, Position(w * TR_W, 0));
    BOOST_REQUIRE_EQUAL(tr.ConvertCoords(Position(w - 1, h), &offset), MapPoint(w - 1, 0));
    BOOST_REQUIRE_EQUAL(offset, Position(0, h * TR_H));
    BOOST_REQUIRE_EQUAL(tr.ConvertCoords(Position(w, h), &offset), MapPoint(0, 0));
    BOOST_REQUIRE_EQUAL(offset, Position(w * TR_W, h * TR_H));
    BOOST_REQUIRE_EQUAL(tr.ConvertCoords(Position(w + w / 2, h + h / 2), &offset), MapPoint(w / 2, h / 2));
    BOOST_REQUIRE_EQUAL(offset, Position(w * TR_W, h * TR_H));
    // Big value
    BOOST_REQUIRE_EQUAL(tr.ConvertCoords(Position(10 * w + w / 2, 11 * h + h / 2), &offset), MapPoint(w / 2, h / 2));
    BOOST_REQUIRE_EQUAL(offset, Position(10 * w * TR_W, 11 * h * TR_H));

    // Negative cases
    BOOST_REQUIRE_EQUAL(tr.ConvertCoords(Position(-1, -1), &offset), MapPoint(w - 1, h - 1));
    BOOST_REQUIRE_EQUAL(offset, Position(-w * TR_W, -h * TR_H));
    BOOST_REQUIRE_EQUAL(tr.ConvertCoords(Position(-w + 1, -h + 2), &offset), MapPoint(1, 2));
    BOOST_REQUIRE_EQUAL(offset, Position(-w * TR_W, -h * TR_H));
    BOOST_REQUIRE_EQUAL(tr.ConvertCoords(Position(-w, -h), &offset), MapPoint(0, 0));
    BOOST_REQUIRE_EQUAL(offset, Position(-w * TR_W, -h * TR_H));
    BOOST_REQUIRE_EQUAL(tr.ConvertCoords(Position(-w - 1, -h - 2), &offset), MapPoint(w - 1, h - 2));
    BOOST_REQUIRE_EQUAL(offset, Position(-2 * w * TR_W, -2 * h * TR_H));
    // Big value
    BOOST_REQUIRE_EQUAL(tr.ConvertCoords(Position(-10 * w + w / 2, -11 * h + h / 2), &offset), MapPoint(w / 2, h / 2));
    BOOST_REQUIRE_EQUAL(offset, Position(-10 * w * TR_W, -11 * h * TR_H));
}

BOOST_AUTO_TEST_SUITE_END()
