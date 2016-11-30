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

#include "mapGenerator/ObjectGenerator.h"
#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_SUITE(ObjectGeneratorTest)

/**
 * Tests the ObjectGenerator::IsHarborAllowed method to ensure the method returns correct
 * values for all terrain types.
 */
BOOST_FIXTURE_TEST_CASE(IsHarborAllowed_TerrainType, ObjectGenerator)
{
    BOOST_REQUIRE_EQUAL(ObjectGenerator::IsHarborAllowed(TT_SNOW),              false);
    BOOST_REQUIRE_EQUAL(ObjectGenerator::IsHarborAllowed(TT_LAVA),              false);
    BOOST_REQUIRE_EQUAL(ObjectGenerator::IsHarborAllowed(TT_LAVA2),             false);
    BOOST_REQUIRE_EQUAL(ObjectGenerator::IsHarborAllowed(TT_LAVA3),             false);
    BOOST_REQUIRE_EQUAL(ObjectGenerator::IsHarborAllowed(TT_LAVA4),             false);
    BOOST_REQUIRE_EQUAL(ObjectGenerator::IsHarborAllowed(TT_WATER),             false);
    BOOST_REQUIRE_EQUAL(ObjectGenerator::IsHarborAllowed(TT_WATER_NOSHIP),      false);
    BOOST_REQUIRE_EQUAL(ObjectGenerator::IsHarborAllowed(TT_DESERT),            false);
    BOOST_REQUIRE_EQUAL(ObjectGenerator::IsHarborAllowed(TT_MOUNTAIN1),         false);
    BOOST_REQUIRE_EQUAL(ObjectGenerator::IsHarborAllowed(TT_MOUNTAIN2),         false);
    BOOST_REQUIRE_EQUAL(ObjectGenerator::IsHarborAllowed(TT_MOUNTAIN3),         false);
    BOOST_REQUIRE_EQUAL(ObjectGenerator::IsHarborAllowed(TT_MOUNTAIN4),         false);
    BOOST_REQUIRE_EQUAL(ObjectGenerator::IsHarborAllowed(TT_SWAMPLAND),         false);
    BOOST_REQUIRE_EQUAL(ObjectGenerator::IsHarborAllowed(TT_BUILDABLE_WATER),   false);
    BOOST_REQUIRE_EQUAL(ObjectGenerator::IsHarborAllowed(TT_STEPPE),            true);
    BOOST_REQUIRE_EQUAL(ObjectGenerator::IsHarborAllowed(TT_SAVANNAH),          true);
    BOOST_REQUIRE_EQUAL(ObjectGenerator::IsHarborAllowed(TT_MEADOW1),           true);
    BOOST_REQUIRE_EQUAL(ObjectGenerator::IsHarborAllowed(TT_MEADOW2),           true);
    BOOST_REQUIRE_EQUAL(ObjectGenerator::IsHarborAllowed(TT_MEADOW3),           true);
    BOOST_REQUIRE_EQUAL(ObjectGenerator::IsHarborAllowed(TT_MEADOW_FLOWERS),    true);
    BOOST_REQUIRE_EQUAL(ObjectGenerator::IsHarborAllowed(TT_MOUNTAINMEADOW),    true);
}

/**
 * Tests the ObjectGenerator::CreateDuck method with a likelyhood of 100%. A duck should
 * be returned ;-).
 */
BOOST_FIXTURE_TEST_CASE(CreateDuck_FullLikelyhood, ObjectGenerator)
{
    BOOST_REQUIRE_EQUAL(ObjectGenerator::CreateDuck(100), 0x05);
}

/**
 * Tests the ObjectGenerator::CreateDuck method with a likelyhood of 0%. An empty object
 * should be returned.
 */
BOOST_FIXTURE_TEST_CASE(CreateDuck_ZeroLikelyhood, ObjectGenerator)
{
    BOOST_REQUIRE_EQUAL(ObjectGenerator::CreateDuck(0), 0x00);
}

/**
 * Tests the ObjectGenerator::IsTree method for a tree object. Should return true.
 */
BOOST_FIXTURE_TEST_CASE(IsTree_TreeExists, ObjectGenerator)
{
    Map* map = new Map(16, 16, "name", "author");

    map->objectInfo[0] = 0xC5;
    
    BOOST_REQUIRE_EQUAL(ObjectGenerator::IsTree(map, 0), true);

    map->objectInfo[0] = 0xC4;
    
    BOOST_REQUIRE_EQUAL(ObjectGenerator::IsTree(map, 0), true);

    delete map;
}

/**
 * Tests the ObjectGenerator::IsTree method for an empty tile. Should return false.
 */
BOOST_FIXTURE_TEST_CASE(IsTree_Empty, ObjectGenerator)
{
    Map* map = new Map(16, 16, "name", "author");
    
    BOOST_REQUIRE_EQUAL(ObjectGenerator::IsTree(map, 0), false);
    
    delete map;
}

/**
 * Tests the ObjectGenerator::CreateTexture method without harbor.
 * The specified texture should be replaced.
 */
BOOST_FIXTURE_TEST_CASE(CreateTexture_NoHarbor, ObjectGenerator)
{
    Map* map = new Map(16, 16, "name", "author");
    
    ObjectGenerator::CreateTexture(map, 0, TT_WATER, false);
    
    BOOST_REQUIRE_EQUAL(map->textureRsu[0], TerrainData::GetTextureIdentifier(TT_WATER));
    BOOST_REQUIRE_EQUAL(map->textureLsd[0], TerrainData::GetTextureIdentifier(TT_WATER));
    
    delete map;
}

/**
 * Tests the ObjectGenerator::CreateTexture method with harbor.
 * The specified texture should be replaced with a harbor texture.
 */
BOOST_FIXTURE_TEST_CASE(CreateTexture_Harbor, ObjectGenerator)
{
    Map* map = new Map(16, 16, "name", "author");
    
    ObjectGenerator::CreateTexture(map, 0, TT_MEADOW1, true);
    
    BOOST_REQUIRE_EQUAL(map->textureRsu[0], 0x48);
    BOOST_REQUIRE_EQUAL(map->textureLsd[0], 0x48);
    
    delete map;
}

/**
 * Tests the ObjectGenerator::CreateTexture method with harbor but non-harbor texture.
 * The specified texture should be replaced without a harbor texture.
 */
BOOST_FIXTURE_TEST_CASE(CreateTexture_HarborNotSupported, ObjectGenerator)
{
    Map* map = new Map(16, 16, "name", "author");
    
    ObjectGenerator::CreateTexture(map, 0, TT_WATER, true);
    
    BOOST_REQUIRE_EQUAL(map->textureRsu[0], TerrainData::GetTextureIdentifier(TT_WATER));
    BOOST_REQUIRE_EQUAL(map->textureLsd[0], TerrainData::GetTextureIdentifier(TT_WATER));
    
    delete map;
}

BOOST_AUTO_TEST_SUITE_END()

