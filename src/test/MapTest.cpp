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
 * Tests the constructor of the Map class. The new map should have correct size 
 * according to the constructor parameters.
 */
BOOST_FIXTURE_TEST_CASE(Constructor_CorrectSize, Map)
{
    Map* map = new Map(64, 64, "name", "author");

    BOOST_REQUIRE_EQUAL(map->width,             (unsigned)64);
    BOOST_REQUIRE_EQUAL(map->height,            (unsigned)64);
    BOOST_REQUIRE_EQUAL(map->z.size(),          (unsigned)4096);
    BOOST_REQUIRE_EQUAL(map->textureRsu.size(), (unsigned)4096);
    BOOST_REQUIRE_EQUAL(map->textureLsd.size(), (unsigned)4096);
    BOOST_REQUIRE_EQUAL(map->build.size(),      (unsigned)4096);
    BOOST_REQUIRE_EQUAL(map->shading.size(),    (unsigned)4096);
    BOOST_REQUIRE_EQUAL(map->resource.size(),   (unsigned)4096);
    BOOST_REQUIRE_EQUAL(map->road.size(),       (unsigned)4096);
    BOOST_REQUIRE_EQUAL(map->objectType.size(), (unsigned)4096);
    BOOST_REQUIRE_EQUAL(map->objectInfo.size(), (unsigned)4096);
    BOOST_REQUIRE_EQUAL(map->animal.size(),     (unsigned)4096);
    BOOST_REQUIRE_EQUAL(map->unknown1.size(),   (unsigned)4096);
    BOOST_REQUIRE_EQUAL(map->unknown2.size(),   (unsigned)4096);
    BOOST_REQUIRE_EQUAL(map->unknown3.size(),   (unsigned)4096);
    BOOST_REQUIRE_EQUAL(map->unknown5.size(),   (unsigned)4096);

    delete map;
}

/**
 * Tests the constructor of the Map class. The name must match the name specified
 * in the constructor arguments.
 */
BOOST_FIXTURE_TEST_CASE(Constructor_CorrectName, Map)
{
    std::string name("name");
    Map* map = new Map(64, 64, name, "author");
    
    BOOST_REQUIRE_EQUAL(map->name, name);

    delete map;
}

/**
 * Tests the constructor of the Map class. The author must match the author specified
 * in the constructor arguments.
 */
BOOST_FIXTURE_TEST_CASE(Constructor_CorrectAuthor, Map)
{
    std::string author("author");
    Map* map = new Map(64, 64, "name", author);
    
    BOOST_REQUIRE_EQUAL(map->author, author);
    
    delete map;
}

/**
 * Tests the Map::CreateArchiv method. Ensure that the generated archiv for a map is 
 * not null.
 */
BOOST_FIXTURE_TEST_CASE(CreateArchiv_NotNull, Map)
{
    std::string author("author");
    Map* map = new Map(64, 64, "name", author);
    
    ArchivInfo* archiv = map->CreateArchiv();
    
    BOOST_REQUIRE(archiv != NULL);

    delete archiv;
    delete map;
}

BOOST_AUTO_TEST_SUITE_END()

