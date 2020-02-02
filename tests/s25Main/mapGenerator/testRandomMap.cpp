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

#include "mapGenerator/RandomMap.h"
#include "testHelper.hpp"

#include <boost/test/unit_test.hpp>

using namespace rttr::mapGenerator;

BOOST_AUTO_TEST_SUITE(RandomMapTests)

void ValidateMap(Map_& map, const MapExtent& size, int players);
void ValidateMap(Map_& map, const MapExtent& size, int players)
{
    // ==============================
    // = Validation of HQ positions =
    // ==============================
    for (int i = 0; i < players; i++)
    {
        BOOST_REQUIRE(map.hqPositions[i].isValid());
    }

    auto hqs = map.hqPositions;
    
    for (int i = 0; i < 7; i++)
    {
        for (int k = i + 1; k < 7; k++)
        {
            BOOST_REQUIRE(hqs[i].x != hqs[k].x || hqs[i].y != hqs[k].y);
        }
    }
    
    // ================================
    // = Validation of map properties =
    // ================================
    size_t gridSize = size.x * size.y;
    
    BOOST_REQUIRE(map.z.size() == gridSize);
    BOOST_REQUIRE(map.textureRsu.size() == gridSize);
    BOOST_REQUIRE(map.textureLsd.size() == gridSize);
    BOOST_REQUIRE(map.build.size() == gridSize);
    BOOST_REQUIRE(map.shading.size() == gridSize);
    BOOST_REQUIRE(map.resource.size() == gridSize);
    BOOST_REQUIRE(map.road.size() == gridSize);
    BOOST_REQUIRE(map.objectType.size() == gridSize);
    BOOST_REQUIRE(map.objectInfo.size() == gridSize);
    BOOST_REQUIRE(map.animal.size() == gridSize);
    BOOST_REQUIRE(map.unknown1.size() == gridSize);
    BOOST_REQUIRE(map.unknown2.size() == gridSize);
    BOOST_REQUIRE(map.unknown3.size() == gridSize);
    BOOST_REQUIRE(map.unknown5.size() == gridSize);
}
/*
BOOST_AUTO_TEST_CASE(CreateCrazyMap_GeneratesAValidMap)
{
    MockTextureMapping mapping;
    RandomUtility rnd(0);
    MapExtent size(64,64);
    
    WorldDescription worldDesc;
    Map map(worldDesc, size);
    map.numPlayers = 7;

    CreateCrazyMap(rnd, map, mapping);
    
    ValidateMap(map, size, 7);
}

BOOST_AUTO_TEST_CASE(CreateRiversMap_GeneratesAValidMap)
{
    MockTextureMapping mapping;
    RandomUtility rnd(0);
    MapExtent size(64,64);
    
    WorldDescription worldDesc;
    Map map(worldDesc, size);
    map.numPlayers = 7;

    CreateRiversMap(rnd, map, mapping);
    
    ValidateMap(map, size, 7);
}

BOOST_AUTO_TEST_CASE(CreateEdgecaseMap_GeneratesAValidMap)
{
    MockTextureMapping mapping;
    RandomUtility rnd(0);
    MapExtent size(64,64);
    
    WorldDescription worldDesc;
    Map map(worldDesc, size);
    map.numPlayers = 7;

    CreateEdgecaseMap(rnd, map, mapping);
    
    ValidateMap(map, size, 7);
}

BOOST_AUTO_TEST_CASE(CreateContinentMap_GeneratesAValidMap)
{
    MockTextureMapping mapping;
    RandomUtility rnd(0);
    MapExtent size(64,64);
    
    WorldDescription worldDesc;
    Map map(worldDesc, size);
    map.numPlayers = 7;

    CreateContinentMap(rnd, map, mapping);
    
    ValidateMap(map, size, 7);
}
*/
BOOST_AUTO_TEST_CASE(CreateMigrationMap_GeneratesAValidMap)
{
    MockTextureMapping_ mapping;
    RandomUtility rnd(0);
    MapExtent size(64,64);
    
    WorldDescription worldDesc;
    Map_ map(worldDesc, size);
    map.numPlayers = 7;
    
    CreateMigrationMap(rnd, map, mapping);
    
    ValidateMap(map, size, 7);
}

BOOST_AUTO_TEST_SUITE_END()
