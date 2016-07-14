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
#include "world/MapLoader.h"
#include "world/GameWorldGame.h"
#include "GamePlayer.h"
#include "GameObject.h"
#include "nodeObjs/noBase.h"
#include "EventManager.h"
#include "GlobalGameSettings.h"
#include "PlayerInfo.h"
#include "ogl/glArchivItem_Map.h"
#include "FileChecksum.h"
#include "libsiedler2/src/ArchivItem_Map_Header.h"
#include "libutil/src/tmpFile.h"
#include <boost/test/unit_test.hpp>
#include <boost/lambda/lambda.hpp>
#include <boost/lambda/bind.hpp>
#include <boost/foreach.hpp>
#include <fstream>

BOOST_AUTO_TEST_SUITE(MapTestSuite)

const char* testMapPath = "RTTR/MAPS/NEW/Bergruft.swd";

BOOST_AUTO_TEST_CASE(LoadSaveMap)
{
    // Check that loading and saving a map does not alter it
    glArchivItem_Map map;
    std::ifstream mapFile(testMapPath, std::ios::binary);
    BOOST_REQUIRE_EQUAL(map.load(mapFile, false), 0);
    TmpFile outMap(".swd");
    BOOST_REQUIRE(outMap.IsValid());
    BOOST_REQUIRE_EQUAL(map.write(outMap.GetStream()), 0);
    mapFile.close();
    outMap.GetStream().close();
    BOOST_REQUIRE_EQUAL(CalcChecksumOfFile(testMapPath), CalcChecksumOfFile(outMap.filePath.c_str()));
}

// Provides a world object with default settings and no players
struct WorldFixture
{
    EventManager em;
    GlobalGameSettings ggs;
    GameWorldGame world;
    WorldFixture(unsigned numPlayers = 0): em(0), world(std::vector<PlayerInfo>(numPlayers, GetPlayer()), ggs, em)
    {
        GameObject::SetPointers(&world);
        BOOST_REQUIRE_EQUAL(world.GetPlayerCount(), numPlayers);
    }
    ~WorldFixture()
    {
        // Reset to allow assertions on GameObject destruction to pass
        GameObject::SetPointers(NULL);
    }
    static PlayerInfo GetPlayer()
    {
        PlayerInfo result;
        result.ps = PS_OCCUPIED;
        return result;
    }
};

BOOST_FIXTURE_TEST_CASE(LoadWorld, WorldFixture)
{
    glArchivItem_Map map;
    std::ifstream mapFile(testMapPath, std::ios::binary);
    BOOST_REQUIRE_EQUAL(map.load(mapFile, false), 0);
    const libsiedler2::ArchivItem_Map_Header& header = map.getHeader();
    BOOST_CHECK_EQUAL(header.getWidth(), 176);
    BOOST_CHECK_EQUAL(header.getHeight(), 80);
    BOOST_CHECK_EQUAL(header.getPlayer(), 4);

    std::vector<Nation> nations(0);
    MapLoader loader(world, nations);
    BOOST_REQUIRE(loader.Load(map, false, EXP_FOGOFWAR));
    BOOST_CHECK_EQUAL(world.GetWidth(), map.getHeader().getWidth());
    BOOST_CHECK_EQUAL(world.GetHeight(), map.getHeader().getHeight());
}

// Additionally loads the map to the world
struct WorldLoadedFixture: public WorldFixture
{
    glArchivItem_Map map;
    std::vector<MapPoint> hqs;

    WorldLoadedFixture(unsigned numPlayers = 0): WorldFixture(numPlayers)
    {
        std::ifstream mapFile(testMapPath, std::ios::binary);
        BOOST_REQUIRE_EQUAL(map.load(mapFile, false), 0);
        std::vector<Nation> nations;
        for(unsigned i = 0; i < numPlayers; i++)
            nations.push_back(world.GetPlayer(i).nation);
        MapLoader loader(world, nations);
        BOOST_REQUIRE(loader.Load(map, false, EXP_FOGOFWAR));
        for(unsigned i = 0; i < numPlayers; i++)
            hqs.push_back(loader.GetHQPos(i));
    }
};

BOOST_FIXTURE_TEST_CASE(HeightLoading, WorldLoadedFixture)
{
    RTTR_FOREACH_PT(MapPoint, world.GetWidth(), world.GetHeight())
    {
        BOOST_REQUIRE_EQUAL(world.GetNode(pt).altitude, map.GetMapDataAt(MAP_ALTITUDE, pt.x, pt.y));
    }
}

const char* bqNames[] = {"Nothing", "Flag", "Hut", "House", "Castle", "Mine"};

BOOST_FIXTURE_TEST_CASE(SameBQasInS2, WorldLoadedFixture)
{
    // Init BQ
    world.InitAfterLoad();
    RTTR_FOREACH_PT(MapPoint, world.GetWidth(), world.GetHeight())
    {
        BuildingQuality s2BQ = BuildingQuality(map.GetMapDataAt(MAP_BQ, pt.x, pt.y) & 0x7);
        BuildingQuality bq = world.GetNode(pt).bq;
        BOOST_REQUIRE_MESSAGE(bq == s2BQ, bqNames[bq] << "!=" << bqNames[s2BQ] << " at " << pt.x << "," << pt.y << " original:" << map.GetMapDataAt(MAP_BQ, pt.x, pt.y));
    }
}

struct WorldFixture1P: public WorldFixture
{
    WorldFixture1P():WorldFixture(1){}
};

BOOST_FIXTURE_TEST_CASE(BQWithRoad, WorldFixture)
{
    glArchivItem_Map map;
    std::ifstream mapFile(testMapPath, std::ios::binary);
    BOOST_REQUIRE_EQUAL(map.load(mapFile, false), 0);
    // Flatten land and set terrain to meadow to avoid side-effects
    RTTR_FOREACH_PT(MapPoint, map.getHeader().getWidth(), map.getHeader().getHeight())
    {
        map.SetMapDataAt(MAP_TERRAIN1, pt.x, pt.y, 8);
        map.SetMapDataAt(MAP_TERRAIN2, pt.x, pt.y, 8);
        map.SetMapDataAt(MAP_ALTITUDE, pt.x, pt.y, 10);
        map.SetMapDataAt(MAP_TYPE, pt.x, pt.y, 0);
    }
    MapLoader loader(world, std::vector<Nation>());
    BOOST_REQUIRE(loader.Load(map, false, EXP_FOGOFWAR));
    // Init BQ
    world.InitAfterLoad();
    RTTR_FOREACH_PT(MapPoint, world.GetWidth(), world.GetHeight())
    {
        BuildingQuality bq = world.GetNode(pt).bq;
        BOOST_REQUIRE_MESSAGE(bq == BQ_CASTLE, bqNames[bq] << "!=" << bqNames[BQ_CASTLE] << " at " << pt.x << "," << pt.y);
    }
    // Create a road of length 6
    std::vector<MapPoint> roadPts;
    MapPoint curPt(10, 10);
    for(unsigned i=0; i<6; i++)
    {
        roadPts.push_back(curPt);
        world.SetPointRoad(curPt, 4, 1);
        world.RecalcBQForRoad(curPt);
        curPt = world.GetNeighbour(curPt, 4);
    }
    // Final pt still belongs to road
    roadPts.push_back(curPt);
    // Normally this is done by the flag
    world.RecalcBQForRoad(curPt);

    BOOST_FOREACH(MapPoint pt, roadPts)
    {
        BOOST_REQUIRE(world.IsOnRoad(pt));
        // On the road we only allow flags
        BOOST_REQUIRE_EQUAL(world.GetNode(pt).bq, BQ_FLAG);
        // Next to the road should be houses
        // But left to first point is still a castle
        BuildingQuality leftBQ = (pt == roadPts[0]) ? BQ_CASTLE : BQ_HOUSE;
        BOOST_REQUIRE_EQUAL(world.GetNode(pt - MapPoint(1, 0)).bq, leftBQ);
        BOOST_REQUIRE_EQUAL(world.GetNode(pt + MapPoint(1, 0)).bq, BQ_HOUSE);
    }
}

struct WorldLoadedPlayerFixture: public WorldLoadedFixture
{
    WorldLoadedPlayerFixture(): WorldLoadedFixture(1) {}
};

BOOST_FIXTURE_TEST_CASE(HQPlacement, WorldLoadedPlayerFixture)
{
    GamePlayer& player = world.GetPlayer(0);
    BOOST_REQUIRE(player.isUsed());
    BOOST_REQUIRE(hqs[0].isValid());
    BOOST_REQUIRE_EQUAL(world.GetNO(hqs[0])->GetGOT(), GOT_NOB_HQ);
}

BOOST_AUTO_TEST_SUITE_END()
