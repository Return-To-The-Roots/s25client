// Copyright (c) 2017 - 2017 Settlers Freaks (sf-team at siedler25.org)
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

#include "rttrDefines.h"
#include "mapGenerator/GridUtility.h"
#include <boost/test/unit_test.hpp>

#include <iostream>

using namespace rttr::mapGenerator;

BOOST_AUTO_TEST_SUITE(GridUtilityTests)

BOOST_AUTO_TEST_CASE(GridPosition_ForIndex_ReturnsExpectedPosition)
{
    BOOST_REQUIRE(GridPosition(0, MapExtent(2,2)) == Position(0,0));
    BOOST_REQUIRE(GridPosition(1, MapExtent(2,2)) == Position(1,0));
    BOOST_REQUIRE(GridPosition(2, MapExtent(2,2)) == Position(0,1));
    BOOST_REQUIRE(GridPosition(3, MapExtent(2,2)) == Position(1,1));
}

BOOST_AUTO_TEST_CASE(GridClamp_ForPositionWithinBounds_ReturnsTheSamePosition)
{
    BOOST_REQUIRE(GridClamp(Position(0,0), MapExtent(2,2)) == Position(0,0));
    BOOST_REQUIRE(GridClamp(Position(1,0), MapExtent(2,2)) == Position(1,0));
    BOOST_REQUIRE(GridClamp(Position(0,1), MapExtent(2,2)) == Position(0,1));
    BOOST_REQUIRE(GridClamp(Position(1,1), MapExtent(2,2)) == Position(1,1));
}

BOOST_AUTO_TEST_CASE(GridClamp_ForPositionOutsideOfBounds_ReturnsClampedPosition)
{
    BOOST_REQUIRE(GridClamp(Position(-1,0), MapExtent(2,2)) == Position(1,0));
    BOOST_REQUIRE(GridClamp(Position(0,-1), MapExtent(2,2)) == Position(0,1));
    BOOST_REQUIRE(GridClamp(Position(2,1), MapExtent(2,2)) == Position(0,1));
    BOOST_REQUIRE(GridClamp(Position(1,2), MapExtent(2,2)) == Position(1,0));
    BOOST_REQUIRE(GridClamp(Position(1,3), MapExtent(2,2)) == Position(1,1));
    BOOST_REQUIRE(GridClamp(Position(-2,-3), MapExtent(2,2)) == Position(0,1));
}

BOOST_AUTO_TEST_CASE(GridDistance_ForTwoPoints_ReturnsExpectedDistance)
{
    Position p1(15,8), p2(0,8);
    
    auto distance = GridDistance(p1, p2, MapExtent(16,16));
    
    BOOST_REQUIRE_CLOSE(distance, 1.0, 0.0001);
}

BOOST_AUTO_TEST_CASE(GridNeighbors_ForAnyPointOnMap_AlwaysReturns4ValidPositions)
{
    MapExtent size(8,8);

    RTTR_FOREACH_PT(MapPoint, size)
    {
        auto neighbors = GridNeighbors(Position(pt), size);
        
        BOOST_REQUIRE(neighbors.size() == 4);
        
        for (Position neighbor : neighbors)
        {
            BOOST_REQUIRE(neighbor.isValid());
        }
    }
}

BOOST_AUTO_TEST_CASE(GridNeighbors_ForCorners_ReturnsExpectedNeighbors)
{
    MapExtent size(8,8);
    
    auto neighbors = GridNeighbors(Position(0, 0), size);

    BOOST_REQUIRE(neighbors[0] == Position(1, 0));
    BOOST_REQUIRE(neighbors[1] == Position(7, 0));
    BOOST_REQUIRE(neighbors[2] == Position(7, 1));
    BOOST_REQUIRE(neighbors[3] == Position(0, 7));

    neighbors = GridNeighbors(Position(7, 0), size);

    BOOST_REQUIRE(neighbors[0] == Position(0, 0));
    BOOST_REQUIRE(neighbors[1] == Position(6, 0));
    BOOST_REQUIRE(neighbors[2] == Position(6, 1));
    BOOST_REQUIRE(neighbors[3] == Position(7, 7));
    
    neighbors = GridNeighbors(Position(7, 7), size);

    BOOST_REQUIRE(neighbors[0] == Position(0, 7));
    BOOST_REQUIRE(neighbors[1] == Position(6, 7));
    BOOST_REQUIRE(neighbors[2] == Position(0, 6));
    BOOST_REQUIRE(neighbors[3] == Position(7, 0));
    
    neighbors = GridNeighbors(Position(0, 7), size);

    BOOST_REQUIRE(neighbors[0] == Position(1, 7));
    BOOST_REQUIRE(neighbors[1] == Position(7, 7));
    BOOST_REQUIRE(neighbors[2] == Position(1, 6));
    BOOST_REQUIRE(neighbors[3] == Position(0, 0));
}

BOOST_AUTO_TEST_CASE(GridNeighbors_ForTopEdge_ReturnsExpectedNeighbors)
{
    MapExtent size(8,8);
    
    const int y = size.y - 1;
    
    for (int x = 1; x < size.x - 1; x++)
    {
        auto neighbors = GridNeighbors(Position(x, y), size);

        BOOST_REQUIRE(neighbors[0] == Position(x + 1, y));
        BOOST_REQUIRE(neighbors[1] == Position(x - 1, y));
        BOOST_REQUIRE(neighbors[2] == Position(x + 1, y - 1));
        BOOST_REQUIRE(neighbors[3] == Position(x, 0));
    }
}

BOOST_AUTO_TEST_CASE(GridNeighbors_ForBottomEdge_ReturnsExpectedNeighbors)
{
    MapExtent size(8,8);
    
    for (int x = 1; x < size.x - 1; x++)
    {
        auto neighbors = GridNeighbors(Position(x, 0), size);

        BOOST_REQUIRE(neighbors[0] == Position(x + 1, 0));
        BOOST_REQUIRE(neighbors[1] == Position(x - 1, 0));
        BOOST_REQUIRE(neighbors[2] == Position(x - 1, 1));
        BOOST_REQUIRE(neighbors[3] == Position(x, 7));
    }
}

BOOST_AUTO_TEST_CASE(GridNeighbors_ForLeftEdge_ReturnsExpectedNeighbors)
{
    MapExtent size(8,8);
    
    for (int y = 1; y < size.y - 1; y++)
    {
        auto neighbors = GridNeighbors(Position(0, y), size);

        BOOST_REQUIRE(neighbors[0] == Position(1, y));
        BOOST_REQUIRE(neighbors[1] == Position(7, y));
        
        if (y % 2 == 0)
        {
            BOOST_REQUIRE(neighbors[2] == Position(7, y + 1));
            BOOST_REQUIRE(neighbors[3] == Position(0, y - 1));
        }
        else
        {
            BOOST_REQUIRE(neighbors[2] == Position(1, y - 1));
            BOOST_REQUIRE(neighbors[3] == Position(0, y + 1));
        }
    }
}

BOOST_AUTO_TEST_CASE(GridNeighbors_ForRightEdge_ReturnsExpectedNeighbors)
{
    MapExtent size(8,8);
    
    const int x = size.x - 1;
    
    for (int y = 1; y < size.y - 1; y++)
    {
        auto neighbors = GridNeighbors(Position(x, y), size);

        BOOST_REQUIRE(neighbors[0] == Position(0, y));
        BOOST_REQUIRE(neighbors[1] == Position(x - 1, y));
        if (y % 2 == 0)
        {
            BOOST_REQUIRE(neighbors[2] == Position(x - 1, y + 1));
            BOOST_REQUIRE(neighbors[3] == Position(x, y - 1));
        }
        else
        {
            BOOST_REQUIRE(neighbors[2] == Position(0, y - 1));
            BOOST_REQUIRE(neighbors[3] == Position(x, y + 1));
        }
    }
}

BOOST_AUTO_TEST_CASE(GridNeighbors_ForAnyPosition_ReturnsPositionsWithMaxDelta1InXOrY)
{
    MapExtent size(8,8);

    RTTR_FOREACH_PT(MapPoint, size)
    {
        Position pos(pt);
        Positions neighbors = GridNeighbors(pos, size);

        for (auto neighbor : neighbors)
        {
            auto delta = GridDelta(pos, neighbor, size);
            
            BOOST_REQUIRE(delta.x == 0 || delta.y == 0);
            BOOST_REQUIRE(delta.x == 1 || delta.x == -1 ||
                          delta.y == 1 || delta.y == -1);
        }
    }
}

/*
BOOST_AUTO_TEST_CASE(GridNeighbors_ForPosition_Returns4PositionsWithMaximumDistance1)
{
    Position p(2,4);
    MapExtent size(12,16);
    
    auto neighbors = GridNeighbors(p, size);
    
    BOOST_REQUIRE(neighbors.size() == 4);
    
    for (auto neighbor : neighbors)
    {
        BOOST_REQUIRE_CLOSE(GridDistance(neighbor, p, size), 1.0, 0.0001);
    }
}
*/

BOOST_AUTO_TEST_CASE(GridCollect_ForPointAndRaidus_ReturnsPointsWithinDistanceOfRadius)
{
    Position p(4,2);
    MapExtent size(16,16);
    
    auto neighbors = GridCollect(p, size, 2.5);
    
    for (auto neighbor : neighbors)
    {
        BOOST_REQUIRE_LT(GridDistance(neighbor, p, size), 2.5);
    }
}

BOOST_AUTO_TEST_CASE(GridDelta_ForTwoPointsWithShiftedColumns_ReturnsExpectedDifferences)
{
    /**
     *  /__/__/44/
     *   /__/__/
     *  /42/__/
     */
    auto delta = GridDelta(Position(4,2), Position(4,4), MapExtent(8,8));
    
    BOOST_REQUIRE(delta.x == 1 && delta.y == 2);
}

BOOST_AUTO_TEST_CASE(GridDelta_BetweenCorners_ReturnsExpectedDifferences)
{
    auto delta = GridDelta(Position(0,0), Position(7,7), MapExtent(8,8));
    
    BOOST_REQUIRE(delta.x == -1 && delta.y == -1);
    
    delta = GridDelta(Position(0,0), Position(0,7), MapExtent(8,8));

    BOOST_REQUIRE(delta.x == 0 && delta.y == -1);

    delta = GridDelta(Position(0,0), Position(7,0), MapExtent(8,8));

    BOOST_REQUIRE(delta.x == -1 && delta.y == 0);
    
    delta = GridDelta(Position(7,7), Position(0,0), MapExtent(8,8));
    
    BOOST_REQUIRE(delta.x == 1 && delta.y == 1);
}

BOOST_AUTO_TEST_CASE(GridDelta_BetweenLeftAndRightEdges_ReturnsExpectedDifferences)
{
    auto size = MapExtent(8, 8);
    for (int x = 1; x < size.x - 1; x++)
    {
        auto delta = GridDelta(Position(x, size.y - 1), Position(x, 0), size);
        
        BOOST_REQUIRE(delta.x == 0 && delta.y == 1);
    }
}

BOOST_AUTO_TEST_CASE(GridDelta_BetweenTopAndBottomEdges_ReturnsExpectedDifferences)
{
    auto size = MapExtent(8, 8);
    for (int y = 1; y < size.y - 1; y++)
    {
        auto delta = GridDelta(Position(7, y), Position(0, y), size);
        
        BOOST_REQUIRE(delta.x == 1 && delta.y == 0);
    }
}

BOOST_AUTO_TEST_CASE(GridDelta_OfAnyPair_ReturnsInverseOfSwappedPair)
{
    // delta between points is inverse of delta between swapped points
    auto size = MapExtent(8, 8);
    for (int x1 = 0; x1 < size.x; x1++)
    {
        for (int y1 = 0; y1 < size.y; y1++)
        {
            for (int x2 = 0; x2 < size.x; x2++)
            {
                for (int y2 = 0; y2 < size.y; y2++)
                {
                    auto d1 = GridDelta(Position(x1, y1), Position(x2, y2), size);
                    auto d2 = GridDelta(Position(x2, y2), Position(x1, y1), size);

                    BOOST_REQUIRE(d1.x == -d2.x && d1.y == -d2.y);
                }
            }

        }
    }
}

BOOST_AUTO_TEST_CASE(GridDistanceNorm_ForAnyTwoPointsOnAGrid_ReturnsDistanceLargerOrEqualToZero)
{
    auto size = MapExtent(23,11);
    
    for (int x1 = 0; x1 < size.x; x1++)
    {
        for (int y1 = 0; y1 < size.y; y1++)
        {
            for (int x2 = 0; x2 < size.x; x2++)
            {
                for (int y2 = 0; y2 < size.y; y2++)
                {
                    BOOST_REQUIRE(GridDistanceNorm(Position(x1,y1), Position(x2,y2), size) >= 0.0);
                }
            }
        }
    }
}

BOOST_AUTO_TEST_CASE(GridDistanceNorm_ForAnyTwoPointsOnAGrid_ReturnsDistanceLessOrEqualToOne)
{
    auto size = MapExtent(23,11);
    
    for (int x1 = 0; x1 < size.x; x1++)
    {
        for (int y1 = 0; y1 < size.y; y1++)
        {
            for (int x2 = 0; x2 < size.x; x2++)
            {
                for (int y2 = 0; y2 < size.y; y2++)
                {
                    BOOST_REQUIRE(GridDistanceNorm(Position(x1,y1), Position(x2,y2), size) <= 1.0);
                }
            }
        }
    }
}

BOOST_AUTO_TEST_CASE(GridPositions_ForMapSize_ReturnsAllPointsOnTheMap)
{
    auto size = MapExtent(8,3);
    auto positions = GridPositions(size);
    
    BOOST_REQUIRE(positions.size() == 24u);
    
    for (int i = 0; i < 24; i++)
    {
        BOOST_REQUIRE(positions[i].x == i % 8);
        BOOST_REQUIRE(positions[i].y == i / 8);
    }
}

BOOST_AUTO_TEST_CASE(GridNeighborsOfRsuTriangle_ForPositionAndMapSize_ReturnsExpectedNeighbors)
{
    auto size = MapExtent(4,4);
    auto neighbors = GridNeighborsOfRsuTriangle(Position(2,2), size);
    
    BOOST_REQUIRE(neighbors.size() == 3u);
    BOOST_REQUIRE(neighbors[0].x == 2);
    BOOST_REQUIRE(neighbors[0].y == 2);
    BOOST_REQUIRE(neighbors[1].x == 1);
    BOOST_REQUIRE(neighbors[1].y == 2);
    BOOST_REQUIRE(neighbors[2].x == 1);
    BOOST_REQUIRE(neighbors[2].y == 3);
    
    auto edgeNeighbors = GridNeighborsOfRsuTriangle(Position(0,0), size);

    BOOST_REQUIRE(edgeNeighbors.size() == 3u);
    BOOST_REQUIRE(edgeNeighbors[0].x == 0);
    BOOST_REQUIRE(edgeNeighbors[0].y == 0);
    BOOST_REQUIRE(edgeNeighbors[1].x == 3);
    BOOST_REQUIRE(edgeNeighbors[1].y == 0);
    BOOST_REQUIRE(edgeNeighbors[2].x == 3);
    BOOST_REQUIRE(edgeNeighbors[2].y == 1);
    
    auto mod2Neighbors = GridNeighborsOfRsuTriangle(Position(1,1), size);

    BOOST_REQUIRE(mod2Neighbors.size() == 3u);
    BOOST_REQUIRE(mod2Neighbors[0].x == 1);
    BOOST_REQUIRE(mod2Neighbors[0].y == 1);
    BOOST_REQUIRE(mod2Neighbors[1].x == 0);
    BOOST_REQUIRE(mod2Neighbors[1].y == 1);
    BOOST_REQUIRE(mod2Neighbors[2].x == 1);
    BOOST_REQUIRE(mod2Neighbors[2].y == 2);
}

BOOST_AUTO_TEST_CASE(GridNeighborsOfLsdTriangle_ForPositionAndMapSize_ReturnsExpectedNeighbors)
{
    auto size = MapExtent(4,4);
    auto neighbors = GridNeighborsOfLsdTriangle(Position(2,2), size);
    
    BOOST_REQUIRE(neighbors.size() == 3u);
    BOOST_REQUIRE(neighbors[0].x == 2);
    BOOST_REQUIRE(neighbors[0].y == 2);
    BOOST_REQUIRE(neighbors[1].x == 2);
    BOOST_REQUIRE(neighbors[1].y == 1);
    BOOST_REQUIRE(neighbors[2].x == 3);
    BOOST_REQUIRE(neighbors[2].y == 2);
    
    auto edgeNeighbors = GridNeighborsOfLsdTriangle(Position(3,0), size);

    BOOST_REQUIRE(edgeNeighbors.size() == 3u);
    BOOST_REQUIRE(edgeNeighbors[0].x == 3);
    BOOST_REQUIRE(edgeNeighbors[0].y == 0);
    BOOST_REQUIRE(edgeNeighbors[1].x == 0);
    BOOST_REQUIRE(edgeNeighbors[1].y == 0);
    BOOST_REQUIRE(edgeNeighbors[2].x == 0);
    BOOST_REQUIRE(edgeNeighbors[2].y == 3);
    
    auto mod2Neighbors = GridNeighborsOfLsdTriangle(Position(1,1), size);

    BOOST_REQUIRE(mod2Neighbors.size() == 3u);
    BOOST_REQUIRE(mod2Neighbors[0].x == 1);
    BOOST_REQUIRE(mod2Neighbors[0].y == 1);
    BOOST_REQUIRE(mod2Neighbors[1].x == 2);
    BOOST_REQUIRE(mod2Neighbors[1].y == 1);
    BOOST_REQUIRE(mod2Neighbors[2].x == 2);
    BOOST_REQUIRE(mod2Neighbors[2].y == 0);
}

BOOST_AUTO_TEST_SUITE_END()
