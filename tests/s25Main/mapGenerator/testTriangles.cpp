// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

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
