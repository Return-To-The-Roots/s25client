// Copyright (c) 2016 - 2020 Settlers Freaks (sf-team at siedler25.org)
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

#include "FileChecksum.h"
#include "GamePlayer.h"
#include "PointOutput.h"
#include "RttrConfig.h"
#include "RttrForeachPt.h"
#include "files.h"
#include "lua/GameDataLoader.h"
#include "ogl/glArchivItem_Map.h"
#include "worldFixtures/CreateEmptyWorld.h"
#include "worldFixtures/MockLocalGameState.h"
#include "worldFixtures/WorldFixture.h"
#include "world/MapLoader.h"
#include "nodeObjs/noBase.h"
#include "gameTypes/GameTypesOutput.h"
#include "libsiedler2/ArchivItem_Map_Header.h"
#include "s25util/tmpFile.h"
#include <boost/filesystem/path.hpp>
#include <boost/test/unit_test.hpp>
#include <vector>

struct MapTestFixture
{
    const boost::filesystem::path testMapPath;
    MapTestFixture() : testMapPath(RTTRCONFIG.ExpandPath(s25::folders::mapsRttr) / "Bergruft.swd") {}
};

BOOST_FIXTURE_TEST_SUITE(MapTestSuite, MapTestFixture)

BOOST_AUTO_TEST_CASE(LoadSaveMap)
{
    // Check that loading and saving a map does not alter it
    glArchivItem_Map map;
    bnw::ifstream mapFile(testMapPath, std::ios::binary);
    BOOST_TEST_REQUIRE(map.load(mapFile, false) == 0);
    TmpFile outMap(".swd");
    BOOST_TEST_REQUIRE(outMap.isValid());
    BOOST_TEST_REQUIRE(map.write(outMap.getStream()) == 0);
    mapFile.close();
    outMap.close();
    BOOST_TEST_REQUIRE(CalcChecksumOfFile(testMapPath) == CalcChecksumOfFile(outMap.filePath));
}

namespace {
struct UninitializedWorldCreator
{
    explicit UninitializedWorldCreator(const MapExtent&) {}
    bool operator()(GameWorldBase&) { return true; }
};

struct LoadWorldFromFileCreator : MapTestFixture
{
    std::vector<MapPoint> hqs;

    explicit LoadWorldFromFileCreator(MapExtent) {}
    bool operator()(GameWorldBase& world)
    {
        MapLoader loader(world);
        BOOST_TEST_REQUIRE(loader.Load(testMapPath));
        for(unsigned i = 0; i < world.GetNumPlayers(); i++)
            hqs.push_back(loader.GetHQPos(i));
        return true;
    }
};
struct LoadWorldAndS2MapCreator : MapTestFixture
{
    glArchivItem_Map map;

    explicit LoadWorldAndS2MapCreator(MapExtent) {}
    bool operator()(GameWorldBase& world)
    {
        bnw::ifstream mapFile(testMapPath, std::ios::binary);
        if(map.load(mapFile, false) != 0)
            throw std::runtime_error("Could not load file " + testMapPath.string()); // LCOV_EXCL_LINE
        MapLoader loader(world);
        if(!loader.Load(map, Exploration::FogOfWar))
            throw std::runtime_error("Could not load map"); // LCOV_EXCL_LINE
        return true;
    }
};

using WorldLoadedWithS2MapFixture = WorldFixture<LoadWorldAndS2MapCreator>;
using WorldLoaded1PFixture = WorldFixture<LoadWorldFromFileCreator, 1>;
using WorldFixtureEmpty1P = WorldFixture<CreateEmptyWorld, 1>;
} // namespace

BOOST_FIXTURE_TEST_CASE(LoadWorld, WorldFixture<UninitializedWorldCreator>)
{
    MapTestFixture fixture;
    glArchivItem_Map map;
    bnw::ifstream mapFile(fixture.testMapPath, std::ios::binary);
    BOOST_TEST_REQUIRE(map.load(mapFile, false) == 0);
    const libsiedler2::ArchivItem_Map_Header& header = map.getHeader();
    BOOST_TEST(header.getWidth() == 176);
    BOOST_TEST(header.getHeight() == 80);
    BOOST_TEST(header.getNumPlayers() == 4);

    MapLoader loader(world);
    BOOST_TEST_REQUIRE(loader.Load(fixture.testMapPath));
    BOOST_TEST(world.GetWidth() == map.getHeader().getWidth());
    BOOST_TEST(world.GetHeight() == map.getHeader().getHeight());
}

BOOST_FIXTURE_TEST_CASE(HeightLoading, WorldLoadedWithS2MapFixture)
{
    RTTR_FOREACH_PT(MapPoint, world.GetSize())
    {
        BOOST_TEST_REQUIRE(world.GetNode(pt).altitude == worldCreator.map.GetMapDataAt(MapLayer::Altitude, pt.x, pt.y));
    }
}

BOOST_FIXTURE_TEST_CASE(SameBQasInS2, WorldLoadedWithS2MapFixture)
{
    // Init BQ
    world.InitAfterLoad();
    RTTR_FOREACH_PT(MapPoint, world.GetSize())
    {
        BOOST_TEST_INFO("pt " << pt);
        const auto original = worldCreator.map.GetMapDataAt(MapLayer::BuildingQuality, pt.x, pt.y);
        BOOST_TEST_INFO(" original: " << original);
        auto s2BQ = BuildingQuality(original & 0x7);
        BuildingQuality bq = world.GetNode(pt).bq;
        BOOST_TEST(bq == s2BQ);
    }
}

BOOST_FIXTURE_TEST_CASE(HQPlacement, WorldLoaded1PFixture)
{
    GamePlayer& player = world.GetPlayer(0);
    BOOST_TEST_REQUIRE(player.isUsed());
    BOOST_TEST_REQUIRE(worldCreator.hqs[0].isValid());
    BOOST_TEST_REQUIRE(world.GetNO(worldCreator.hqs[0])->GetGOT() == GO_Type::NobHq);
}

BOOST_FIXTURE_TEST_CASE(CloseHarborSpots, WorldFixture<UninitializedWorldCreator>)
{
    loadGameData(world.GetDescriptionWriteable());
    DescIdx<TerrainDesc> tWater(0);
    for(; tWater.value < world.GetDescription().terrain.size(); tWater.value++)
    {
        if(world.GetDescription().get(tWater).kind == TerrainKind::Water
           && !world.GetDescription().get(tWater).Is(ETerrain::Walkable))
            break;
    }
    DescIdx<TerrainDesc> tLand(0);
    for(; tLand.value < world.GetDescription().terrain.size(); tLand.value++)
    {
        if(world.GetDescription().get(tLand).kind == TerrainKind::Land
           && world.GetDescription().get(tLand).Is(ETerrain::Walkable))
            break;
    }

    world.Init(MapExtent(30, 30));
    RTTR_FOREACH_PT(MapPoint, world.GetSize())
    {
        MapNode& node = world.GetNodeWriteable(pt);
        node.t1 = node.t2 = tWater;
    }

    // Place multiple harbor spots next to each other so their coastal points are on the same node
    std::vector<MapPoint> hbPos;
    hbPos.push_back(MapPoint(10, 10));
    hbPos.push_back(MapPoint(9, 10));
    hbPos.push_back(MapPoint(11, 10));

    hbPos.push_back(MapPoint(20, 10));
    hbPos.push_back(world.GetNeighbour(hbPos.back(), Direction::NorthWest));

    hbPos.push_back(MapPoint(10, 20)); //-V525
    hbPos.push_back(world.GetNeighbour(hbPos.back(), Direction::NorthEast));

    hbPos.push_back(MapPoint(0, 10));
    hbPos.push_back(world.GetNeighbour(hbPos.back(), Direction::SouthEast));

    hbPos.push_back(MapPoint(20, 10));
    hbPos.push_back(world.GetNeighbour(hbPos.back(), Direction::SouthWest));

    // Place land in radius 2
    for(const MapPoint& pt : hbPos)
    {
        for(const MapPoint& curPt : world.GetPointsInRadius(pt, 1))
        {
            for(const auto dir : helpers::EnumRange<Direction>{})
                setRightTerrain(world, curPt, dir, tLand);
        }
    }

    // And a node of water nearby so we do have a coast
    std::vector<MapPoint> waterPts;
    waterPts.push_back(world.GetNeighbour2(hbPos[0], 10));
    waterPts.push_back(world.GetNeighbour2(hbPos[0], 8));
    waterPts.push_back(world.GetNeighbour2(hbPos[3], 4));
    waterPts.push_back(world.GetNeighbour2(hbPos[5], 6));
    waterPts.push_back(world.GetNeighbour2(hbPos[7], 10));
    waterPts.push_back(world.GetNeighbour2(hbPos[9], 8));

    for(const MapPoint& pt : waterPts)
    {
        for(const auto dir : helpers::EnumRange<Direction>{})
            setRightTerrain(world, pt, dir, tWater);
    }

    // Check if this works
    BOOST_TEST_REQUIRE(MapLoader::InitSeasAndHarbors(world, hbPos));
    // All harbors valid
    BOOST_TEST_REQUIRE(world.GetNumHarborPoints() == hbPos.size());
    for(unsigned startHb = 1; startHb < world.GetNumHarborPoints(); startHb++)
    {
        for(const auto dir : helpers::EnumRange<Direction>{})
        {
            unsigned seaId = world.GetSeaId(startHb, dir);
            if(!seaId)
                continue;
            MapPoint startPt = world.GetCoastalPoint(startHb, seaId);
            BOOST_TEST_REQUIRE(startPt == world.GetNeighbour(world.GetHarborPoint(startHb), dir));
            for(unsigned targetHb = 1; targetHb < world.GetNumHarborPoints(); targetHb++)
            {
                MapPoint destPt = world.GetCoastalPoint(targetHb, seaId);
                BOOST_TEST_REQUIRE(destPt.isValid());
                std::vector<Direction> route;
                BOOST_TEST_REQUIRE((startPt == destPt || world.FindShipPath(startPt, destPt, 10000, &route, nullptr)));
                BOOST_TEST_REQUIRE(route.size() == world.CalcHarborDistance(startHb, targetHb));
            }
        }
    }
}

BOOST_FIXTURE_TEST_CASE(NONothingOnEmptyNode, WorldFixtureEmpty1P)
{
    const MapPoint hqPos = world.GetPlayer(0).GetHQPos();
    BOOST_TEST(world.GetNode(hqPos).obj != nullptr);
    BOOST_TEST_REQUIRE(world.GetNO(hqPos));
    BOOST_TEST(world.GetNO(hqPos)->GetGOT() == GO_Type::NobHq);
    BOOST_TEST(world.GetGOT(hqPos) == GO_Type::NobHq);

    const MapPoint emptySpot = world.GetNeighbour(hqPos, Direction::SouthWest);
    BOOST_TEST(world.GetNode(emptySpot).obj == nullptr);
    BOOST_TEST_REQUIRE(world.GetNO(emptySpot));
    BOOST_TEST(world.GetNO(emptySpot)->GetGOT() == GO_Type::Nothing);
    BOOST_TEST(world.GetGOT(emptySpot) == GO_Type::Nothing);
}

BOOST_FIXTURE_TEST_CASE(LoadLua, WorldFixture<UninitializedWorldCreator>)
{
    MapLoader loader(world);
    MockLocalGameState lgs;
    TmpFile invalidLuaFile(".lua");
    invalidLuaFile.getStream() << "-- No getRequiredLuaVersion\n";
    invalidLuaFile.close();
    BOOST_TEST_REQUIRE(!loader.LoadLuaScript(*game, lgs, invalidLuaFile.filePath));
    BOOST_TEST(!world.HasLua());

    TmpFile validLuaFile(".lua");
    validLuaFile.getStream() << "function getRequiredLuaVersion()\n return " << LuaInterfaceGameBase::GetVersion()
                             << "\n end";
    validLuaFile.close();

    BOOST_TEST_REQUIRE(loader.LoadLuaScript(*game, lgs, validLuaFile.filePath));
    BOOST_TEST(world.HasLua());
}

BOOST_AUTO_TEST_CASE(GetTerrainReturnsCorrectValues)
{
    using TerrainIdx = DescIdx<TerrainDesc>;
    TestWorld world(MapExtent(6, 4));
    const auto calcT1 = [&world](MapPoint pt) { return TerrainIdx(world.GetIdx(pt) * 2); };
    const auto calcT2 = [&world](MapPoint pt) { return TerrainIdx(world.GetIdx(pt) * 2 + 1); };
    RTTR_FOREACH_PT(MapPoint, world.GetSize())
    {
        auto& node = world.GetNodeInt(pt);
        node.t1 = calcT1(pt);
        node.t2 = calcT2(pt);
    }
    {
        const MapPoint testPt(1, 1);
        // t1 (idx) is the triangle directly below, t2 (idx+1) on right lower
        auto terrain = world.GetTerrain(testPt, Direction::SouthEast);
        BOOST_TEST(terrain.left == calcT2(testPt));
        BOOST_TEST(terrain.right == calcT1(testPt));

        terrain = world.GetTerrain(testPt, Direction::West);
        // right lower from previous point
        BOOST_TEST(terrain.left == calcT2(MapPoint(0, 1)));
        // below and right lower from upper point
        BOOST_TEST(terrain.right == calcT1(MapPoint(1, 0)));

        terrain = world.GetTerrain(testPt, Direction::NorthEast);
        BOOST_TEST(terrain.left == calcT2(MapPoint(1, 0)));
        // below of the point next to it
        BOOST_TEST(terrain.right == calcT1(MapPoint(2, 0)));
    }
    {
        const MapPoint testPt(5, 3); // Last point -> check borders
        auto terrain = world.GetTerrain(testPt, Direction::SouthEast);
        BOOST_TEST(terrain.left == calcT2(testPt));
        BOOST_TEST(terrain.right == calcT1(testPt));
        terrain = world.GetTerrain(testPt, Direction::West);
        BOOST_TEST(terrain.left == calcT2(MapPoint(4, 3)));
        BOOST_TEST(terrain.right == calcT1(MapPoint(5, 2)));
        terrain = world.GetTerrain(testPt, Direction::NorthEast);
        BOOST_TEST(terrain.left == calcT2(MapPoint(5, 2)));
        BOOST_TEST(terrain.right == calcT1(MapPoint(0, 2)));
    }
    // Now assume GetTerrain works and only check for consistency:
    RTTR_FOREACH_PT(MapPoint, world.GetSize())
    {
        BOOST_TEST_CONTEXT(pt)
        {
            const auto terrains = world.GetTerrainsAround(pt);
            for(const auto dir : helpers::enumRange<Direction>())
            {
                const auto terrain = world.GetTerrain(pt, dir);
                BOOST_TEST(terrain.left == terrains[dir - 1u]);
                BOOST_TEST(terrain.right == terrains[dir]);
            }
        }
    }
}

BOOST_AUTO_TEST_SUITE_END()
