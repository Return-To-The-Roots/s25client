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

#include "PointOutput.h"
#include "mapGenerator/Map.h"
#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_SUITE(MapTest)

/**
 * Tests the default constructor of the Map class. Width and height of the map
 * are expected to be zero.
 */
BOOST_AUTO_TEST_CASE(Constructor_DefaultZeroSize)
{
    Map map;

    BOOST_REQUIRE_EQUAL(map.size, MapExtent::all(0));
}

/**
 * Tests the constructor of the Map class. The new map should have correct size
 * according to the constructor parameters.
 */
BOOST_AUTO_TEST_CASE(Constructor_CorrectSize)
{
    const MapExtent size(64, 32);
    const unsigned numNodes = size.x * size.y;
    Map map(size, "name", "author");

    BOOST_REQUIRE_EQUAL(map.size, size);
    BOOST_REQUIRE_EQUAL(map.z.size(), numNodes);
    BOOST_REQUIRE_EQUAL(map.textureRsu.size(), numNodes);
    BOOST_REQUIRE_EQUAL(map.textureLsd.size(), numNodes);
    BOOST_REQUIRE_EQUAL(map.build.size(), numNodes);
    BOOST_REQUIRE_EQUAL(map.shading.size(), numNodes);
    BOOST_REQUIRE_EQUAL(map.resource.size(), numNodes);
    BOOST_REQUIRE_EQUAL(map.road.size(), numNodes);
    BOOST_REQUIRE_EQUAL(map.objectType.size(), numNodes);
    BOOST_REQUIRE_EQUAL(map.objectInfo.size(), numNodes);
    BOOST_REQUIRE_EQUAL(map.animal.size(), numNodes);
    BOOST_REQUIRE_EQUAL(map.unknown1.size(), numNodes);
    BOOST_REQUIRE_EQUAL(map.unknown2.size(), numNodes);
    BOOST_REQUIRE_EQUAL(map.unknown3.size(), numNodes);
    BOOST_REQUIRE_EQUAL(map.unknown5.size(), numNodes);
}

/**
 * Tests the constructor of the Map class. The name must match the name specified
 * in the constructor arguments.
 */
BOOST_AUTO_TEST_CASE(Constructor_CorrectName)
{
    std::string name("name");
    Map map(MapExtent(64, 32), name, "author");
    BOOST_REQUIRE_EQUAL(map.name, name);

    std::string name2("name2");
    Map map2(MapExtent(64, 32), name2, "author");
    BOOST_REQUIRE_EQUAL(map2.name, name2);
}

/**
 * Tests the constructor of the Map class. The author must match the author specified
 * in the constructor arguments.
 */
BOOST_AUTO_TEST_CASE(Constructor_CorrectAuthor)
{
    std::string author1("author1");
    Map mapA(MapExtent(64, 32), "name", author1);

    BOOST_REQUIRE_EQUAL(mapA.author, author1);

    std::string author2("author2");
    Map mapB(MapExtent(64, 32), "name", author2);
    BOOST_REQUIRE_EQUAL(mapB.author, author2);
}

BOOST_AUTO_TEST_SUITE_END()
