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

#include "RttrForeachPt.h"
#include "mapGenerator/Triangles.h"
#include <boost/test/unit_test.hpp>

using namespace rttr::mapGenerator;

BOOST_AUTO_TEST_SUITE(TriangleTests)

BOOST_AUTO_TEST_CASE(GetTriangleNeighbors_always_returns_three_neighbors)
{
    MapExtent size(16, 8);

    RTTR_FOREACH_PT(MapPoint, size)
    {
        BOOST_TEST_REQUIRE(GetTriangleNeighbors(Triangle(true, pt), size).size() == 3u);
        BOOST_TEST_REQUIRE(GetTriangleNeighbors(Triangle(false, pt), size).size() == 3u);
    }
}

BOOST_AUTO_TEST_CASE(GetTriangles_always_returns_six_triangles)
{
    MapExtent size(16, 8);

    RTTR_FOREACH_PT(MapPoint, size)
    {
        BOOST_TEST_REQUIRE(GetTriangles(pt, size).size() == 6u);
    }
}

BOOST_AUTO_TEST_CASE(GetTriangleEdges_always_returns_three_edges)
{
    MapExtent size(16, 8);

    RTTR_FOREACH_PT(MapPoint, size)
    {
        BOOST_TEST_REQUIRE(GetTriangleEdges(Triangle(true, pt), size).size() == 3u);
        BOOST_TEST_REQUIRE(GetTriangleEdges(Triangle(false, pt), size).size() == 3u);
    }
}

BOOST_AUTO_TEST_SUITE_END()
