// Copyright (c) 2005 - 2015 Settlers Freaks (sf-team at siedler25.org)
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
#include "GameObject.h"
#include "EventManager.h"
#include "GlobalGameSettings.h"
#include "PlayerInfo.h"
#include "world/BQCalculator.h"
#include "ogl/glArchivItem_Map.h"
#include "FileChecksum.h"
#include "libsiedler2/src/ArchivItem_Map_Header.h"
#include "libutil/src/tmpFile.h"
#include <boost/test/unit_test.hpp>
#include <fstream>

#define RTTR_FOREACH_PT(TYPE, WIDTH, HEIGHT)        \
        for(TYPE pt(0, 0); pt.y < (HEIGHT); ++pt.y) \
            for(pt.x = 0; pt.x < (WIDTH); ++pt.x)

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
};

// Provides a world object with default settings and no players
struct WorldFixture
{
    EventManager em;
    GlobalGameSettings ggs;
    GameWorldGame world;
    WorldFixture(): em(0), world(std::vector<PlayerInfo>(0), ggs, em)
    {
        GameObject::SetPointers(&world);
    }
    ~WorldFixture()
    {
        // Reset to allow assertions on GameObject destruction to pass
        GameObject::SetPointers(NULL);
    }
};

bool RetFalse(MapPoint pt){
    return false;
}

BOOST_FIXTURE_TEST_CASE(LoadWorld, WorldFixture)
{
    glArchivItem_Map map;
    std::ifstream mapFile(testMapPath, std::ios::binary);
    BOOST_REQUIRE_EQUAL(map.load(mapFile, false), 0);
    BOOST_CHECK_EQUAL(map.getHeader().getWidth(), 176);
    BOOST_CHECK_EQUAL(map.getHeader().getHeight(), 80);
    BOOST_CHECK_EQUAL(map.getHeader().getPlayer(), 4);

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

    WorldLoadedFixture()
    {
        std::ifstream mapFile(testMapPath, std::ios::binary);
        BOOST_REQUIRE_EQUAL(map.load(mapFile, false), 0);
        std::vector<Nation> nations(0);
        MapLoader loader(world, nations);
        BOOST_REQUIRE(loader.Load(map, false, EXP_FOGOFWAR));
    }
};

BOOST_FIXTURE_TEST_CASE(CheckHeight, WorldLoadedFixture)
{
    RTTR_FOREACH_PT(MapPoint, world.GetWidth(), world.GetHeight())
    {
        BOOST_REQUIRE_EQUAL(world.GetNode(pt).altitude, map.GetMapDataAt(MAP_ALTITUDE, pt.x, pt.y));
    }
}

BOOST_FIXTURE_TEST_CASE(CheckBQs, WorldLoadedFixture)
{
    const char* bqNames[] = {"Nothing", "Flag", "Hut", "House", "Castle", "Mine"};

    BQCalculator bqCalculator(world);
    bqCalculator(MapPoint(41, 9), RetFalse);


    RTTR_FOREACH_PT(MapPoint, world.GetWidth(), world.GetHeight())
    {
        BuildingQuality s2BQ = BuildingQuality(map.GetMapDataAt(MAP_BQ, pt.x, pt.y) & 0x7);
        BuildingQuality bq = bqCalculator(pt, RetFalse);
        BOOST_REQUIRE_MESSAGE(bq == s2BQ, bqNames[bq] << "!=" << bqNames[s2BQ] << " at " << pt.x << "," << pt.y << " original:" << map.GetMapDataAt(MAP_BQ, pt.x, pt.y));
    }
}

BOOST_AUTO_TEST_SUITE_END()
