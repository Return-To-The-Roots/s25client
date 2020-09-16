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

#include "BQOutput.h"
#include "FileChecksum.h"
#include "GamePlayer.h"
#include "PointOutput.h"
#include "RttrConfig.h"
#include "RttrForeachPt.h"
#include "files.h"
#include "lua/GameDataLoader.h"
#include "ogl/glArchivItem_Map.h"
#include "worldFixtures/CreateEmptyWorld.h"
#include "worldFixtures/WorldFixture.h"
#include "world/MapLoader.h"
#include "nodeObjs/noBase.h"
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
    BOOST_REQUIRE_EQUAL(map.load(mapFile, false), 0);
    TmpFile outMap(".swd");
    BOOST_REQUIRE(outMap.isValid());
    BOOST_REQUIRE_EQUAL(map.write(outMap.getStream()), 0);
    mapFile.close();
    outMap.close();
    BOOST_REQUIRE_EQUAL(CalcChecksumOfFile(testMapPath), CalcChecksumOfFile(outMap.filePath));
}

namespace {
struct UninitializedWorldCreator
{
    explicit UninitializedWorldCreator(const MapExtent&) {}
    bool operator()(GameWorldBase&) { return true; }
};

struct LoadWorldFromFileCreator : MapTestFixture
{
    glArchivItem_Map map;
    std::vector<MapPoint> hqs;

    explicit LoadWorldFromFileCreator(const MapExtent&) {}
    bool operator()(GameWorldBase& world)
    {
        bnw::ifstream mapFile(testMapPath, std::ios::binary);
        if(map.load(mapFile, false) != 0)
            throw std::runtime_error("Could not load file " + testMapPath.string()); // LCOV_EXCL_LINE
        MapLoader loader(world);
        if(!loader.Load(map, EXP_FOGOFWAR))
            throw std::runtime_error("Could not load map"); // LCOV_EXCL_LINE
        if(!loader.PlaceHQs(world, false))
            throw std::runtime_error("Could not place HQs"); // LCOV_EXCL_LINE
        for(unsigned i = 0; i < world.GetNumPlayers(); i++)
            hqs.push_back(loader.GetHQPos(i));
        return true;
    }
};

struct WorldLoadedFixture : public WorldFixture<LoadWorldFromFileCreator>
{
    using WorldFixture<LoadWorldFromFileCreator>::world;
};
struct WorldLoaded1PFixture : public WorldFixture<LoadWorldFromFileCreator, 1>
{
    using WorldFixture<LoadWorldFromFileCreator, 1>::world;
};
} // namespace

BOOST_FIXTURE_TEST_CASE(LoadWorld, WorldFixture<UninitializedWorldCreator>)
{
    MapTestFixture fixture;
    glArchivItem_Map map;
    bnw::ifstream mapFile(fixture.testMapPath, std::ios::binary);
    BOOST_REQUIRE_EQUAL(map.load(mapFile, false), 0);
    const libsiedler2::ArchivItem_Map_Header& header = map.getHeader();
    BOOST_CHECK_EQUAL(header.getWidth(), 176);
    BOOST_CHECK_EQUAL(header.getHeight(), 80);
    BOOST_CHECK_EQUAL(header.getNumPlayers(), 4);

    MapLoader loader(world);
    BOOST_REQUIRE(loader.Load(map, EXP_FOGOFWAR));
    BOOST_CHECK_EQUAL(world.GetWidth(), map.getHeader().getWidth());
    BOOST_CHECK_EQUAL(world.GetHeight(), map.getHeader().getHeight());
}

BOOST_FIXTURE_TEST_CASE(HeightLoading, WorldLoadedFixture)
{
    RTTR_FOREACH_PT(MapPoint, world.GetSize())
    {
        BOOST_REQUIRE_EQUAL(world.GetNode(pt).altitude, worldCreator.map.GetMapDataAt(MAP_ALTITUDE, pt.x, pt.y));
    }
}

BOOST_FIXTURE_TEST_CASE(SameBQasInS2, WorldLoadedFixture)
{
    // Init BQ
    world.InitAfterLoad();
    RTTR_FOREACH_PT(MapPoint, world.GetSize())
    {
        auto s2BQ = BuildingQuality(worldCreator.map.GetMapDataAt(MAP_BQ, pt.x, pt.y) & 0x7);
        BuildingQuality bq = world.GetNode(pt).bq;
        BOOST_REQUIRE_MESSAGE(bq == s2BQ, bqNames[bq] << "!=" << bqNames[s2BQ] << " at " << pt << " original:"
                                                      << worldCreator.map.GetMapDataAt(MAP_BQ, pt.x, pt.y));
    }
}

BOOST_FIXTURE_TEST_CASE(HQPlacement, WorldLoaded1PFixture)
{
    GamePlayer& player = world.GetPlayer(0);
    BOOST_REQUIRE(player.isUsed());
    BOOST_REQUIRE(worldCreator.hqs[0].isValid());
    BOOST_REQUIRE_EQUAL(world.GetNO(worldCreator.hqs[0])->GetGOT(), GOT_NOB_HQ);
}

BOOST_FIXTURE_TEST_CASE(CloseHarborSpots, WorldFixture<UninitializedWorldCreator>)
{
    loadGameData(world.GetDescriptionWriteable());
    DescIdx<TerrainDesc> tWater(0);
    for(; tWater.value < world.GetDescription().terrain.size(); tWater.value++)
    {
        if(world.GetDescription().get(tWater).kind == TerrainKind::WATER
           && !world.GetDescription().get(tWater).Is(ETerrain::Walkable))
            break;
    }
    DescIdx<TerrainDesc> tLand(0);
    for(; tLand.value < world.GetDescription().terrain.size(); tLand.value++)
    {
        if(world.GetDescription().get(tLand).kind == TerrainKind::LAND
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
    hbPos.push_back(world.GetNeighbour(hbPos.back(), Direction::NORTHWEST));

    hbPos.push_back(MapPoint(10, 20)); //-V525
    hbPos.push_back(world.GetNeighbour(hbPos.back(), Direction::NORTHEAST));

    hbPos.push_back(MapPoint(0, 10));
    hbPos.push_back(world.GetNeighbour(hbPos.back(), Direction::SOUTHEAST));

    hbPos.push_back(MapPoint(20, 10));
    hbPos.push_back(world.GetNeighbour(hbPos.back(), Direction::SOUTHWEST));

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
    waterPts.push_back(world.GetNeighbour(world.GetNeighbour(hbPos[0], Direction::SOUTHWEST), Direction::SOUTHWEST));
    waterPts.push_back(world.GetNeighbour(world.GetNeighbour(hbPos[0], Direction::SOUTHEAST), Direction::SOUTHEAST));
    waterPts.push_back(world.GetNeighbour(world.GetNeighbour(hbPos[3], Direction::NORTHEAST), Direction::NORTHEAST));
    waterPts.push_back(world.GetNeighbour(world.GetNeighbour(hbPos[5], Direction::EAST), Direction::EAST));
    waterPts.push_back(world.GetNeighbour(world.GetNeighbour(hbPos[7], Direction::SOUTHWEST), Direction::SOUTHWEST));
    waterPts.push_back(world.GetNeighbour(world.GetNeighbour(hbPos[9], Direction::SOUTHEAST), Direction::SOUTHEAST));

    for(const MapPoint& pt : waterPts)
    {
        for(const auto dir : helpers::EnumRange<Direction>{})
            setRightTerrain(world, pt, dir, tWater);
    }

    // Check if this works
    BOOST_REQUIRE(MapLoader::InitSeasAndHarbors(world, hbPos));
    // All harbors valid
    BOOST_REQUIRE_EQUAL(world.GetNumHarborPoints(), hbPos.size());
    for(unsigned startHb = 1; startHb < world.GetNumHarborPoints(); startHb++)
    {
        for(const auto dir : helpers::EnumRange<Direction>{})
        {
            unsigned seaId = world.GetSeaId(startHb, dir);
            if(!seaId)
                continue;
            MapPoint startPt = world.GetCoastalPoint(startHb, seaId);
            BOOST_REQUIRE_EQUAL(startPt, world.GetNeighbour(world.GetHarborPoint(startHb), dir));
            for(unsigned targetHb = 1; targetHb < world.GetNumHarborPoints(); targetHb++)
            {
                MapPoint destPt = world.GetCoastalPoint(targetHb, seaId);
                BOOST_REQUIRE(destPt.isValid());
                std::vector<Direction> route;
                BOOST_REQUIRE(startPt == destPt || world.FindShipPath(startPt, destPt, 10000, &route, nullptr));
                BOOST_REQUIRE_EQUAL(route.size(), world.CalcHarborDistance(startHb, targetHb));
            }
        }
    }
}

BOOST_AUTO_TEST_SUITE_END()
