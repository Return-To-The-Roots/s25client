// Copyright (c) 2005 - 2015 Settlers Freaks (sf-team at siedler25.org)
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

#include "mapGenerator/Map.h"
#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_SUITE(MapTest)

/**
 * Tests the default constructor of the Map class. Width and height of the map
 * are expected to be zero.
 */
BOOST_FIXTURE_TEST_CASE(Constructor_DefaultZeroSize, Map)
{
    Map* map = new Map();
    
    BOOST_REQUIRE_EQUAL(map->width,  0u);
    BOOST_REQUIRE_EQUAL(map->height, 0u);
    
    delete map;
}

/**
 * Tests the constructor of the Map class. The new map should have correct size 
 * according to the constructor parameters.
 */
BOOST_FIXTURE_TEST_CASE(Constructor_CorrectSize, Map)
{
    Map map(64, 64, "name", "author");

    BOOST_REQUIRE_EQUAL(map.width,             64u);
    BOOST_REQUIRE_EQUAL(map.height,            64u);
    BOOST_REQUIRE_EQUAL(map.z.size(),          4096u);
    BOOST_REQUIRE_EQUAL(map.textureRsu.size(), 4096u);
    BOOST_REQUIRE_EQUAL(map.textureLsd.size(), 4096u);
    BOOST_REQUIRE_EQUAL(map.build.size(),      4096u);
    BOOST_REQUIRE_EQUAL(map.shading.size(),    4096u);
    BOOST_REQUIRE_EQUAL(map.resource.size(),   4096u);
    BOOST_REQUIRE_EQUAL(map.road.size(),       4096u);
    BOOST_REQUIRE_EQUAL(map.objectType.size(), 4096u);
    BOOST_REQUIRE_EQUAL(map.objectInfo.size(), 4096u);
    BOOST_REQUIRE_EQUAL(map.animal.size(),     4096u);
    BOOST_REQUIRE_EQUAL(map.unknown1.size(),   4096u);
    BOOST_REQUIRE_EQUAL(map.unknown2.size(),   4096u);
    BOOST_REQUIRE_EQUAL(map.unknown3.size(),   4096u);
    BOOST_REQUIRE_EQUAL(map.unknown5.size(),   4096u);
}

/**
 * Tests the constructor of the Map class. The name must match the name specified
 * in the constructor arguments.
 */
BOOST_FIXTURE_TEST_CASE(Constructor_CorrectName, Map)
{
    std::string name("name");
    Map map(64, 64, name, "author");
    BOOST_REQUIRE_EQUAL(map.name, name);
    
    std::string name2("name2");
    Map map2(64, 64, name2, "author");
    BOOST_REQUIRE_EQUAL(map2.name, name2);
}

/**
 * Tests the constructor of the Map class. The author must match the author specified
 * in the constructor arguments.
 */
BOOST_FIXTURE_TEST_CASE(Constructor_CorrectAuthor, Map)
{
    std::string author1("author1");
    Map mapA(64, 64, "name", author1);
    
    BOOST_REQUIRE_EQUAL(mapA.author, author1);
    
    std::string author2("author2");
    Map mapB(64, 64, "name", author2);
    BOOST_REQUIRE_EQUAL(mapB.author, author2);
}

/**
 * Tests the Map::CreateArchiv method. Ensure that the generated archiv for a map is 
 * not null.
 */
BOOST_FIXTURE_TEST_CASE(CreateArchiv_NotNull, Map)
{
    std::string author("author");
    Map map(64, 64, "name", author);
    
    ArchivInfo* archiv = map.CreateArchiv();
    
    BOOST_REQUIRE(archiv != NULL);

    delete archiv;
}

BOOST_AUTO_TEST_SUITE_END()

