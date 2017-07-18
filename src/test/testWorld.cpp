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
#include "files.h"
#include "test/WorldFixture.h"
#include "test/CreateEmptyWorld.h"
#include "test/BQOutput.h"
#include "libsiedler2/src/ArchivItem_Map_Header.h"
#include "libutil/src/tmpFile.h"
#include <boost/test/unit_test.hpp>
#include <boost/lambda/lambda.hpp>
#include <boost/lambda/bind.hpp>
#include <boost/foreach.hpp>
#include <boost/filesystem/fstream.hpp>

BOOST_AUTO_TEST_SUITE(MapTestSuite)

const std::string testMapPath = std::string(FILE_PATHS[52]) + "Bergruft.swd";

BOOST_AUTO_TEST_CASE(LoadSaveMap)
{
    // Check that loading and saving a map does not alter it
    glArchivItem_Map map;
    bfs::ifstream mapFile(testMapPath, std::ios::binary);
    BOOST_REQUIRE_EQUAL(map.load(mapFile, false), 0);
    TmpFile outMap(".swd");
    BOOST_REQUIRE(outMap.IsValid());
    BOOST_REQUIRE_EQUAL(map.write(outMap.GetStream()), 0);
    mapFile.close();
    outMap.GetStream().close();
    BOOST_REQUIRE_EQUAL(CalcChecksumOfFile(testMapPath), CalcChecksumOfFile(outMap.filePath));
}

namespace{
    struct UninitializedWorldCreator
    {
        UninitializedWorldCreator(const MapExtent& size, unsigned numPlayers){}
        bool operator()(GameWorldBase& world){ return true; }
    };

    struct LoadWorldFromFileCreator
    {
        glArchivItem_Map map;
        std::vector<MapPoint> hqs;
        const unsigned numPlayers_;

        LoadWorldFromFileCreator(const MapExtent& size, unsigned numPlayers): numPlayers_(numPlayers){}
        bool operator()(GameWorldBase& world)
        {
            bfs::ifstream mapFile(testMapPath, std::ios::binary);
            BOOST_REQUIRE_EQUAL(map.load(mapFile, false), 0);
            std::vector<Nation> nations;
            for(unsigned i = 0; i < numPlayers_; i++)
                nations.push_back(world.GetPlayer(i).nation);
            MapLoader loader(world, nations);
            BOOST_REQUIRE(loader.Load(map, false, EXP_FOGOFWAR));
            for(unsigned i = 0; i < numPlayers_; i++)
                hqs.push_back(loader.GetHQPos(i));
            return true;
        }
    };

    struct WorldLoadedFixture: public WorldFixture<LoadWorldFromFileCreator>
    {
        using WorldFixture<LoadWorldFromFileCreator>::world;
    };
    struct WorldLoaded1PFixture: public WorldFixture<LoadWorldFromFileCreator, 1>
    {
        using WorldFixture<LoadWorldFromFileCreator, 1>::world;
    };
}

BOOST_FIXTURE_TEST_CASE(LoadWorld, WorldFixture<UninitializedWorldCreator>)
{
    glArchivItem_Map map;
    bfs::ifstream mapFile(testMapPath, std::ios::binary);
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
        BuildingQuality s2BQ = BuildingQuality(worldCreator.map.GetMapDataAt(MAP_BQ, pt.x, pt.y) & 0x7);
        BuildingQuality bq = world.GetNode(pt).bq;
        BOOST_REQUIRE_MESSAGE(bq == s2BQ, bqNames[bq] << "!=" << bqNames[s2BQ] << " at " << pt.x << "," << pt.y
            << " original:" << worldCreator.map.GetMapDataAt(MAP_BQ, pt.x, pt.y));
    }
}

BOOST_FIXTURE_TEST_CASE(HQPlacement, WorldLoaded1PFixture)
{
    GamePlayer& player = world.GetPlayer(0);
    BOOST_REQUIRE(player.isUsed());
    BOOST_REQUIRE(worldCreator.hqs[0].isValid());
    BOOST_REQUIRE_EQUAL(world.GetNO(worldCreator.hqs[0])->GetGOT(), GOT_NOB_HQ);
}

BOOST_AUTO_TEST_SUITE_END()
