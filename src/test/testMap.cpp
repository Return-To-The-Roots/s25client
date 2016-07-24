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
#include "test/WorldFixture.h"
#include "test/CreateEmptyWorld.h"
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

namespace{
    struct UninitializedWorldCreator
    {
        UninitializedWorldCreator(unsigned w, unsigned h, unsigned numPlayers){}
        bool operator()(GameWorldBase& world){ return true; }
    };

    struct LoadWorldFromFileCreator
    {
        glArchivItem_Map map;
        std::vector<MapPoint> hqs;
        const unsigned numPlayers_;

        LoadWorldFromFileCreator(unsigned w, unsigned h, unsigned numPlayers): numPlayers_(numPlayers){}
        bool operator()(GameWorldBase& world)
        {
            std::ifstream mapFile(testMapPath, std::ios::binary);
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

BOOST_FIXTURE_TEST_CASE(HeightLoading, WorldLoadedFixture)
{
    RTTR_FOREACH_PT(MapPoint, world.GetWidth(), world.GetHeight())
    {
        BOOST_REQUIRE_EQUAL(world.GetNode(pt).altitude, worldCreator.map.GetMapDataAt(MAP_ALTITUDE, pt.x, pt.y));
    }
}

const char* bqNames[] = {"Nothing", "Flag", "Hut", "House", "Castle", "Mine"};

BOOST_FIXTURE_TEST_CASE(SameBQasInS2, WorldLoadedFixture)
{
    // Init BQ
    world.InitAfterLoad();
    RTTR_FOREACH_PT(MapPoint, world.GetWidth(), world.GetHeight())
    {
        BuildingQuality s2BQ = BuildingQuality(worldCreator.map.GetMapDataAt(MAP_BQ, pt.x, pt.y) & 0x7);
        BuildingQuality bq = world.GetNode(pt).bq;
        BOOST_REQUIRE_MESSAGE(bq == s2BQ, bqNames[bq] << "!=" << bqNames[s2BQ] << " at " << pt.x << "," << pt.y
            << " original:" << worldCreator.map.GetMapDataAt(MAP_BQ, pt.x, pt.y));
    }
}

typedef WorldFixture<UninitializedWorldCreator, 1> UninitializedWorld1PFixture;

BOOST_FIXTURE_TEST_CASE(BQWithRoad, UninitializedWorld1PFixture)
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

typedef WorldFixture<CreateEmptyWorld, 1, 22, 22> EmptyWorldFixture1P;

BOOST_FIXTURE_TEST_CASE(BQ_AtBorder, EmptyWorldFixture1P)
{
    // Calc all BQs
    world.InitAfterLoad();
    GamePlayer& player = world.GetPlayer(0);
    const MapPoint hqPos = player.GetHQPos();
    const nobBaseMilitary* hq = world.GetSpecObj<nobBaseMilitary>(hqPos);
    BOOST_REQUIRE(hq);
    const unsigned hqRadius = hq->GetMilitaryRadius();
    BOOST_REQUIRE_GT(hqRadius, 4u);
    std::vector<MapPoint> pts = world.GetPointsInRadius(hqPos, hq->GetMilitaryRadius() + 1);
    BOOST_FOREACH(const MapPoint pt, pts)
    {
        const unsigned distance = world.CalcDistance(pt, hqPos);
        if(distance < 3)
            continue; // Influenced by HQ
        else if(distance + 1 < hqRadius)
        {
            // Our territory -> No restrictions
            BOOST_REQUIRE(world.IsPlayerTerritory(pt));
            BOOST_REQUIRE_EQUAL(world.GetBQ(pt, 0), BQ_CASTLE);
        } else if(distance < hqRadius)
        {
            // Near border, flag if we cant build a buildings flag
            BOOST_REQUIRE(world.IsPlayerTerritory(pt));
            if(!world.IsPlayerTerritory(world.GetNeighbour(pt, Direction::SOUTHEAST)))
                BOOST_REQUIRE_EQUAL(world.GetBQ(pt, 0), BQ_FLAG);
            else
                BOOST_REQUIRE_EQUAL(world.GetBQ(pt, 0), BQ_CASTLE);
        } else if(distance == hqRadius)
        {
            // At border
            BOOST_REQUIRE(!world.IsPlayerTerritory(pt));
            BOOST_REQUIRE_EQUAL(world.GetNode(pt).owner, 0u + 1u);
            BOOST_REQUIRE_EQUAL(world.GetBQ(pt, 0), BQ_NOTHING);
        } else
        {
            // Outside border
            BOOST_REQUIRE(!world.IsPlayerTerritory(pt));
            BOOST_REQUIRE_EQUAL(world.GetNode(pt).owner, 0u);
            BOOST_REQUIRE_EQUAL(world.GetBQ(pt, 0), BQ_NOTHING);
        }
    }

    // Place a flag near a border
    MapPoint flagPt = hqPos;
    for(unsigned i = 0; i < hqRadius - 2; i++)
        flagPt = world.GetNeighbour(flagPt, i % 2 ? Direction::SOUTHEAST : Direction::SOUTHWEST);
    // This is near the border, so only a flag is possible
    BOOST_REQUIRE_EQUAL(world.GetBQ(world.GetNeighbour(flagPt, Direction::SOUTHEAST), 0), BQ_FLAG);
    world.SetFlag(flagPt, 0);
    BOOST_REQUIRE_EQUAL(world.GetNO(flagPt)->GetGOT(), GOT_FLAG);
    BOOST_REQUIRE_EQUAL(world.GetBQ(world.GetNeighbour(flagPt, Direction::WEST), 0), BQ_NOTHING); // Buildings flag or flag to close to this
    BOOST_REQUIRE_EQUAL(world.GetBQ(world.GetNeighbour(flagPt, Direction::NORTHWEST), 0), BQ_CASTLE); // Building to this flag
    BOOST_REQUIRE_EQUAL(world.GetBQ(world.GetNeighbour(flagPt, Direction::NORTHEAST), 0), BQ_NOTHING); // Buildings flag or flag to close to this
    BOOST_REQUIRE_EQUAL(world.GetBQ(world.GetNeighbour(flagPt, Direction::EAST), 0), BQ_HOUSE); // This flag blocks castles extensions
    // This flag is to close and border prohibits building
    BOOST_REQUIRE_EQUAL(world.GetBQ(world.GetNeighbour(flagPt, Direction::SOUTHEAST), 0), BQ_NOTHING);
    BOOST_REQUIRE_EQUAL(world.GetBQ(world.GetNeighbour(flagPt, Direction::SOUTHWEST), 0), BQ_NOTHING);
}

BOOST_FIXTURE_TEST_CASE(HQPlacement, WorldLoaded1PFixture)
{
    GamePlayer& player = world.GetPlayer(0);
    BOOST_REQUIRE(player.isUsed());
    BOOST_REQUIRE(worldCreator.hqs[0].isValid());
    BOOST_REQUIRE_EQUAL(world.GetNO(worldCreator.hqs[0])->GetGOT(), GOT_NOB_HQ);
}

BOOST_AUTO_TEST_SUITE_END()
