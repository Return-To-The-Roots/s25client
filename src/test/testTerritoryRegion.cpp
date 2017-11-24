// Copyright (c) 2016 - 2017 Settlers Freaks (sf-team at siedler25.org)
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

#include "rttrDefines.h" // IWYU pragma: keep
#include "CreateEmptyWorld.h"
#include "GamePlayer.h"
#include "PointOutput.h"
#include "WorldFixture.h"
#include "factories/BuildingFactory.h"
#include "figures/nofPassiveSoldier.h"
#include "helperFuncs.h"
#include "helpers/containerUtils.h"
#include "helpers/setTraits.h"
#include "world/GameWorld.h"
#include "world/TerritoryRegion.h"
#include <boost/array.hpp>
#include <boost/assign/std/set.hpp>
#include <boost/assign/std/vector.hpp>
#include <boost/bind.hpp>
#include <boost/foreach.hpp>
#include <boost/range/algorithm_ext/push_back.hpp>
#include <boost/test/unit_test.hpp>
#include <iostream>
#include <string>
#include <vector>

using namespace boost::assign;

struct MapPointComp
{
    bool operator()(const MapPoint& lhs, const MapPoint& rhs) const { return (lhs.y < rhs.y) || ((lhs.y == rhs.y) && (lhs.x < rhs.x)); }
};

BOOST_AUTO_TEST_SUITE(TerritoryRegionTestSuite)

BOOST_AUTO_TEST_CASE(IsPointValid)
{
    // Max tested point is (20, 20) -> size > 20
    MapExtent worldSize(24, 22);
    std::vector<MapPoint> hole, hole_reversed;
    std::vector<MapPoint> outer, outer_reversed;

    // Hole
    hole += MapPoint(14, 14), MapPoint(16, 14), MapPoint(16, 16), MapPoint(14, 16), MapPoint(14, 14);

    // Reverse it...
    hole_reversed = hole;
    std::reverse(hole_reversed.begin(), hole_reversed.end());

    // Outer polygon
    outer += MapPoint(10, 10), MapPoint(20, 10), MapPoint(20, 20), MapPoint(10, 20), MapPoint(10, 10);

    // Reverse it...
    outer_reversed = outer;
    std::reverse(outer_reversed.begin(), outer_reversed.end());

    // Set of MapPoints that should return true
    std::set<MapPoint, MapPointComp> results;

    // Auto-generated data (from different implementation with tests)
    results += MapPoint(10, 10), MapPoint(10, 11), MapPoint(10, 12), MapPoint(10, 13), MapPoint(10, 14);
    results += MapPoint(10, 15), MapPoint(10, 16), MapPoint(10, 17), MapPoint(10, 18), MapPoint(10, 19);
    results += MapPoint(11, 10), MapPoint(11, 11), MapPoint(11, 12), MapPoint(11, 13), MapPoint(11, 14);
    results += MapPoint(11, 15), MapPoint(11, 16), MapPoint(11, 17), MapPoint(11, 18), MapPoint(11, 19);
    results += MapPoint(12, 10), MapPoint(12, 11), MapPoint(12, 12), MapPoint(12, 13), MapPoint(12, 14);
    results += MapPoint(12, 15), MapPoint(12, 16), MapPoint(12, 17), MapPoint(12, 18), MapPoint(12, 19);
    results += MapPoint(13, 10), MapPoint(13, 11), MapPoint(13, 12), MapPoint(13, 13), MapPoint(13, 14);
    results += MapPoint(13, 15), MapPoint(13, 16), MapPoint(13, 17), MapPoint(13, 18), MapPoint(13, 19);
    results += MapPoint(14, 10), MapPoint(14, 11), MapPoint(14, 12), MapPoint(14, 13), MapPoint(14, 16);
    results += MapPoint(14, 17), MapPoint(14, 18), MapPoint(14, 19), MapPoint(15, 10), MapPoint(15, 11);
    results += MapPoint(15, 12), MapPoint(15, 13), MapPoint(15, 16), MapPoint(15, 17), MapPoint(15, 18);
    results += MapPoint(15, 19), MapPoint(16, 10), MapPoint(16, 11), MapPoint(16, 12), MapPoint(16, 13);
    results += MapPoint(16, 14), MapPoint(16, 15), MapPoint(16, 16), MapPoint(16, 17), MapPoint(16, 18);
    results += MapPoint(16, 19), MapPoint(17, 10), MapPoint(17, 11), MapPoint(17, 12), MapPoint(17, 13);
    results += MapPoint(17, 14), MapPoint(17, 15), MapPoint(17, 16), MapPoint(17, 17), MapPoint(17, 18);
    results += MapPoint(17, 19), MapPoint(18, 10), MapPoint(18, 11), MapPoint(18, 12), MapPoint(18, 13);
    results += MapPoint(18, 14), MapPoint(18, 15), MapPoint(18, 16), MapPoint(18, 17), MapPoint(18, 18);
    results += MapPoint(18, 19), MapPoint(19, 10), MapPoint(19, 11), MapPoint(19, 12), MapPoint(19, 13);
    results += MapPoint(19, 14), MapPoint(19, 15), MapPoint(19, 16), MapPoint(19, 17), MapPoint(19, 18);
    results += MapPoint(19, 19);

    // check the whole area
    std::vector<MapPoint> polygon[8];

    // Generate polygons for all eight cases of ordering
    for(int i = 0; i < 8; ++i)
    {
        // i = 1, 3, 5, 7 -> hole, then outer
        // i = 2, 3, 6, 7 -> hole reversed
        // i = 4, 5, 6, 7 -> outer reversed

        polygon[i] += MapPoint(0, 0);
        polygon[i] = boost::push_back(polygon[i],
                                      (i & (1 << 0)) ? ((i & (1 << 1)) ? hole : hole_reversed) : ((i & (1 << 2)) ? outer : outer_reversed));
        polygon[i] += MapPoint(0, 0);
        polygon[i] = boost::push_back(polygon[i],
                                      (i & (1 << 0)) ? ((i & (1 << 2)) ? outer : outer_reversed) : ((i & (1 << 1)) ? hole : hole_reversed));
        polygon[i] += MapPoint(0, 0);
    }

    for(int i = 0; i < 8; ++i)
    {
        // check the whole area
        RTTR_FOREACH_PT(MapPoint, worldSize)
        {
            // Result for this particular point
            bool result = helpers::contains(results, pt);
            BOOST_REQUIRE_EQUAL(TerritoryRegion::IsPointValid(worldSize, polygon[i], pt), result);
        }
    }

    // Consistency checks

    // Make a rectangle (10,5)->(15,10) and 2 parallelograms below
    boost::array<std::vector<MapPoint>, 4> rectAreas;
    rectAreas[0] += MapPoint(10, 5), MapPoint(15, 5), MapPoint(15, 10);
    rectAreas[0] += MapPoint(18, 13), MapPoint(15, 15);
    rectAreas[0] += MapPoint(10, 15);
    rectAreas[0] += MapPoint(13, 13), MapPoint(10, 10);
    // With duplicate start point
    rectAreas[1].clear();
    boost::push_back(rectAreas[1], rectAreas[0]) += rectAreas[0][0];
    rectAreas[2].resize(rectAreas[0].size());
    std::reverse_copy(rectAreas[0].begin(), rectAreas[0].end(), rectAreas[2].begin());
    rectAreas[3].resize(rectAreas[1].size());
    std::reverse_copy(rectAreas[1].begin(), rectAreas[1].end(), rectAreas[3].begin());

    results.clear();
    // All points inside must be inside
    for(unsigned i = 0; i < rectAreas.size(); i++)
    {
        for(int x = 11; x < 15; ++x)
        {
            for(int y = 6; y < 15; ++y)
            {
                MapPoint pt(x, y);
                if(y > 10 && y < 13)
                    pt.x += y - 10;
                else if(y >= 13)
                    pt.x += 15 - y;
                BOOST_REQUIRE(TerritoryRegion::IsPointValid(worldSize, rectAreas[i], pt));
                if(i == 0)
                    results.insert(pt);
            }
        }
    }
    // Get the points exactly at the border and just outside of it
    std::vector<MapPoint> borderPts;
    std::vector<MapPoint> outsidePts;
    outsidePts += MapPoint(9, 4), MapPoint(16, 4), MapPoint(16, 16), MapPoint(9, 16);
    outsidePts += MapPoint(10, 12), MapPoint(10, 13), MapPoint(11, 13), MapPoint(10, 14);
    for(int x = 10; x <= 15; x++)
    {
        borderPts += MapPoint(x, 5), MapPoint(x, 15);
        outsidePts += MapPoint(x, 5 - 1), MapPoint(x, 15 + 1);
    }
    for(int y = 5; y <= 10; y++)
    {
        borderPts += MapPoint(10, y), MapPoint(15, y);
        outsidePts += MapPoint(10 - 1, y), MapPoint(15 + 1, y);
    }
    // Border at the parallelogram (not really all border, but hard to figure out which are outside
    for(int y = 11; y < 15; y++)
    {
        for(int x = 0; x <= 3; x++)
            borderPts += MapPoint(x + 11, y), MapPoint(x + 16, y);
    }
    for(unsigned i = 0; i < rectAreas.size(); i++)
    {
        // Those must be outside
        BOOST_FOREACH(MapPoint pt, outsidePts)
        {
            BOOST_REQUIRE(!TerritoryRegion::IsPointValid(worldSize, rectAreas[i], pt));
        }
    }
    // Border points are unspecified, but must be consistently either inside or outside
    BOOST_FOREACH(MapPoint pt, borderPts)
    {
        const bool isValid = TerritoryRegion::IsPointValid(worldSize, rectAreas[0], pt);
        if(isValid)
            results.insert(pt);
        for(unsigned i = 1; i < rectAreas.size(); i++)
            BOOST_REQUIRE_EQUAL(TerritoryRegion::IsPointValid(worldSize, rectAreas[i], pt), isValid);
    }

    std::vector<MapPoint> fullMapArea;
    // Note the usage of width and height to include the border points
    fullMapArea += MapPoint(0, 0), MapPoint(0, worldSize.y), MapPoint(worldSize), MapPoint(worldSize.x, 0), MapPoint(0, 0);
    std::vector<MapPoint> fullMapAreaReversed(fullMapArea.size());
    std::reverse_copy(fullMapArea.begin(), fullMapArea.end(), fullMapAreaReversed.begin());

    for(unsigned i = 0; i < 4; i++)
    {
        std::vector<MapPoint> fullArea;
        fullArea += MapPoint(0, 0);
        if(i < 2)
            boost::push_back(fullArea, fullMapArea);
        else
            boost::push_back(fullArea, fullMapAreaReversed);
        fullArea += MapPoint(0, 0);
        if(i % 2 == 0)
            boost::push_back(fullArea, rectAreas[1]);
        else
            boost::push_back(fullArea, rectAreas[3]);
        fullArea += MapPoint(0, 0);

        // check the whole area
        RTTR_FOREACH_PT(MapPoint, worldSize)
        {
            // If the point is in the set, it is in the small rect and should not be in the big rect with this as a hole
            const bool result = !helpers::contains(results, pt);
            const bool isValid = TerritoryRegion::IsPointValid(worldSize, fullArea, pt);
            BOOST_REQUIRE_MESSAGE(isValid == result, isValid << "!=" << result << " at " << pt << " (iteration " << i << ")");
        }
    }
}

// HQ radius = 9, HQs 2 + 5 + 6 = 13 fields apart
typedef WorldFixture<CreateEmptyWorld, 2, 26, 10> WorldFixtureEmpty2P;

BOOST_FIXTURE_TEST_CASE(CreateTerritoryRegion, WorldFixtureEmpty2P)
{
    boost::array<MapPoint, 3> milBldPos;
    milBldPos[0] = world.MakeMapPoint(world.GetPlayer(0).GetHQPos() + Position(2, 0));
    milBldPos[1] = world.MakeMapPoint(milBldPos[0] + Position(5, 4));
    milBldPos[2] = world.MakeMapPoint(milBldPos[0] + Position(5, -4));
    // Distance to HQ must be less than distance to other blds or they will be destroyed on capture
    BOOST_REQUIRE_LT(world.CalcDistance(milBldPos[0], world.GetPlayer(0).GetHQPos()) + 1, world.CalcDistance(milBldPos[0], milBldPos[1]));
    BOOST_REQUIRE_LT(world.CalcDistance(milBldPos[0], world.GetPlayer(0).GetHQPos()) + 1, world.CalcDistance(milBldPos[0], milBldPos[2]));
    // Create them in different orders. 3 blds -> 6 orders
    for(unsigned i = 0; i < 6; i++)
    {
        // Create permutation
        std::vector<MapPoint> positions;
        if(i < 2)
            positions.push_back(milBldPos[0]);
        else if(i < 4)
            positions.push_back(milBldPos[1]);
        else
            positions.push_back(milBldPos[2]);
        if(i == 0 || i == 5)
            positions.push_back(milBldPos[1]);
        else if(i % 2 == 0)
            positions.push_back(milBldPos[0]);
        else
            positions.push_back(milBldPos[2]);
        if(i == 1 || i == 4)
            positions.push_back(milBldPos[1]);
        else if(i % 2 == 1)
            positions.push_back(milBldPos[0]);
        else
            positions.push_back(milBldPos[2]);
        BOOST_FOREACH(const MapPoint pt, positions)
            BuildingFactory::CreateBuilding(world, BLD_BARRACKS, pt, (pt == milBldPos[0]) ? 0 : 1, NAT_AFRICANS);
        boost::array<nobBaseMilitary*, 5> milBlds;
        // bld 0 last as it would destroy others
        for(int j = 2; j >= 0; --j)
        {
            milBlds[j] = world.GetSpecObj<nobBaseMilitary>(milBldPos[j]);
            MapPoint flagPt = milBlds[j]->GetFlagPos();
            nofPassiveSoldier* sld = new nofPassiveSoldier(flagPt, milBlds[j]->GetPlayer(), static_cast<nobBaseMilitary*>(milBlds[j]),
                                                           static_cast<nobBaseMilitary*>(milBlds[j]), 0);
            world.AddFigure(flagPt, sld);
            sld->ActAtFirst();
        }
        milBlds[3] = world.GetSpecObj<nobBaseMilitary>(world.GetPlayer(0).GetHQPos());
        milBlds[4] = world.GetSpecObj<nobBaseMilitary>(world.GetPlayer(1).GetHQPos());
        RTTR_SKIP_GFS(30);

        TerritoryRegion region(Position(0, 0), Position(world.GetSize()), world);
        sortedMilitaryBlds buildings = world.LookForMilitaryBuildings(MapPoint(0, 0), 99);
        BOOST_REQUIRE_EQUAL(buildings.size(), 5u);
        BOOST_FOREACH(const nobBaseMilitary* bld, buildings)
            region.CalcTerritoryOfBuilding(*bld);
        // Check that TerritoryRegion assigned owners as expected
        RTTR_FOREACH_PT(MapPoint, world.GetSize())
        {
            uint8_t owner = 0;
            unsigned bestDist = 1000;
            unsigned bestID = 0;
            BOOST_FOREACH(const nobBaseMilitary* bld, milBlds)
            {
                unsigned distance = world.CalcDistance(pt, bld->GetPos());
                // The closest bld gets the point. If 2 players are tied, the younger bld gets it
                if(distance > bestDist || distance > bld->GetMilitaryRadius())
                    continue;
                if(distance < bestDist || bld->GetObjId() > bestID)
                {
                    owner = bld->GetPlayer() + 1;
                    bestDist = distance;
                    bestID = bld->GetObjId();
                }
            }
            RTTR_REQUIRE_EQUAL_MSG(region.GetOwner(Position(pt)), owner, " on " << pt << " iteration " << i);
        }
        // Check that all world points that should have an owner do have one
        RTTR_FOREACH_PT(MapPoint, world.GetSize())
        {
            uint8_t owner = region.GetOwner(Position(pt));
            if(!owner)
                RTTR_REQUIRE_EQUAL_MSG(world.GetNode(pt).owner, 0u, " on " << pt << " iteration " << i);
            else
                RTTR_REQUIRE_NE_MSG(world.GetNode(pt).owner, 0u, " on " << pt << " iteration " << i);
        }
        BOOST_FOREACH(const MapPoint pt, milBldPos)
        {
            world.DestroyNO(pt);
            world.DestroyNO(pt); // Destroy fire
            // Pause figure
            BOOST_FOREACH(noBase* sld, world.GetFigures(pt))
            {
                std::vector<GameEvent*> evts = em.GetObjEvents(*sld);
                BOOST_FOREACH(GameEvent* ev, evts)
                    em.RescheduleEvent(*ev, em.GetCurrentGF() + 10000);
            }
        }
    }
}

BOOST_AUTO_TEST_SUITE_END()
