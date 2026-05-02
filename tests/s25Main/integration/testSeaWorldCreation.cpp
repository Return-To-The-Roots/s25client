// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "RTTR_AssertError.h"
#include "RttrForeachPt.h"
#include "helpers/IdRange.h"
#include "addons/const_addons.h"
#include "worldFixtures/CreateSeaWorld.h"
#include "worldFixtures/SeaWorldWithGCExecution.h"
#include "worldFixtures/WorldFixture.h"
#include "worldFixtures/terrainHelpers.h"
#include "gameTypes/GameTypesOutput.h"
#include "gameTypes/ShipDirection.h"
#include "lua/GameDataLoader.h"
#include "world/MapLoader.h"
#include <rttr/test/LogAccessor.hpp>
#include <boost/test/unit_test.hpp>

// LCOV_EXCL_START
static std::ostream& operator<<(std::ostream& out, const ShipDirection& dir)
{
    return out << static_cast<int>(rttr::enum_cast(dir));
}
// LCOV_EXCL_STOP

BOOST_AUTO_TEST_SUITE(SeaWorldCreationSuite)

namespace {

/// Return the ship dir from a point to an other point given by their difference
ShipDirection getShipDir(const MapBase& world, MapPoint fromPt, const Position& diff)
{
    MapPoint toPt = world.MakeMapPoint(Position(fromPt) + diff);
    return world.GetShipDir(fromPt, toPt);
}

/// Test getting the ship dir for the various cases coming from a single point
void testShipDir(const MapBase& world, const MapPoint fromPt)
{
    using DiffPt = Position;
    // General cases
    BOOST_TEST_REQUIRE(getShipDir(world, fromPt, DiffPt(0, -10)) == ShipDirection::North);
    BOOST_TEST_REQUIRE(getShipDir(world, fromPt, DiffPt(10, -1)) == ShipDirection::NorthEast);
    BOOST_TEST_REQUIRE(getShipDir(world, fromPt, DiffPt(10, 1)) == ShipDirection::SouthEast);
    BOOST_TEST_REQUIRE(getShipDir(world, fromPt, DiffPt(0, 10)) == ShipDirection::South);
    BOOST_TEST_REQUIRE(getShipDir(world, fromPt, DiffPt(-10, 1)) == ShipDirection::SouthWest);
    BOOST_TEST_REQUIRE(getShipDir(world, fromPt, DiffPt(-10, -1)) == ShipDirection::NorthWest);

    // y diff is zero -> Go south (convention)
    BOOST_TEST_REQUIRE(getShipDir(world, fromPt, DiffPt(-10, 0)) == ShipDirection::SouthWest);
    BOOST_TEST_REQUIRE(getShipDir(world, fromPt, DiffPt(10, 0)) == ShipDirection::SouthEast);
    BOOST_TEST_REQUIRE(getShipDir(world, fromPt, DiffPt(1, 0)) == ShipDirection::SouthEast);
    BOOST_TEST_REQUIRE(getShipDir(world, fromPt, DiffPt(-1, 0)) == ShipDirection::SouthWest);

    // 6 directions -> 60deg covered per direction, mainDir +- 30deg
    // Switch pt between north and south is simple: Above or below zero diff (already tested above)
    // But S to SE or SW (same for N) is harder. Dividing line as an angle of +- 60deg compared to x-axis
    // hence: |dy/dx| > tan(60deg) -> South, tan(60deg) ~= 1.732. Test here with |dy| = |dx| * 1.732 as the divider
    BOOST_TEST_REQUIRE(getShipDir(world, fromPt, DiffPt(100, 173)) == ShipDirection::SouthEast);
    // Switch point
    BOOST_TEST_REQUIRE(getShipDir(world, fromPt, DiffPt(100, 174)) == ShipDirection::South);
    // Same for other 3 switch points
    BOOST_TEST_REQUIRE(getShipDir(world, fromPt, DiffPt(-100, 173)) == ShipDirection::SouthWest);
    BOOST_TEST_REQUIRE(getShipDir(world, fromPt, DiffPt(-100, 174)) == ShipDirection::South);
    BOOST_TEST_REQUIRE(getShipDir(world, fromPt, DiffPt(-100, -173)) == ShipDirection::NorthWest);
    BOOST_TEST_REQUIRE(getShipDir(world, fromPt, DiffPt(-100, -174)) == ShipDirection::North);
    BOOST_TEST_REQUIRE(getShipDir(world, fromPt, DiffPt(100, -173)) == ShipDirection::NorthEast);
    BOOST_TEST_REQUIRE(getShipDir(world, fromPt, DiffPt(100, -174)) == ShipDirection::North);
}

void createMarkerlessIslandWorld(GameWorld& world)
{
    world.Unload();
    loadGameData(world.GetDescriptionWriteable());
    world.Init(MapExtent(30, 30));

    const auto water = GetWaterTerrain(world.GetDescription());
    RTTR_FOREACH_PT(MapPoint, world.GetSize())
    {
        MapNode& node = world.GetNodeWriteable(pt);
        node.t1 = node.t2 = water;
    }

    const auto land = GetLandTerrain(world.GetDescription(), ETerrain::Buildable);
    for(MapPoint pt(8, 8); pt.y < 22; ++pt.y)
    {
        for(pt.x = 8; pt.x < 22; ++pt.x)
        {
            MapNode& node = world.GetNodeWriteable(pt);
            node.t1 = node.t2 = land;
        }
    }
}

unsigned countHarborBQ(const GameWorld& world)
{
    unsigned result = 0;
    RTTR_FOREACH_PT(MapPoint, world.GetSize())
    {
        if(world.GetNode(pt).bq == BuildingQuality::Harbor)
            ++result;
    }
    return result;
}

void testHarborPoint(const GameWorld& world, const HarborId harborId)
{
    const MapPoint harborPt = world.GetHarborPoint(harborId);
    BOOST_TEST_REQUIRE(harborPt.isValid());
    BOOST_TEST_REQUIRE(world.GetHarborPointID(harborPt) == harborId);

    bool hasSea = false;
    for(const auto dir : helpers::EnumRange<Direction>{})
    {
        const SeaId seaId = world.GetSeaId(harborId, dir);
        if(!seaId)
            continue;

        hasSea = true;
        const MapPoint coastalPt = world.GetCoastalPoint(harborId, seaId);
        BOOST_TEST_REQUIRE(coastalPt.isValid());
        BOOST_TEST_REQUIRE(world.GetSeaFromCoastalPoint(coastalPt) == seaId);
    }
    BOOST_TEST_REQUIRE(hasSea);
    BOOST_TEST_REQUIRE(world.GetNode(harborPt).bq == BuildingQuality::Harbor);
}

using SeaWorldFixture = WorldFixture<CreateSeaWorld, 3, SeaWorldDefault::width, SeaWorldDefault::height>;

struct MarkerlessIslandFixture : WorldFixtureBase
{
    MarkerlessIslandFixture() : WorldFixtureBase(3) { createMarkerlessIslandWorld(world); }
};
} // namespace

BOOST_AUTO_TEST_CASE(GetShipDir)
{
    // Width must be > 100*2 and Height > 174 * 2 to avoid wrapping errors (see testShipDir)
    MapBase world;
    world.Resize(MapExtent(202, 350));
    // Basic case
    testShipDir(world, MapPoint(world.GetWidth() / 2, world.GetHeight() / 2));
    // Left/Right border
    testShipDir(world, MapPoint(0, world.GetHeight() / 2));
    testShipDir(world, MapPoint(world.GetWidth() - 1, world.GetHeight() / 2));
    // Top/Bottom border
    testShipDir(world, MapPoint(world.GetWidth() / 2, 0));
    testShipDir(world, MapPoint(world.GetWidth() / 2, world.GetHeight() - 1));
    // Diagonal ends
    testShipDir(world, MapPoint(0, 0));
    testShipDir(world, MapPoint(world.GetWidth() - 1, world.GetHeight() - 1));
}

BOOST_FIXTURE_TEST_CASE(HarborSpotCreation, SeaWorldWithGCExecution<>)
{
    rttr::test::LogAccessor logAcc;
    // Point 0,0 is definitely inside the sea
    BOOST_TEST_REQUIRE(world.IsWaterPoint(MapPoint(0, 0)));
    BOOST_TEST_REQUIRE(world.IsSeaPoint(MapPoint(0, 0)));
    // 2 harbors for each of the 4 player spots
    BOOST_TEST_REQUIRE(world.GetNumHarborPoints() == 8u);
    // 2 seas: 1 outside, 1 inside
    BOOST_TEST_REQUIRE(world.GetNumSeas() == 2u);
// Harbor ID 0 is means invalid harbor
#if RTTR_ENABLE_ASSERTS
    RTTR_REQUIRE_ASSERT(world.GetHarborPoint(HarborId::invalidValue()));
#endif
    for(const auto curHarborId : helpers::idRange<HarborId>(world.GetNumHarborPoints()))
    {
        const MapPoint curHarborPt = world.GetHarborPoint(curHarborId);
        BOOST_TEST_REQUIRE(curHarborPt.isValid());
        // Reverse mapping must match
        BOOST_TEST_REQUIRE(world.GetHarborPointID(curHarborPt) == curHarborId);
        // We must be at one of the 2 seas
        SeaId seaId(1);
        if(!world.IsHarborAtSea(curHarborId, seaId))
        {
            seaId = SeaId(2);
            BOOST_TEST_REQUIRE(world.IsHarborAtSea(curHarborId, seaId));
        }
        // We must have a coast point at that sea
        const MapPoint coastPt = world.GetCoastalPoint(curHarborId, seaId);
        BOOST_TEST_REQUIRE(coastPt.isValid());
        BOOST_TEST_REQUIRE(world.GetSeaFromCoastalPoint(coastPt).isValid());
        // Sea in the direction of the coast must match
        bool coastPtFound = false;
        for(const auto dir : helpers::EnumRange<Direction>{})
        {
            if(world.GetNeighbour(curHarborPt, dir) == coastPt)
            {
                BOOST_TEST_REQUIRE(world.GetSeaId(curHarborId, dir) == seaId);
                coastPtFound = true;
                break;
            }
        }
        BOOST_TEST_REQUIRE(coastPtFound);
        BOOST_TEST_REQUIRE(world.GetNode(curHarborPt).bq == BuildingQuality::Harbor);
    }
}

BOOST_FIXTURE_TEST_CASE(FreeHarborSpotsAddonAddsCoastalHarbors, SeaWorldFixture)
{
    const unsigned initialHarbors = world.GetNumHarborPoints();

    ggs.setSelection(AddonId::FREE_HARBOR_SPOTS, 1);
    BOOST_TEST_REQUIRE(MapLoader::InitSeasAndHarbors(world, std::vector<MapPoint>(), true));
    world.InitAfterLoad();

    BOOST_TEST_REQUIRE(world.GetNumHarborPoints() > initialHarbors);
    for(unsigned harborIdx = initialHarbors + 1; harborIdx <= world.GetNumHarborPoints(); ++harborIdx)
    {
        testHarborPoint(world, HarborId(harborIdx));
    }
}

BOOST_FIXTURE_TEST_CASE(FreeHarborSpotsAddonWorksWithoutMapMarkers, MarkerlessIslandFixture)
{
    BOOST_TEST_REQUIRE(MapLoader::InitSeasAndHarbors(world));
    world.InitAfterLoad();
    BOOST_TEST_REQUIRE(world.GetNumHarborPoints() == 0u);
    BOOST_TEST_REQUIRE(countHarborBQ(world) == 0u);

    ggs.setSelection(AddonId::FREE_HARBOR_SPOTS, 1);
    BOOST_TEST_REQUIRE(MapLoader::InitSeasAndHarbors(world));
    world.InitAfterLoad();
    BOOST_TEST_REQUIRE(world.GetNumHarborPoints() == 0u);
    BOOST_TEST_REQUIRE(countHarborBQ(world) == 0u);

    BOOST_TEST_REQUIRE(MapLoader::InitSeasAndHarbors(world, std::vector<MapPoint>(), true));
    world.InitAfterLoad();

    BOOST_TEST_REQUIRE(world.GetNumHarborPoints() > 0u);
    for(const auto harborId : helpers::idRange<HarborId>(world.GetNumHarborPoints()))
    {
        testHarborPoint(world, harborId);
    }
}

BOOST_FIXTURE_TEST_CASE(HarborNeighbors, SeaWorldWithGCExecution<>)
{
    // Now just test some assumptions: 2 harbor spots per possible HQ.
    // Square land, 1 HQ on each side, harbors top and bottom or left and right of it
    // 1) Compare those around each HQ
    BOOST_TEST_REQUIRE(world.GetHarborPoint(HarborId(1)).y < world.GetHarborPoint(HarborId(2)).y); //-V807
    BOOST_TEST_REQUIRE(world.GetHarborPoint(HarborId(7)).y < world.GetHarborPoint(HarborId(8)).y);
    BOOST_TEST_REQUIRE(world.GetHarborPoint(HarborId(3)).x < world.GetHarborPoint(HarborId(4)).x);
    BOOST_TEST_REQUIRE(world.GetHarborPoint(HarborId(5)).x < world.GetHarborPoint(HarborId(6)).x); //-V807
    // 2) Compare between them
    BOOST_TEST_REQUIRE(world.GetHarborPoint(HarborId(2)).y < world.GetHarborPoint(HarborId(7)).y);
    BOOST_TEST_REQUIRE(world.GetHarborPoint(HarborId(4)).x < world.GetHarborPoint(HarborId(5)).x);
    BOOST_TEST_REQUIRE(world.GetHarborPoint(HarborId(3)).x < world.GetHarborPoint(HarborId(1)).x);
    BOOST_TEST_REQUIRE(world.GetHarborPoint(HarborId(1)).x < world.GetHarborPoint(HarborId(5)).x);

    // Test neighbors
    std::vector<HarborPos::Neighbor> nb;
    // Hb 1 (outside)
    HarborId hbId(1);
    nb = world.GetHarborNeighbors(hbId, ShipDirection::SouthWest);
    BOOST_TEST_REQUIRE(nb.size() == 1u);
    BOOST_TEST(nb[0].id == HarborId(3));
    nb = world.GetHarborNeighbors(hbId, ShipDirection::SouthEast);
    BOOST_TEST_REQUIRE(nb.size() == 1u);
    BOOST_TEST(nb[0].id == HarborId(6));
    nb = world.GetHarborNeighbors(hbId, ShipDirection::North);
    BOOST_TEST_REQUIRE(nb.size() == 1u);
    BOOST_TEST(nb[0].id == HarborId(8));
    BOOST_TEST_REQUIRE(world.GetHarborNeighbors(hbId, ShipDirection::South).size() == 0u);
    BOOST_TEST_REQUIRE(world.GetHarborNeighbors(hbId, ShipDirection::NorthEast).size() == 0u);
    BOOST_TEST_REQUIRE(world.GetHarborNeighbors(hbId, ShipDirection::NorthWest).size() == 0u);

    // Hb 7 (inside)
    hbId = HarborId(7);
    nb = world.GetHarborNeighbors(hbId, ShipDirection::NorthWest);
    BOOST_TEST_REQUIRE(nb.size() == 1u);
    BOOST_TEST(nb[0].id == HarborId(4));
    nb = world.GetHarborNeighbors(hbId, ShipDirection::NorthEast);
    BOOST_TEST_REQUIRE(nb.size() == 1u);
    BOOST_TEST(nb[0].id == HarborId(5));
    nb = world.GetHarborNeighbors(hbId, ShipDirection::North);
    BOOST_TEST_REQUIRE(nb.size() == 1u);
    BOOST_TEST(nb[0].id == HarborId(2));
    BOOST_TEST_REQUIRE(world.GetHarborNeighbors(hbId, ShipDirection::South).size() == 0u);
    BOOST_TEST_REQUIRE(world.GetHarborNeighbors(hbId, ShipDirection::SouthEast).size() == 0u);
    BOOST_TEST_REQUIRE(world.GetHarborNeighbors(hbId, ShipDirection::SouthWest).size() == 0u);
}

BOOST_AUTO_TEST_SUITE_END()
