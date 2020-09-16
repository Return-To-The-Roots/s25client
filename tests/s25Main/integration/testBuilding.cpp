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
#include "GamePlayer.h"
#include "PointOutput.h"
#include "RttrForeachPt.h"
#include "buildings/nobBaseMilitary.h"
#include "desktops/dskGameInterface.h"
#include "helpers/containerUtils.h"
#include "uiHelper/uiHelpers.hpp"
#include "worldFixtures/CreateEmptyWorld.h"
#include "worldFixtures/WorldFixture.h"
#include "world/GameWorldViewer.h"
#include "nodeObjs/noEnvObject.h"
#include "nodeObjs/noStaticObject.h"
#include "gameTypes/GameTypesOutput.h"
#include <boost/test/unit_test.hpp>

// Test stuff related to building/building quality
BOOST_AUTO_TEST_SUITE(BuildingSuite)

namespace {
using EmptyWorldFixture0P = WorldFixture<CreateEmptyWorld, 0>;
using EmptyWorldFixture1P = WorldFixture<CreateEmptyWorld, 1>;
using EmptyWorldFixture1PBigger = WorldFixture<CreateEmptyWorld, 1, 18, 16>;
using EmptyWorldFixture1PBiggest = WorldFixture<CreateEmptyWorld, 1, 22, 20>;
using ReducedBQMap = std::map<MapPoint, BuildingQuality, MapPointLess>;

/// Check that the BQ at all points is BQ_CASTLE except the points in the reducedBQs map which have given BQs
boost::test_tools::predicate_result checkBQs(const GameWorldBase& world, const std::vector<MapPoint>& pts,
                                             const ReducedBQMap& reducedBQs)
{
    for(MapPoint pt : pts)
    {
        BuildingQuality bqReq;
        if(helpers::contains(reducedBQs, pt))
            bqReq = reducedBQs.find(pt)->second; //-V783
        else
            bqReq = BQ_CASTLE;
        const BuildingQuality isBQ = world.GetNode(pt).bq;
        if(bqReq == isBQ)
            continue;
        boost::test_tools::predicate_result result(false);
        result.message() << isBQ << "!=" << bqReq << " at " << pt;
        return result;
    }
    return true;
}
} // namespace

BOOST_FIXTURE_TEST_CASE(checkBQs_Correct, EmptyWorldFixture1P)
{
    const MapPoint flagPos = world.MakeMapPoint(world.GetPlayer(0).GetHQPos() - Position(5, 6));
    const std::vector<MapPoint> pts = world.GetPointsInRadiusWithCenter(flagPos, 4);
    std::stringstream s;
    s << flagPos;

    ReducedBQMap reducedBQs;
    BOOST_TEST(checkBQs(world, pts, reducedBQs));

    world.GetNodeWriteable(flagPos).bq = BQ_FLAG;
    BOOST_TEST(checkBQs(world, pts, reducedBQs).message().str() == "Flag!=Castle at " + s.str());
    reducedBQs[flagPos] = BQ_FLAG;
    BOOST_TEST(checkBQs(world, pts, reducedBQs));
    world.GetNodeWriteable(flagPos).bq = BQ_CASTLE;
    BOOST_TEST(checkBQs(world, pts, reducedBQs).message().str() == "Castle!=Flag at " + s.str());
}

BOOST_FIXTURE_TEST_CASE(BQNextToBuilding, EmptyWorldFixture1P)
{
    const MapPoint flagPos = world.MakeMapPoint(world.GetPlayer(0).GetHQPos() - Position(5, 6));
    const MapPoint bldPos = world.GetNeighbour(flagPos, Direction::NORTHWEST);
    // All points possibly affected by a building
    const std::vector<MapPoint> pts = world.GetPointsInRadiusWithCenter(bldPos, 4);
    const std::vector<MapPoint> radius1Pts = world.GetPointsInRadius(bldPos, 1);
    ReducedBQMap reducedBQs;
    // Initially all are full
    BOOST_REQUIRE(checkBQs(world, pts, ReducedBQMap()));

    // Place flag
    world.SetFlag(flagPos, 0);
    reducedBQs.clear();
    reducedBQs[flagPos] = BQ_NOTHING;
    // Flag to any building would be to close and flag also to close (min diff of 2 required)
    reducedBQs[world.GetNeighbour(flagPos, Direction::WEST)] = BQ_NOTHING;
    reducedBQs[world.GetNeighbour(flagPos, Direction::NORTHEAST)] = BQ_NOTHING;
    // Can build houses but not castles
    for(Direction dir = Direction::EAST; dir != Direction::WEST; ++dir) //-V621
        reducedBQs[world.GetNeighbour(flagPos, dir)] = BQ_HOUSE;
    // Flag to bld is blocked by flag
    for(Direction dir = Direction::WEST; dir != Direction::EAST; ++dir)
        reducedBQs[world.GetNeighbour(bldPos, dir)] = BQ_FLAG;
    BOOST_REQUIRE(checkBQs(world, pts, reducedBQs));
    BOOST_REQUIRE(!world.IsRoadAvailable(false, flagPos));

    // Remove flag -> BQ reset
    world.DestroyFlag(flagPos, 0);
    BOOST_REQUIRE(checkBQs(world, pts, ReducedBQMap()));

    // Place hut -> Addionally to changes by flag we can't place blds in range 1
    // and no large blds in range 2
    world.SetBuildingSite(BLD_WOODCUTTER, bldPos, 0);
    BOOST_REQUIRE_EQUAL(world.GetSpecObj<noBaseBuilding>(bldPos)->GetSize(), BQ_HUT); //-V807
    reducedBQs[bldPos] = BQ_NOTHING;
    // Every bq with distance==2 has BQ_HOUSE
    for(unsigned i = 0; i < 12; i++)
        reducedBQs[world.GetNeighbour2(bldPos, i)] = BQ_HOUSE;
    BOOST_REQUIRE(checkBQs(world, pts, reducedBQs));
    BOOST_REQUIRE(!world.IsRoadAvailable(false, flagPos));
    BOOST_REQUIRE(!world.IsRoadAvailable(false, bldPos));
    for(MapPoint pt : radius1Pts)
        BOOST_REQUIRE(pt == flagPos || world.IsRoadAvailable(false, pt));

    // Remove -> BQ reset
    world.DestroyFlag(flagPos, 0);
    BOOST_REQUIRE(checkBQs(world, pts, ReducedBQMap()));

    // Place house -> Same as hut
    world.SetBuildingSite(BLD_SAWMILL, bldPos, 0);
    BOOST_REQUIRE_EQUAL(world.GetSpecObj<noBaseBuilding>(bldPos)->GetSize(), BQ_HOUSE);
    BOOST_REQUIRE(checkBQs(world, pts, reducedBQs));
    BOOST_REQUIRE(!world.IsRoadAvailable(false, flagPos));
    BOOST_REQUIRE(!world.IsRoadAvailable(false, bldPos));
    for(MapPoint pt : radius1Pts)
        BOOST_REQUIRE(pt == flagPos || world.IsRoadAvailable(false, pt));

    // Remove -> BQ reset
    world.DestroyFlag(flagPos, 0);
    BOOST_REQUIRE(checkBQs(world, pts, ReducedBQMap()));

    // Place castle
    world.SetBuildingSite(BLD_FORTRESS, bldPos, 0);
    BOOST_REQUIRE_EQUAL(world.GetSpecObj<noBaseBuilding>(bldPos)->GetSize(), BQ_CASTLE);
    // Addionally to reduced BQs by hut:
    // Even flag is blocked by castle (model size)
    for(Direction dir = Direction::WEST; dir != Direction::EAST; ++dir)
    {
        const MapPoint wouldBeFlagPt = world.GetNeighbour(bldPos, dir);
        reducedBQs[wouldBeFlagPt] = BQ_NOTHING;
        // And therefore also the bld
        reducedBQs[world.GetNeighbour(wouldBeFlagPt, Direction::NORTHWEST)] = BQ_FLAG;
    }
    BOOST_REQUIRE(checkBQs(world, pts, reducedBQs));
    BOOST_REQUIRE(!world.IsRoadAvailable(false, flagPos));
    BOOST_REQUIRE(!world.IsRoadAvailable(false, bldPos));
    // No roads over attachment
    BOOST_REQUIRE(!world.IsRoadAvailable(false, world.GetNeighbour(bldPos, Direction::WEST)));
    BOOST_REQUIRE(!world.IsRoadAvailable(false, world.GetNeighbour(bldPos, Direction::NORTHWEST)));
    BOOST_REQUIRE(!world.IsRoadAvailable(false, world.GetNeighbour(bldPos, Direction::NORTHEAST)));
    // But other 2 points are ok
    BOOST_REQUIRE(world.IsRoadAvailable(false, world.GetNeighbour(bldPos, Direction::SOUTHWEST)));
    BOOST_REQUIRE(world.IsRoadAvailable(false, world.GetNeighbour(bldPos, Direction::EAST)));

    // Remove -> BQ reset
    world.DestroyFlag(flagPos, 0);
    BOOST_REQUIRE(checkBQs(world, pts, ReducedBQMap()));
}

BOOST_FIXTURE_TEST_CASE(BQWithRoad, EmptyWorldFixture0P)
{
    RTTR_FOREACH_PT(MapPoint, world.GetSize())
    {
        BuildingQuality bq = world.GetNode(pt).bq;
        BOOST_REQUIRE_MESSAGE(bq == BQ_CASTLE, bqNames[bq] << "!=" << bqNames[BQ_CASTLE] << " at " << pt);
    }
    // Create a road of length 6
    std::vector<MapPoint> roadPts;
    MapPoint curPt(2, 3);
    for(unsigned i = 0; i < 6; i++)
    {
        roadPts.push_back(curPt);
        world.SetPointRoad(curPt, Direction::SOUTHEAST, PointRoad::Normal);
        world.RecalcBQForRoad(curPt);
        curPt = world.GetNeighbour(curPt, Direction::SOUTHEAST);
    }
    // Final pt still belongs to road
    roadPts.push_back(curPt);
    // Normally this is done by the flag
    world.RecalcBQForRoad(curPt);

    for(MapPoint pt : roadPts)
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

BOOST_FIXTURE_TEST_CASE(BQWithVisualRoad, EmptyWorldFixture1PBigger)
{
    uiHelper::initGUITests();
    RTTR_FOREACH_PT(MapPoint, world.GetSize())
        world.SetOwner(pt, 1);

    dskGameInterface gameDesktop(this->game, std::shared_ptr<NWFInfo>(), 0, false);
    const GameWorldViewer& gwv = gameDesktop.GetView().GetViewer();
    // Start at a position a bit away from the HQ so all points are castles
    const MapPoint roadPt = world.MakeMapPoint(world.GetPlayer(0).GetHQPos() - Position(6, 6));
    const std::vector<MapPoint> roadRadiusPts = world.GetPointsInRadiusWithCenter(roadPt, 6);
    for(const MapPoint& pt : roadRadiusPts)
    {
        BuildingQuality bq = world.GetNode(pt).bq;
        BOOST_REQUIRE_MESSAGE(bq == BQ_CASTLE, bq << "!=" << BQ_CASTLE << " at " << pt);
        bq = gwv.GetBQ(pt);
        BOOST_REQUIRE_MESSAGE(bq == BQ_CASTLE, bq << "!=" << BQ_CASTLE << " at " << pt);
    }
    gameDesktop.GI_StartRoadBuilding(roadPt, false);
    BOOST_REQUIRE_EQUAL(gameDesktop.GetRoadMode(), RM_NORMAL);

    std::vector<MapPoint> roadPts;
    MapPoint curPt = roadPt;
    for(unsigned i = 0; i < 6; i++)
    {
        roadPts.push_back(curPt);
        curPt = world.GetNeighbour(curPt, Direction::SOUTHEAST);
    }
    // Final pt still belongs to road
    roadPts.push_back(curPt);

    MapPoint curRoadEndPt = curPt;
    gameDesktop.BuildRoadPart(curRoadEndPt);
    BOOST_REQUIRE_EQUAL(curRoadEndPt, curPt);

    for(MapPoint pt : roadPts)
    {
        BOOST_REQUIRE(gwv.IsOnRoad(pt));
        // On the road we only allow flags
        BOOST_REQUIRE_EQUAL(gwv.GetBQ(pt), BQ_FLAG);
        // Next to the road should be houses
        // But left to first point is still a castle
        BuildingQuality leftBQ = (pt == roadPts[0]) ? BQ_CASTLE : BQ_HOUSE;
        BOOST_REQUIRE_EQUAL(gwv.GetBQ(world.GetNeighbour(pt, Direction::WEST)), leftBQ);
        BOOST_REQUIRE_EQUAL(gwv.GetBQ(world.GetNeighbour(pt, Direction::EAST)), BQ_HOUSE);
    }
    // Destroy road partially
    // The first point (after start) must have ID 2
    BOOST_REQUIRE_EQUAL(gameDesktop.GetIdInCurBuildRoad(roadPts[1]), 2u);
    gameDesktop.DemolishRoad(2);
    BOOST_REQUIRE(gwv.IsOnRoad(roadPts[0]));
    BOOST_REQUIRE(gwv.IsOnRoad(roadPts[1]));
    for(unsigned i = 2; i < roadPts.size(); i++)
        BOOST_REQUIRE(!gwv.IsOnRoad(roadPts[i]));
    // Remove rest
    gameDesktop.GI_CancelRoadBuilding();
    BOOST_REQUIRE(!gwv.IsOnRoad(roadPts[0]));
    BOOST_REQUIRE(!gwv.IsOnRoad(roadPts[1]));
    // BQ should be restored
    for(const MapPoint& pt : roadRadiusPts)
    {
        BuildingQuality bq = gwv.GetBQ(pt);
        BOOST_REQUIRE_MESSAGE(bq == BQ_CASTLE, bq << "!=" << BQ_CASTLE << " at " << pt);
    }

    BOOST_REQUIRE_EQUAL(gameDesktop.GetRoadMode(), RM_DISABLED);
    gameDesktop.GI_StartRoadBuilding(roadPt, false);
    BOOST_REQUIRE_EQUAL(gameDesktop.GetRoadMode(), RM_NORMAL);
    // Build horizontal road
    roadPts.clear();
    curPt = roadPt;
    for(unsigned i = 0; i < 6; i++)
    {
        roadPts.push_back(curPt);
        curPt = world.GetNeighbour(curPt, Direction::EAST);
    }
    // Final pt still belongs to road
    roadPts.push_back(curPt);

    curRoadEndPt = curPt;
    gameDesktop.BuildRoadPart(curRoadEndPt);
    BOOST_REQUIRE_EQUAL(curRoadEndPt, curPt);

    for(MapPoint pt : roadPts)
    {
        BOOST_REQUIRE(gwv.IsOnRoad(pt));
        // On the road we only allow flags
        BOOST_REQUIRE_EQUAL(gwv.GetBQ(pt), BQ_FLAG);
        // Above should be castles
        BOOST_REQUIRE_EQUAL(gwv.GetBQ(world.GetNeighbour(pt, Direction::NORTHWEST)), BQ_CASTLE);
        BOOST_REQUIRE_EQUAL(gwv.GetBQ(world.GetNeighbour(pt, Direction::NORTHEAST)), BQ_CASTLE);
        // Below should be big houses
        BOOST_REQUIRE_EQUAL(gwv.GetBQ(world.GetNeighbour(pt, Direction::SOUTHWEST)), BQ_HOUSE);
        BOOST_REQUIRE_EQUAL(gwv.GetBQ(world.GetNeighbour(pt, Direction::SOUTHEAST)), BQ_HOUSE);
    }

    // Destroy road
    gameDesktop.GI_CancelRoadBuilding();
    for(MapPoint pt : roadPts)
        BOOST_REQUIRE(!gwv.IsOnRoad(pt));
    // BQ should be restored
    for(const MapPoint& pt : roadRadiusPts)
    {
        BuildingQuality bq = gwv.GetBQ(pt);
        BOOST_REQUIRE_MESSAGE(bq == BQ_CASTLE, bq << "!=" << BQ_CASTLE << " at " << pt);
    }
}

BOOST_FIXTURE_TEST_CASE(BQ_AtBorder, EmptyWorldFixture1PBiggest)
{
    GamePlayer& player = world.GetPlayer(0);
    const MapPoint hqPos = player.GetHQPos();
    const nobBaseMilitary* hq = world.GetSpecObj<nobBaseMilitary>(hqPos);
    BOOST_REQUIRE(hq);
    const unsigned hqRadius = hq->GetMilitaryRadius();
    BOOST_REQUIRE_GT(hqRadius, 4u);
    BOOST_TEST_REQUIRE(hqRadius * 2u < world.GetWidth());
    BOOST_TEST_REQUIRE(hqRadius * 2u < world.GetHeight());
    std::vector<MapPoint> pts = world.GetPointsInRadius(hqPos, hqRadius + 1);
    for(const MapPoint pt : pts)
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
    BOOST_REQUIRE_EQUAL(world.GetBQ(world.GetNeighbour(flagPt, Direction::WEST), 0),
                        BQ_NOTHING); // Buildings flag or flag to close to this
    BOOST_REQUIRE_EQUAL(world.GetBQ(world.GetNeighbour(flagPt, Direction::NORTHWEST), 0),
                        BQ_CASTLE); // Building to this flag
    BOOST_REQUIRE_EQUAL(world.GetBQ(world.GetNeighbour(flagPt, Direction::NORTHEAST), 0),
                        BQ_NOTHING); // Buildings flag or flag to close to this
    BOOST_REQUIRE_EQUAL(world.GetBQ(world.GetNeighbour(flagPt, Direction::EAST), 0),
                        BQ_HOUSE); // This flag blocks castles extensions
                                   // This flag is to close and border prohibits building
    BOOST_REQUIRE_EQUAL(world.GetBQ(world.GetNeighbour(flagPt, Direction::SOUTHEAST), 0), BQ_NOTHING);
    BOOST_REQUIRE_EQUAL(world.GetBQ(world.GetNeighbour(flagPt, Direction::SOUTHWEST), 0), BQ_NOTHING);
}

static void addStaticObj(GameWorldBase& world, const MapPoint& pos, unsigned size)
{
    world.DestroyNO(pos, false);
    world.SetNO(pos, new noStaticObject(pos, 0, 0, size));
    world.RecalcBQAroundPointBig(pos);
}

BOOST_FIXTURE_TEST_CASE(BQNearObjects, EmptyWorldFixture1P)
{
    const MapPoint objPos = world.MakeMapPoint(world.GetPlayer(0).GetHQPos() - Position(3, 6));
    const std::vector<MapPoint> ptsAroundObj = world.GetPointsInRadiusWithCenter(objPos, 2);
    const std::vector<MapPoint> radius1Pts = world.GetPointsInRadius(objPos, 1);
    BOOST_REQUIRE(checkBQs(world, ptsAroundObj, ReducedBQMap()));
    ReducedBQMap reducedBQs;

    // Size=0 -> Block Nothing
    addStaticObj(world, objPos, 0);
    BOOST_REQUIRE(checkBQs(world, ptsAroundObj, ReducedBQMap()));
    BOOST_REQUIRE(world.IsRoadAvailable(false, objPos));
    for(MapPoint pt : radius1Pts)
        BOOST_REQUIRE(world.IsRoadAvailable(false, pt));

    // Size=1 -> Block only point
    addStaticObj(world, objPos, 1);
    reducedBQs[objPos] = BQ_NOTHING;
    reducedBQs[world.GetNeighbour(objPos, Direction::NORTHWEST)] = BQ_FLAG;
    // Can build houses but not castles
    for(Direction dir = Direction::EAST; dir != Direction::WEST; ++dir) //-V621
        reducedBQs[world.GetNeighbour(objPos, dir)] = BQ_HOUSE;
    BOOST_REQUIRE(checkBQs(world, ptsAroundObj, reducedBQs));
    BOOST_REQUIRE(!world.IsRoadAvailable(false, objPos));
    for(MapPoint pt : radius1Pts)
        BOOST_REQUIRE(world.IsRoadAvailable(false, pt));

    // Size=2 -> Block like a castle
    addStaticObj(world, objPos, 2);
    // Addionally reduced BQs:
    // Extensions block spots
    for(Direction dir = Direction::WEST; dir != Direction::EAST; ++dir)
    {
        const MapPoint extensionPos = world.GetNeighbour(objPos, dir);
        reducedBQs[extensionPos] = BQ_NOTHING;
        // And therefore also the bld
        reducedBQs[world.GetNeighbour(extensionPos, Direction::NORTHWEST)] = BQ_FLAG;
    }
    // Also blocked by extensions:
    reducedBQs[world.GetNeighbour(world.GetNeighbour(objPos, Direction::WEST), Direction::SOUTHWEST)] = BQ_HOUSE;
    reducedBQs[world.GetNeighbour(world.GetNeighbour(objPos, Direction::NORTHEAST), Direction::EAST)] = BQ_HOUSE;
    BOOST_REQUIRE(checkBQs(world, ptsAroundObj, reducedBQs));
    BOOST_REQUIRE(!world.IsRoadAvailable(false, objPos));
    // No roads over attachment
    BOOST_REQUIRE(!world.IsRoadAvailable(false, world.GetNeighbour(objPos, Direction::WEST)));
    BOOST_REQUIRE(!world.IsRoadAvailable(false, world.GetNeighbour(objPos, Direction::NORTHWEST)));
    BOOST_REQUIRE(!world.IsRoadAvailable(false, world.GetNeighbour(objPos, Direction::NORTHEAST)));
    // But other 3 points are ok
    BOOST_REQUIRE(world.IsRoadAvailable(false, world.GetNeighbour(objPos, Direction::EAST)));
    BOOST_REQUIRE(world.IsRoadAvailable(false, world.GetNeighbour(objPos, Direction::SOUTHEAST)));
    BOOST_REQUIRE(world.IsRoadAvailable(false, world.GetNeighbour(objPos, Direction::SOUTHWEST)));
}

BOOST_FIXTURE_TEST_CASE(RoadRemovesObjs, EmptyWorldFixture1P)
{
    RTTR_FOREACH_PT(MapPoint, world.GetSize())
        world.SetOwner(pt, 1);
    const MapPoint hqPos = world.GetPlayer(0).GetHQPos();
    const MapPoint startPos = world.GetNeighbour(hqPos, Direction::SOUTHEAST);
    const MapPoint endPos = world.MakeMapPoint(startPos + Position(4, 0));
    // Place these env objs
    const std::vector<unsigned> ids{505, 506, 507, 508, 509, 510, 512, 513, 514, 515, 531, 536, 541, 542, 543,
                                    544, 545, 546, 547, 548, 550, 551, 552, 553, 554, 555, 556, 557, 558, 559};
    for(unsigned curId : ids)
    {
        MapPoint curPos = startPos;
        for(unsigned i = 0; i < 4; i++)
        {
            curPos = world.GetNeighbour(curPos, Direction::EAST);
            world.SetNO(curPos, new noEnvObject(curPos, curId));
        }
        world.BuildRoad(0, false, startPos, std::vector<Direction>(4, Direction::EAST));
        // Check road build and objs removed
        curPos = startPos;
        for(unsigned i = 0; i < 3; i++)
        {
            BOOST_REQUIRE_EQUAL(world.GetPointRoad(curPos, Direction::EAST), PointRoad::Normal);
            curPos = world.GetNeighbour(curPos, Direction::EAST);
            BOOST_REQUIRE_EQUAL(world.GetNO(curPos)->GetType(), NOP_NOTHING);
        }
        world.DestroyFlag(endPos, 0);
    }
}

BOOST_AUTO_TEST_SUITE_END()
