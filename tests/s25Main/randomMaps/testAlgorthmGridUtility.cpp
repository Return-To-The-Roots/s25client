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

#include "rttrDefines.h" // IWYU pragma: keep
#include "randomMaps/algorithm/GridUtility.h"

#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_SUITE(GridUtilityTests)

BOOST_AUTO_TEST_CASE(GetPositionOfValidIndex)
{
    BOOST_REQUIRE(GridUtility::GetPosition(0, MapExtent(2,2)) == Position(0,0));
    BOOST_REQUIRE(GridUtility::GetPosition(1, MapExtent(2,2)) == Position(1,0));
    BOOST_REQUIRE(GridUtility::GetPosition(2, MapExtent(2,2)) == Position(0,1));
    BOOST_REQUIRE(GridUtility::GetPosition(3, MapExtent(2,2)) == Position(1,1));
}

BOOST_AUTO_TEST_CASE(ClampPositionInsideGridReturnsPosition)
{
    BOOST_REQUIRE(GridUtility::Clamp(Position(0,0), MapExtent(2,2)) == Position(0,0));
    BOOST_REQUIRE(GridUtility::Clamp(Position(1,0), MapExtent(2,2)) == Position(1,0));
    BOOST_REQUIRE(GridUtility::Clamp(Position(0,1), MapExtent(2,2)) == Position(0,1));
    BOOST_REQUIRE(GridUtility::Clamp(Position(1,1), MapExtent(2,2)) == Position(1,1));
}

BOOST_AUTO_TEST_CASE(ClampPositionOutsideOfGridReturnsPositionInsideOfGrid)
{
    BOOST_REQUIRE(GridUtility::Clamp(Position(-1,0), MapExtent(2,2)) == Position(1,0));
    BOOST_REQUIRE(GridUtility::Clamp(Position(0,-1), MapExtent(2,2)) == Position(0,1));
    BOOST_REQUIRE(GridUtility::Clamp(Position(2,1), MapExtent(2,2)) == Position(0,1));
    BOOST_REQUIRE(GridUtility::Clamp(Position(1,2), MapExtent(2,2)) == Position(1,0));
    BOOST_REQUIRE(GridUtility::Clamp(Position(1,3), MapExtent(2,2)) == Position(1,1));
    BOOST_REQUIRE(GridUtility::Clamp(Position(-2,-3), MapExtent(2,2)) == Position(0,1));
}

BOOST_AUTO_TEST_CASE(DistanceIsCloseForTwoPointsNearTheSameEdge)
{
    Position p1(15,8), p2(0,8);
    
    auto distance = GridUtility::Distance(p1, p2, MapExtent(16,16));
    
    BOOST_REQUIRE_CLOSE(distance, 1.0, 0.0001);
}

BOOST_AUTO_TEST_CASE(NeighborsReturnsFourNeighbors)
{
    Position p(2,4);
    MapExtent size(12,16);
    
    auto neighbors = GridUtility::Neighbors(p, size);
    
    BOOST_REQUIRE(neighbors.size() == 4);
    
    for (auto neighbor = neighbors.begin(); neighbor != neighbors.end(); ++neighbor)
    {
        BOOST_REQUIRE_CLOSE(GridUtility::Distance(*neighbor, p, size), 1.0, 0.0001);
    }
}

BOOST_AUTO_TEST_CASE(CollectReturnsPointsWithinExpectedRadius)
{
    Position p(4,2);
    MapExtent size(16,16);
    
    auto neighbors = GridUtility::Collect(p, size, 2.5);
    
    for (auto neighbor = neighbors.begin(); neighbor != neighbors.end(); ++neighbor)
    {
        BOOST_REQUIRE_LT(GridUtility::Distance(*neighbor, p, size), 2.5);
    }
}

BOOST_AUTO_TEST_CASE(DeltaComputesShortestDistanceBetweenTwoPoints)
{
    auto size = MapExtent(7,22);
    auto delta = GridUtility::Delta(Position(0,0), Position(6,21), size);
    
    BOOST_REQUIRE(delta.x == 1 && delta.y == 1);
}

BOOST_AUTO_TEST_CASE(NormalizedDistanceBetweenPointsIsZeroOrGreater)
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
                    auto distance = GridUtility::DistanceNorm(Position(x1,y1),
                                                              Position(x2,y2), size);
                    BOOST_REQUIRE(distance >= 0.0);
                }
            }
        }
    }
}

BOOST_AUTO_TEST_CASE(NormalizedDistanceBetweenPointsIsOneOrLess)
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
                    auto distance = GridUtility::DistanceNorm(Position(x1,y1),
                                                              Position(x2,y2), size);
                    BOOST_REQUIRE(distance <= 1.0);
                }
            }
        }
    }
}

BOOST_AUTO_TEST_CASE(PositionsForMapSizeReturnsAllGridPoints)
{
    auto size = MapExtent(8,3);
    auto positions = GridUtility::Positions(size);
    
    BOOST_REQUIRE(positions.size() == 24u);
    
    for (int i = 0; i < 24; i++)
    {
        BOOST_REQUIRE(positions[i].x == i % 8);
        BOOST_REQUIRE(positions[i].y == i / 8);
    }
}

BOOST_AUTO_TEST_CASE(RsuTriangleNeighborsComputedCorrectly)
{
    auto size = MapExtent(4,4);
    auto neighbors = GridUtility::NeighborsOfRsuTriangle(Position(2,2), size);
    
    BOOST_REQUIRE(neighbors.size() == 3u);
    BOOST_REQUIRE(neighbors[0].x == 2);
    BOOST_REQUIRE(neighbors[0].y == 2);
    BOOST_REQUIRE(neighbors[1].x == 1);
    BOOST_REQUIRE(neighbors[1].y == 2);
    BOOST_REQUIRE(neighbors[2].x == 1);
    BOOST_REQUIRE(neighbors[2].y == 3);
    
    auto edgeNeighbors = GridUtility::NeighborsOfRsuTriangle(Position(0,0), size);

    BOOST_REQUIRE(edgeNeighbors.size() == 3u);
    BOOST_REQUIRE(edgeNeighbors[0].x == 0);
    BOOST_REQUIRE(edgeNeighbors[0].y == 0);
    BOOST_REQUIRE(edgeNeighbors[1].x == 3);
    BOOST_REQUIRE(edgeNeighbors[1].y == 0);
    BOOST_REQUIRE(edgeNeighbors[2].x == 3);
    BOOST_REQUIRE(edgeNeighbors[2].y == 1);
    
    auto mod2Neighbors = GridUtility::NeighborsOfRsuTriangle(Position(1,1), size);

    BOOST_REQUIRE(mod2Neighbors.size() == 3u);
    BOOST_REQUIRE(mod2Neighbors[0].x == 1);
    BOOST_REQUIRE(mod2Neighbors[0].y == 1);
    BOOST_REQUIRE(mod2Neighbors[1].x == 0);
    BOOST_REQUIRE(mod2Neighbors[1].y == 1);
    BOOST_REQUIRE(mod2Neighbors[2].x == 1);
    BOOST_REQUIRE(mod2Neighbors[2].y == 2);
}

BOOST_AUTO_TEST_CASE(LsdTriangleNeighborsComputedCorrectly)
{
    auto size = MapExtent(4,4);
    auto neighbors = GridUtility::NeighborsOfLsdTriangle(Position(2,2), size);
    
    BOOST_REQUIRE(neighbors.size() == 3u);
    BOOST_REQUIRE(neighbors[0].x == 2);
    BOOST_REQUIRE(neighbors[0].y == 2);
    BOOST_REQUIRE(neighbors[1].x == 2);
    BOOST_REQUIRE(neighbors[1].y == 1);
    BOOST_REQUIRE(neighbors[2].x == 3);
    BOOST_REQUIRE(neighbors[2].y == 2);
    
    auto edgeNeighbors = GridUtility::NeighborsOfLsdTriangle(Position(3,0), size);

    BOOST_REQUIRE(edgeNeighbors.size() == 3u);
    BOOST_REQUIRE(edgeNeighbors[0].x == 3);
    BOOST_REQUIRE(edgeNeighbors[0].y == 0);
    BOOST_REQUIRE(edgeNeighbors[1].x == 0);
    BOOST_REQUIRE(edgeNeighbors[1].y == 0);
    BOOST_REQUIRE(edgeNeighbors[2].x == 0);
    BOOST_REQUIRE(edgeNeighbors[2].y == 3);
    
    auto mod2Neighbors = GridUtility::NeighborsOfLsdTriangle(Position(1,1), size);

    BOOST_REQUIRE(mod2Neighbors.size() == 3u);
    BOOST_REQUIRE(mod2Neighbors[0].x == 1);
    BOOST_REQUIRE(mod2Neighbors[0].y == 1);
    BOOST_REQUIRE(mod2Neighbors[1].x == 2);
    BOOST_REQUIRE(mod2Neighbors[1].y == 1);
    BOOST_REQUIRE(mod2Neighbors[2].x == 2);
    BOOST_REQUIRE(mod2Neighbors[2].y == 0);
}

BOOST_AUTO_TEST_SUITE_END()
