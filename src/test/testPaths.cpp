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
#include "WorldFixture.h"
#include "CreateEmptyWorld.h"
#include "nodeObjs/noGranite.h"
#include "gameData/GameConsts.h"
#include "test/PointOutput.h"
#include <boost/test/unit_test.hpp>
#include <boost/foreach.hpp>
#include <vector>

BOOST_AUTO_TEST_SUITE(PathSuite)

typedef WorldFixture<CreateEmptyWorld, 0, 20, 20> WorldFixtureEmpty0P;

void SetRightTerrain(GameWorldGame& world, const MapPoint& pt, Direction dir, TerrainType t)
{
    switch(Direction::Type(dir))
    {
    case Direction::WEST:
        world.GetNodeWriteable(world.GetNeighbour(pt, Direction::NORTHWEST)).t1 = t;
        break;
    case Direction::NORTHWEST:
        world.GetNodeWriteable(world.GetNeighbour(pt, Direction::NORTHWEST)).t2 = t;
        break;
    case Direction::NORTHEAST:
        world.GetNodeWriteable(world.GetNeighbour(pt, Direction::NORTHEAST)).t1 = t;
        break;
    case Direction::EAST:
        world.GetNodeWriteable(pt).t2 = t;
        break;
    case Direction::SOUTHEAST:
        world.GetNodeWriteable(pt).t1 = t;
        break;
    case Direction::SOUTHWEST:
        world.GetNodeWriteable(world.GetNeighbour(pt, Direction::WEST)).t2 = t;
        break;
    }
}

void SetLeftTerrain(GameWorldGame& world, const MapPoint& pt, Direction dir, TerrainType t)
{
    SetRightTerrain(world, pt, Direction(dir.toUInt() + 6 - 1), t);
}

/// Check if we can walk if on of the 2 or both surrounding terrains is changed
boost::test_tools::predicate_result checkWalkOnTerrain(GameWorldGame& world, const MapPoint& startPt, const MapPoint& targetPt,
    Direction dir, TerrainType t, bool isWalkable)
{
    // Block either side
    SetRightTerrain(world, startPt, dir, t);
    if((world.FindHumanPath(startPt, targetPt) != INVALID_DIR) != isWalkable)
    {
        boost::test_tools::predicate_result result(false);
        result.message() << "Failed for right terrain";
        return result;
    }
    if((world.FindHumanPath(targetPt, startPt) != INVALID_DIR) != isWalkable)
    {
        boost::test_tools::predicate_result result(false);
        result.message() << "Failed for right terrain2";
        return result;
    }
    SetRightTerrain(world, startPt, dir, TT_STEPPE);
    SetLeftTerrain(world, startPt, dir, t);
    if((world.FindHumanPath(startPt, targetPt) != INVALID_DIR) != isWalkable)
    {
        boost::test_tools::predicate_result result(false);
        result.message() << "Failed for left terrain";
        return result;
    }
    if((world.FindHumanPath(targetPt, startPt) != INVALID_DIR) != isWalkable)
    {
        boost::test_tools::predicate_result result(false);
        result.message() << "Failed for left terrain2";
        return result;
    }
    SetRightTerrain(world, startPt, dir, t);
    if((world.FindHumanPath(startPt, targetPt) != INVALID_DIR) != isWalkable)
    {
        boost::test_tools::predicate_result result(false);
        result.message() << "Failed for both terrain";
        return result;
    }
    if((world.FindHumanPath(targetPt, startPt) != INVALID_DIR) != isWalkable)
    {
        boost::test_tools::predicate_result result(false);
        result.message() << "Failed for both terrain2";
        return result;
    }
    SetRightTerrain(world, startPt, dir, TT_STEPPE);
    SetLeftTerrain(world, startPt, dir, TT_STEPPE);
    return true;
}

/// Check if we can walk on the point if it is completely covered in terrain
boost::test_tools::predicate_result checkWalkOnPoint(GameWorldGame& world, const MapPoint& startPt, const MapPoint& targetPt,
    Direction dir, TerrainType t)
{
    // Block whole point
    const MapPoint nextPt = world.GetNeighbour(startPt, dir);
    for(unsigned i=0; i<6; i++)
        SetRightTerrain(world, nextPt, Direction::fromInt(i), t);
    if(world.FindHumanPath(startPt, targetPt) != INVALID_DIR)
    {
        boost::test_tools::predicate_result result(false);
        result.message() << "Failed for fwd direction";
        return result;
    }
    if(world.FindHumanPath(targetPt, startPt) != INVALID_DIR)
    {
        boost::test_tools::predicate_result result(false);
        result.message() << "Failed for bwd direction";
        return result;
    }
    for(unsigned i = 0; i < 6; i++)
        SetRightTerrain(world, nextPt, Direction::fromInt(i), TT_STEPPE);
    return true;
}

BOOST_FIXTURE_TEST_CASE(BlockedPaths, WorldFixtureEmpty0P)
{
    MapPoint startPt(10, 10);
    // Create a circle of stones so the path is completely blocked
    std::vector<MapPoint> surroundingPts = world.GetPointsInRadius(startPt, 1);
    BOOST_FOREACH(const MapPoint& pt, surroundingPts)
        world.SetNO(pt, new noGranite(GT_1, 1));
    std::vector<MapPoint> surroundingPts2;
    for(unsigned i=0; i<12; i++)
        surroundingPts2.push_back(world.GetNeighbour2(startPt, i));
    BOOST_FOREACH(const MapPoint& pt, surroundingPts2)
        BOOST_REQUIRE_EQUAL(world.FindHumanPath(startPt, pt), INVALID_DIR);
    // Allow left exit
    world.DestroyNO(surroundingPts[0]);
    BOOST_REQUIRE_EQUAL(world.FindHumanPath(startPt, surroundingPts2[0]), 0);
    BOOST_REQUIRE(checkWalkOnTerrain(world, startPt, surroundingPts2[0], Direction::WEST, TT_WATER, true));
    BOOST_REQUIRE(checkWalkOnTerrain(world, startPt, surroundingPts2[0], Direction::WEST, TT_SWAMPLAND, true));
    BOOST_REQUIRE(checkWalkOnTerrain(world, startPt, surroundingPts2[0], Direction::WEST, TT_LAVA, false));
    BOOST_REQUIRE(checkWalkOnTerrain(world, startPt, surroundingPts2[0], Direction::WEST, TT_SNOW, false));

    BOOST_REQUIRE(checkWalkOnPoint(world, startPt, surroundingPts2[0], Direction::WEST, TT_WATER));
    BOOST_REQUIRE(checkWalkOnPoint(world, startPt, surroundingPts2[0], Direction::WEST, TT_SWAMPLAND));
}

BOOST_AUTO_TEST_SUITE_END()
