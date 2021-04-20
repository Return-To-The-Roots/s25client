// Copyright (c) 2016 - 2020 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

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

// LCOV_EXCL_START
static std::ostream& operator<<(std::ostream& out, const NodalObjectType& e)
{
    return out << static_cast<unsigned>(rttr::enum_cast(e));
}
static std::ostream& operator<<(std::ostream& out, const RoadBuildMode& e)
{
    return out << static_cast<unsigned>(rttr::enum_cast(e));
}
// LCOV_EXCL_STOP

// Test stuff related to building/building quality
BOOST_AUTO_TEST_SUITE(BuildingSuite)

namespace {
using EmptyWorldFixture0P = WorldFixture<CreateEmptyWorld, 0>;
using EmptyWorldFixture1P = WorldFixture<CreateEmptyWorld, 1>;
using EmptyWorldFixture1PBigger = WorldFixture<CreateEmptyWorld, 1, 18, 16>;
using EmptyWorldFixture1PBiggest = WorldFixture<CreateEmptyWorld, 1, 22, 20>;
using ReducedBQMap = std::map<MapPoint, BuildingQuality, MapPointLess>;

/// Check that the BQ at all points is BuildingQuality::Castle except the points in the reducedBQs map which have given
/// BQs
boost::test_tools::predicate_result checkBQs(const GameWorldBase& world, const std::vector<MapPoint>& pts,
                                             const ReducedBQMap& reducedBQs)
{
    for(MapPoint pt : pts)
    {
        BuildingQuality bqReq;
        if(helpers::contains(reducedBQs, pt))
            bqReq = reducedBQs.find(pt)->second; //-V783
        else
            bqReq = BuildingQuality::Castle;
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

    world.GetNodeWriteable(flagPos).bq = BuildingQuality::Flag;
    BOOST_TEST(checkBQs(world, pts, reducedBQs).message().str()
               == "BuildingQuality::Flag!=BuildingQuality::Castle at " + s.str());
    reducedBQs[flagPos] = BuildingQuality::Flag;
    BOOST_TEST(checkBQs(world, pts, reducedBQs));
    world.GetNodeWriteable(flagPos).bq = BuildingQuality::Castle;
    BOOST_TEST(checkBQs(world, pts, reducedBQs).message().str()
               == "BuildingQuality::Castle!=BuildingQuality::Flag at " + s.str());
}

BOOST_FIXTURE_TEST_CASE(BQNextToBuilding, EmptyWorldFixture1P)
{
    const MapPoint flagPos = world.MakeMapPoint(world.GetPlayer(0).GetHQPos() - Position(5, 6));
    const MapPoint bldPos = world.GetNeighbour(flagPos, Direction::NorthWest);
    // All points possibly affected by a building
    const std::vector<MapPoint> pts = world.GetPointsInRadiusWithCenter(bldPos, 4);
    const std::vector<MapPoint> radius1Pts = world.GetPointsInRadius(bldPos, 1);
    ReducedBQMap reducedBQs;
    // Initially all are full
    BOOST_TEST_REQUIRE(checkBQs(world, pts, ReducedBQMap()));

    // Place flag
    world.SetFlag(flagPos, 0);
    reducedBQs.clear();
    reducedBQs[flagPos] = BuildingQuality::Nothing;
    // Flag to any building would be to close and flag also to close (min diff of 2 required)
    reducedBQs[world.GetNeighbour(flagPos, Direction::West)] = BuildingQuality::Nothing;
    reducedBQs[world.GetNeighbour(flagPos, Direction::NorthEast)] = BuildingQuality::Nothing;
    // Can build houses but not castles
    for(const Direction dir : {Direction::East, Direction::SouthEast, Direction::SouthWest})
        reducedBQs[world.GetNeighbour(flagPos, dir)] = BuildingQuality::House;
    // Flag to bld is blocked by flag
    for(const Direction dir : {Direction::West, Direction::NorthWest, Direction::NorthEast})
        reducedBQs[world.GetNeighbour(bldPos, dir)] = BuildingQuality::Flag;
    BOOST_TEST_REQUIRE(checkBQs(world, pts, reducedBQs));
    BOOST_TEST_REQUIRE(!world.IsRoadAvailable(false, flagPos));

    // Remove flag -> BQ reset
    world.DestroyFlag(flagPos, 0);
    BOOST_TEST_REQUIRE(checkBQs(world, pts, ReducedBQMap()));

    // Place hut -> Addionally to changes by flag we can't place blds in range 1
    // and no large blds in range 2
    world.SetBuildingSite(BuildingType::Woodcutter, bldPos, 0);
    BOOST_TEST_REQUIRE(world.GetSpecObj<noBaseBuilding>(bldPos)->GetSize() == BuildingQuality::Hut); //-V807
    reducedBQs[bldPos] = BuildingQuality::Nothing;
    // Every bq with distance==2 has BuildingQuality::House
    for(unsigned i = 0; i < 12; i++)
        reducedBQs[world.GetNeighbour2(bldPos, i)] = BuildingQuality::House;
    BOOST_TEST_REQUIRE(checkBQs(world, pts, reducedBQs));
    BOOST_TEST_REQUIRE(!world.IsRoadAvailable(false, flagPos));
    BOOST_TEST_REQUIRE(!world.IsRoadAvailable(false, bldPos));
    for(MapPoint pt : radius1Pts)
        BOOST_TEST_REQUIRE((pt == flagPos || world.IsRoadAvailable(false, pt)));

    // Remove -> BQ reset
    world.DestroyFlag(flagPos, 0);
    BOOST_TEST_REQUIRE(checkBQs(world, pts, ReducedBQMap()));

    // Place house -> Same as hut
    world.SetBuildingSite(BuildingType::Sawmill, bldPos, 0);
    BOOST_TEST_REQUIRE(world.GetSpecObj<noBaseBuilding>(bldPos)->GetSize() == BuildingQuality::House);
    BOOST_TEST_REQUIRE(checkBQs(world, pts, reducedBQs));
    BOOST_TEST_REQUIRE(!world.IsRoadAvailable(false, flagPos));
    BOOST_TEST_REQUIRE(!world.IsRoadAvailable(false, bldPos));
    for(MapPoint pt : radius1Pts)
        BOOST_TEST_REQUIRE((pt == flagPos || world.IsRoadAvailable(false, pt)));

    // Remove -> BQ reset
    world.DestroyFlag(flagPos, 0);
    BOOST_TEST_REQUIRE(checkBQs(world, pts, ReducedBQMap()));

    // Place castle
    world.SetBuildingSite(BuildingType::Fortress, bldPos, 0);
    BOOST_TEST_REQUIRE(world.GetSpecObj<noBaseBuilding>(bldPos)->GetSize() == BuildingQuality::Castle);
    // Addionally to reduced BQs by hut:
    // Even flag is blocked by castle (model size)
    for(const Direction dir : {Direction::West, Direction::NorthWest, Direction::NorthEast})
    {
        const MapPoint wouldBeFlagPt = world.GetNeighbour(bldPos, dir);
        reducedBQs[wouldBeFlagPt] = BuildingQuality::Nothing;
        // And therefore also the bld
        reducedBQs[world.GetNeighbour(wouldBeFlagPt, Direction::NorthWest)] = BuildingQuality::Flag;
    }
    BOOST_TEST_REQUIRE(checkBQs(world, pts, reducedBQs));
    BOOST_TEST_REQUIRE(!world.IsRoadAvailable(false, flagPos));
    BOOST_TEST_REQUIRE(!world.IsRoadAvailable(false, bldPos));
    // No roads over attachment
    BOOST_TEST_REQUIRE(!world.IsRoadAvailable(false, world.GetNeighbour(bldPos, Direction::West)));
    BOOST_TEST_REQUIRE(!world.IsRoadAvailable(false, world.GetNeighbour(bldPos, Direction::NorthWest)));
    BOOST_TEST_REQUIRE(!world.IsRoadAvailable(false, world.GetNeighbour(bldPos, Direction::NorthEast)));
    // But other 2 points are ok
    BOOST_TEST_REQUIRE(world.IsRoadAvailable(false, world.GetNeighbour(bldPos, Direction::SouthWest)));
    BOOST_TEST_REQUIRE(world.IsRoadAvailable(false, world.GetNeighbour(bldPos, Direction::East)));

    // Remove -> BQ reset
    world.DestroyFlag(flagPos, 0);
    BOOST_TEST_REQUIRE(checkBQs(world, pts, ReducedBQMap()));
}

BOOST_FIXTURE_TEST_CASE(BQWithRoad, EmptyWorldFixture0P)
{
    RTTR_FOREACH_PT(MapPoint, world.GetSize())
    {
        BuildingQuality bq = world.GetNode(pt).bq;
        BOOST_TEST_INFO(" at " << pt);
        BOOST_TEST_REQUIRE(bq == BuildingQuality::Castle);
    }
    // Create a road of length 6
    std::vector<MapPoint> roadPts;
    MapPoint curPt(2, 3);
    for(unsigned i = 0; i < 6; i++)
    {
        roadPts.push_back(curPt);
        world.SetPointRoad(curPt, Direction::SouthEast, PointRoad::Normal);
        world.RecalcBQForRoad(curPt);
        curPt = world.GetNeighbour(curPt, Direction::SouthEast);
    }
    // Final pt still belongs to road
    roadPts.push_back(curPt);
    // Normally this is done by the flag
    world.RecalcBQForRoad(curPt);

    for(MapPoint pt : roadPts)
    {
        BOOST_TEST_REQUIRE(world.IsOnRoad(pt));
        // On the road we only allow flags
        BOOST_TEST_REQUIRE(world.GetNode(pt).bq == BuildingQuality::Flag);
        // Next to the road should be houses
        // But left to first point is still a castle
        BuildingQuality leftBQ = (pt == roadPts[0]) ? BuildingQuality::Castle : BuildingQuality::House;
        BOOST_TEST_REQUIRE(world.GetNode(pt - MapPoint(1, 0)).bq == leftBQ);
        BOOST_TEST_REQUIRE(world.GetNode(pt + MapPoint(1, 0)).bq == BuildingQuality::House);
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
        BOOST_TEST_CONTEXT(" at " << pt)
        {
            BuildingQuality bq = world.GetNode(pt).bq;
            BOOST_TEST_REQUIRE(bq == BuildingQuality::Castle);
            bq = gwv.GetBQ(pt);
            BOOST_TEST_REQUIRE(bq == BuildingQuality::Castle);
        }
    gameDesktop.GI_StartRoadBuilding(roadPt, false);
    BOOST_TEST_REQUIRE(gameDesktop.GetRoadMode() == RoadBuildMode::Normal);

    std::vector<MapPoint> roadPts;
    MapPoint curPt = roadPt;
    for(unsigned i = 0; i < 6; i++)
    {
        roadPts.push_back(curPt);
        curPt = world.GetNeighbour(curPt, Direction::SouthEast);
    }
    // Final pt still belongs to road
    roadPts.push_back(curPt);

    MapPoint curRoadEndPt = curPt;
    gameDesktop.BuildRoadPart(curRoadEndPt);
    BOOST_TEST_REQUIRE(curRoadEndPt == curPt);

    for(MapPoint pt : roadPts)
    {
        BOOST_TEST_REQUIRE(gwv.IsOnRoad(pt));
        // On the road we only allow flags
        BOOST_TEST_REQUIRE(gwv.GetBQ(pt) == BuildingQuality::Flag);
        // Next to the road should be houses
        // But left to first point is still a castle
        BuildingQuality leftBQ = (pt == roadPts[0]) ? BuildingQuality::Castle : BuildingQuality::House;
        BOOST_TEST_REQUIRE(gwv.GetBQ(world.GetNeighbour(pt, Direction::West)) == leftBQ);
        BOOST_TEST_REQUIRE(gwv.GetBQ(world.GetNeighbour(pt, Direction::East)) == BuildingQuality::House);
    }
    // Destroy road partially
    // The first point (after start) must have ID 2
    BOOST_TEST_REQUIRE(gameDesktop.GetIdInCurBuildRoad(roadPts[1]) == 2u);
    gameDesktop.DemolishRoad(2);
    BOOST_TEST_REQUIRE(gwv.IsOnRoad(roadPts[0]));
    BOOST_TEST_REQUIRE(gwv.IsOnRoad(roadPts[1]));
    for(unsigned i = 2; i < roadPts.size(); i++)
        BOOST_TEST_REQUIRE(!gwv.IsOnRoad(roadPts[i]));
    // Remove rest
    gameDesktop.GI_CancelRoadBuilding();
    BOOST_TEST_REQUIRE(!gwv.IsOnRoad(roadPts[0]));
    BOOST_TEST_REQUIRE(!gwv.IsOnRoad(roadPts[1]));
    // BQ should be restored
    for(const MapPoint& pt : roadRadiusPts)
    {
        BuildingQuality bq = gwv.GetBQ(pt);
        BOOST_TEST_INFO(" at " << pt);
        BOOST_TEST_REQUIRE(bq == BuildingQuality::Castle);
    }

    BOOST_TEST_REQUIRE(gameDesktop.GetRoadMode() == RoadBuildMode::Disabled);
    gameDesktop.GI_StartRoadBuilding(roadPt, false);
    BOOST_TEST_REQUIRE(gameDesktop.GetRoadMode() == RoadBuildMode::Normal);
    // Build horizontal road
    roadPts.clear();
    curPt = roadPt;
    for(unsigned i = 0; i < 6; i++)
    {
        roadPts.push_back(curPt);
        curPt = world.GetNeighbour(curPt, Direction::East);
    }
    // Final pt still belongs to road
    roadPts.push_back(curPt);

    curRoadEndPt = curPt;
    gameDesktop.BuildRoadPart(curRoadEndPt);
    BOOST_TEST_REQUIRE(curRoadEndPt == curPt);

    for(MapPoint pt : roadPts)
    {
        BOOST_TEST_REQUIRE(gwv.IsOnRoad(pt));
        // On the road we only allow flags
        BOOST_TEST_REQUIRE(gwv.GetBQ(pt) == BuildingQuality::Flag);
        // Above should be castles
        BOOST_TEST_REQUIRE(gwv.GetBQ(world.GetNeighbour(pt, Direction::NorthWest)) == BuildingQuality::Castle);
        BOOST_TEST_REQUIRE(gwv.GetBQ(world.GetNeighbour(pt, Direction::NorthEast)) == BuildingQuality::Castle);
        // Below should be big houses
        BOOST_TEST_REQUIRE(gwv.GetBQ(world.GetNeighbour(pt, Direction::SouthWest)) == BuildingQuality::House);
        BOOST_TEST_REQUIRE(gwv.GetBQ(world.GetNeighbour(pt, Direction::SouthEast)) == BuildingQuality::House);
    }

    // Destroy road
    gameDesktop.GI_CancelRoadBuilding();
    for(MapPoint pt : roadPts)
        BOOST_TEST_REQUIRE(!gwv.IsOnRoad(pt));
    // BQ should be restored
    for(const MapPoint& pt : roadRadiusPts)
    {
        BuildingQuality bq = gwv.GetBQ(pt);
        BOOST_TEST_INFO(" at " << pt);
        BOOST_TEST_REQUIRE(bq == BuildingQuality::Castle);
    }
}

BOOST_FIXTURE_TEST_CASE(BQ_AtBorder, EmptyWorldFixture1PBiggest)
{
    GamePlayer& player = world.GetPlayer(0);
    const MapPoint hqPos = player.GetHQPos();
    const nobBaseMilitary* hq = world.GetSpecObj<nobBaseMilitary>(hqPos);
    BOOST_TEST_REQUIRE(hq);
    const unsigned hqRadius = hq->GetMilitaryRadius();
    BOOST_TEST_REQUIRE(hqRadius > 4u);
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
            BOOST_TEST_REQUIRE(world.IsPlayerTerritory(pt));
            BOOST_TEST_REQUIRE(world.GetBQ(pt, 0) == BuildingQuality::Castle);
        } else if(distance < hqRadius)
        {
            // Near border, flag if we cant build a buildings flag
            BOOST_TEST_REQUIRE(world.IsPlayerTerritory(pt));
            if(!world.IsPlayerTerritory(world.GetNeighbour(pt, Direction::SouthEast)))
                BOOST_TEST_REQUIRE(world.GetBQ(pt, 0) == BuildingQuality::Flag);
            else
                BOOST_TEST_REQUIRE(world.GetBQ(pt, 0) == BuildingQuality::Castle);
        } else if(distance == hqRadius)
        {
            // At border
            BOOST_TEST_REQUIRE(!world.IsPlayerTerritory(pt));
            BOOST_TEST_REQUIRE(world.GetNode(pt).owner == 0u + 1u);
            BOOST_TEST_REQUIRE(world.GetBQ(pt, 0) == BuildingQuality::Nothing);
        } else
        {
            // Outside border
            BOOST_TEST_REQUIRE(!world.IsPlayerTerritory(pt));
            BOOST_TEST_REQUIRE(world.GetNode(pt).owner == 0u);
            BOOST_TEST_REQUIRE(world.GetBQ(pt, 0) == BuildingQuality::Nothing);
        }
    }

    // Place a flag near a border
    MapPoint flagPt = hqPos;
    for(unsigned i = 0; i < hqRadius - 2; i++)
        flagPt = world.GetNeighbour(flagPt, i % 2 ? Direction::SouthEast : Direction::SouthWest);
    // This is near the border, so only a flag is possible
    BOOST_TEST_REQUIRE(world.GetBQ(world.GetNeighbour(flagPt, Direction::SouthEast), 0) == BuildingQuality::Flag);
    world.SetFlag(flagPt, 0);
    BOOST_TEST_REQUIRE(world.GetNO(flagPt)->GetGOT() == GO_Type::Flag);
    BOOST_TEST_REQUIRE(world.GetBQ(world.GetNeighbour(flagPt, Direction::West), 0)
                       == BuildingQuality::Nothing); // Buildings flag or flag to close to this
    BOOST_TEST_REQUIRE(world.GetBQ(world.GetNeighbour(flagPt, Direction::NorthWest), 0)
                       == BuildingQuality::Castle); // Building to this flag
    BOOST_TEST_REQUIRE(world.GetBQ(world.GetNeighbour(flagPt, Direction::NorthEast), 0)
                       == BuildingQuality::Nothing); // Buildings flag or flag to close to this
    BOOST_TEST_REQUIRE(world.GetBQ(world.GetNeighbour(flagPt, Direction::East), 0)
                       == BuildingQuality::House); // This flag blocks castles extensions
                                                   // This flag is to close and border prohibits building
    BOOST_TEST_REQUIRE(world.GetBQ(world.GetNeighbour(flagPt, Direction::SouthEast), 0) == BuildingQuality::Nothing);
    BOOST_TEST_REQUIRE(world.GetBQ(world.GetNeighbour(flagPt, Direction::SouthWest), 0) == BuildingQuality::Nothing);
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
    BOOST_TEST_REQUIRE(checkBQs(world, ptsAroundObj, ReducedBQMap()));
    ReducedBQMap reducedBQs;

    // Size=0 -> Block Nothing
    addStaticObj(world, objPos, 0);
    BOOST_TEST_REQUIRE(checkBQs(world, ptsAroundObj, ReducedBQMap()));
    BOOST_TEST_REQUIRE(world.IsRoadAvailable(false, objPos));
    for(MapPoint pt : radius1Pts)
        BOOST_TEST_REQUIRE(world.IsRoadAvailable(false, pt));

    // Size=1 -> Block only point
    addStaticObj(world, objPos, 1);
    reducedBQs[objPos] = BuildingQuality::Nothing;
    reducedBQs[world.GetNeighbour(objPos, Direction::NorthWest)] = BuildingQuality::Flag;
    // Can build houses but not castles
    for(const Direction dir : {Direction::East, Direction::SouthEast, Direction::SouthWest})
        reducedBQs[world.GetNeighbour(objPos, dir)] = BuildingQuality::House;
    BOOST_TEST_REQUIRE(checkBQs(world, ptsAroundObj, reducedBQs));
    BOOST_TEST_REQUIRE(!world.IsRoadAvailable(false, objPos));
    for(MapPoint pt : radius1Pts)
        BOOST_TEST_REQUIRE(world.IsRoadAvailable(false, pt));

    // Size=2 -> Block like a castle
    addStaticObj(world, objPos, 2);
    // Addionally reduced BQs:
    // Extensions block spots
    for(const Direction dir : {Direction::West, Direction::NorthWest, Direction::NorthEast})
    {
        const MapPoint extensionPos = world.GetNeighbour(objPos, dir);
        reducedBQs[extensionPos] = BuildingQuality::Nothing;
        // And therefore also the bld
        reducedBQs[world.GetNeighbour(extensionPos, Direction::NorthWest)] = BuildingQuality::Flag;
    }
    // Also blocked by extensions:
    reducedBQs[world.GetNeighbour(world.GetNeighbour(objPos, Direction::West), Direction::SouthWest)] =
      BuildingQuality::House;
    reducedBQs[world.GetNeighbour(world.GetNeighbour(objPos, Direction::NorthEast), Direction::East)] =
      BuildingQuality::House;
    BOOST_TEST_REQUIRE(checkBQs(world, ptsAroundObj, reducedBQs));
    BOOST_TEST_REQUIRE(!world.IsRoadAvailable(false, objPos));
    // No roads over attachment
    BOOST_TEST_REQUIRE(!world.IsRoadAvailable(false, world.GetNeighbour(objPos, Direction::West)));
    BOOST_TEST_REQUIRE(!world.IsRoadAvailable(false, world.GetNeighbour(objPos, Direction::NorthWest)));
    BOOST_TEST_REQUIRE(!world.IsRoadAvailable(false, world.GetNeighbour(objPos, Direction::NorthEast)));
    // But other 3 points are ok
    BOOST_TEST_REQUIRE(world.IsRoadAvailable(false, world.GetNeighbour(objPos, Direction::East)));
    BOOST_TEST_REQUIRE(world.IsRoadAvailable(false, world.GetNeighbour(objPos, Direction::SouthEast)));
    BOOST_TEST_REQUIRE(world.IsRoadAvailable(false, world.GetNeighbour(objPos, Direction::SouthWest)));
}

BOOST_FIXTURE_TEST_CASE(RoadRemovesObjs, EmptyWorldFixture1P)
{
    RTTR_FOREACH_PT(MapPoint, world.GetSize())
        world.SetOwner(pt, 1);
    const MapPoint hqPos = world.GetPlayer(0).GetHQPos();
    const MapPoint startPos = world.GetNeighbour(hqPos, Direction::SouthEast);
    const MapPoint endPos = world.MakeMapPoint(startPos + Position(4, 0));
    // Place these env objs
    const std::vector<unsigned> ids{505, 506, 507, 508, 509, 510, 512, 513, 514, 515, 531, 536, 541, 542, 543,
                                    544, 545, 546, 547, 548, 550, 551, 552, 553, 554, 555, 556, 557, 558, 559};
    for(unsigned curId : ids)
    {
        MapPoint curPos = startPos;
        for(unsigned i = 0; i < 4; i++)
        {
            curPos = world.GetNeighbour(curPos, Direction::East);
            world.SetNO(curPos, new noEnvObject(curPos, curId));
        }
        world.BuildRoad(0, false, startPos, std::vector<Direction>(4, Direction::East));
        // Check road build and objs removed
        curPos = startPos;
        for(unsigned i = 0; i < 3; i++)
        {
            BOOST_TEST_REQUIRE(world.GetPointRoad(curPos, Direction::East) == PointRoad::Normal);
            curPos = world.GetNeighbour(curPos, Direction::East);
            BOOST_TEST_REQUIRE(world.GetNO(curPos)->GetType() == NodalObjectType::Nothing);
        }
        world.DestroyFlag(endPos, 0);
    }
}

BOOST_AUTO_TEST_SUITE_END()
