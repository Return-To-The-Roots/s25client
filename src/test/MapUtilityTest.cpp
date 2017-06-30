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

#include "defines.h" // IWYU pragma: keep
#include "mapGenerator/MapUtility.h"
#include "mapGenerator/Map.h"
#include "mapGenerator/RandomConfig.h"
#include "mapGenerator/ObjectGenerator.h"
#include "gameData/TerrainData.h"
#include <boost/test/unit_test.hpp>
#include <vector>

namespace{
    class ObjGenFixture{
    protected:
        RandomConfig config;
        ObjectGenerator objGen;
    public:
        ObjGenFixture(): config(RandomConfig::Random, 0x1337), objGen(config){}
    };
}

BOOST_AUTO_TEST_SUITE(MapUtilityTest)

/**
 * Tests the MapUtility::GetBodySize method for a water map.
 */
BOOST_AUTO_TEST_CASE(GetBodySize_Water)
{
    const unsigned width = 16u;
    const unsigned height = 8u;
    const unsigned limit = 300u; // limit is higher than body size (width * height = 128)
    
    Map map(width, height, "map", "author");
    
    for (unsigned i = 0; i < width * height; i++)
    {
        map.textureLsd[i] = TerrainData::GetTextureIdentifier(TT_WATER);
        map.textureRsu[i] = TerrainData::GetTextureIdentifier(TT_WATER);
    }
    
    const Point<int> position(10,10); // any position is valid here, because everything is water
    
    unsigned water = MapUtility::GetBodySize(map, position, limit);
    
    BOOST_REQUIRE_EQUAL(water, width * height);
}

/**
 * Tests the MapUtility::GetBodySize method for a water map with maximum limit.
 */
BOOST_AUTO_TEST_CASE(GetBodySize_Limit)
{
    const unsigned width = 16u;
    const unsigned height = 8u;
    const unsigned limit = 100u; // limit is lower than body size (width * height = 128)

    Map map(width, height, "map", "author");
    for (unsigned i = 0; i < width * height; i++)
    {
        map.textureLsd[i] = TerrainData::GetTextureIdentifier(TT_WATER);
        map.textureRsu[i] = TerrainData::GetTextureIdentifier(TT_WATER);
    }
    
    const Point<int> position(10,10);
    unsigned water = MapUtility::GetBodySize(map, position, limit);
    
    BOOST_REQUIRE_EQUAL(water, limit);
}

/**
 * Tests the MapUtility::SetHill method.
 */
BOOST_AUTO_TEST_CASE(SetHill_Height)
{
    const unsigned width = 16u;
    const unsigned height = 8u;
    const unsigned z = 4u; // height of the hill
    const Point<int> position(0,0);

    Map map(width, height, "map", "author");
    
    MapUtility::SetHill(map, position, z);
    
    BOOST_REQUIRE_EQUAL(map.z[0], z);
}

/**
 * Tests the MapUtility::Smooth method to ensure mountain-meadow textures are
 * replaced by meadow if they have no neighboring mountain-textures.
 */
BOOST_AUTO_TEST_CASE(Smooth_MountainMeadowReplaced)
{
    const unsigned width = 16u;
    const unsigned height = 8u;

    Map map(width, height, "map", "author");
    
    for (unsigned i = 0; i < width * height; i++)
    {
        map.textureLsd[i] = TerrainData::GetTextureIdentifier(TT_MOUNTAINMEADOW);
        map.textureRsu[i] = TerrainData::GetTextureIdentifier(TT_MOUNTAINMEADOW);
    }
    
    MapUtility::Smooth(map);

    for (unsigned i = 0; i < width * height; i++)
    {
        BOOST_REQUIRE_EQUAL(map.textureLsd[i],
                            TerrainData::GetTextureIdentifier(TT_MEADOW1));
        BOOST_REQUIRE_EQUAL(map.textureRsu[i],
                            TerrainData::GetTextureIdentifier(TT_MEADOW1));
    }
}

/**
 * Tests the MapUtility::Smooth method to ensure mountain-meadow textures are
 * not replaced if they have neighboring mountain-textures.
 */
BOOST_AUTO_TEST_CASE(Smooth_MountainMeadowNotReplaced)
{
    const unsigned width = 16u;
    const unsigned height = 8u;
    
    Map map(width, height, "map", "author");
    
    for (unsigned i = 0; i < width * height; i++)
    {
        if (i % width == 0) // mountain-meadow on the left
        {
            map.textureLsd[i] = TerrainData::GetTextureIdentifier(TT_MOUNTAINMEADOW);
            map.textureRsu[i] = TerrainData::GetTextureIdentifier(TT_MOUNTAINMEADOW);
        }
        else // everything else mountains
        {
            map.textureLsd[i] = TerrainData::GetTextureIdentifier(TT_MOUNTAIN1);
            map.textureRsu[i] = TerrainData::GetTextureIdentifier(TT_MOUNTAIN1);
        }
    }
    
    MapUtility::Smooth(map);
    
    for (unsigned i = 0; i < width * height; i++)
    {
        if (i % width == 0)
        {
            BOOST_REQUIRE_EQUAL(map.textureLsd[i],
                                TerrainData::GetTextureIdentifier(TT_MOUNTAINMEADOW));
            BOOST_REQUIRE_EQUAL(map.textureRsu[i],
                                TerrainData::GetTextureIdentifier(TT_MOUNTAINMEADOW));
        }
    }
}

/**
 * Tests the MapUtility::Smooth method to ensure that height of mountain-textures
 * is increased.
 */
BOOST_AUTO_TEST_CASE(Smooth_MountainIncreased)
{
    const unsigned width = 16u;
    const unsigned height = 8u;
    const unsigned z = 11u; // height of the mountain
    
    Map map(width, height, "map", "author");
    
    for (unsigned i = 0; i < width * height; i++)
    {
        map.z[i] = z;
        map.textureLsd[i] = TerrainData::GetTextureIdentifier(TT_MOUNTAIN1);
        map.textureRsu[i] = TerrainData::GetTextureIdentifier(TT_MOUNTAIN1);
    }
    
    MapUtility::Smooth(map);
    
    for (unsigned i = 0; i < width * height; i++)
    {
        BOOST_REQUIRE_GT(map.z[i], z); // mountain got increased for better visual appearance
    }
}

/**
 * Tests the MapUtility::Smooth method to ensure that height of snow-textures
 * is increased.
 */
BOOST_AUTO_TEST_CASE(Smooth_SnowIncreased)
{
    const unsigned width = 16u;
    const unsigned height = 8u;
    const unsigned z = 11u; // height of the snow area
    
    Map map(width, height, "map", "author");
    
    for (unsigned i = 0; i < width * height; i++)
    {
        map.z[i] = z;
        map.textureLsd[i] = TerrainData::GetTextureIdentifier(TT_SNOW);
        map.textureRsu[i] = TerrainData::GetTextureIdentifier(TT_SNOW);
    }
    
    MapUtility::Smooth(map);
    
    for (unsigned i = 0; i < width * height; i++)
    {
        // snow got increased for better visual appearance:
        // this assumes that snow is always placed on top of a mountain area,
        // so its z-value is increased along with the mountain area
        BOOST_REQUIRE_GT(map.z[i], z);
    }
}

/**
 * Tests the MapUtility::Smooth method to ensure that height of meadow-textures
 * are NOT increased.
 */
BOOST_AUTO_TEST_CASE(Smooth_MeadowNotIncreased)
{
    const unsigned width = 16u;
    const unsigned height = 8u;
    const unsigned z = 11u; // height of the meadow area
    
    Map map(width, height, "map", "author");
    
    for (unsigned i = 0; i < width * height; i++)
    {
        map.z[i] = z;
        map.textureLsd[i] = TerrainData::GetTextureIdentifier(TT_MEADOW1);
        map.textureRsu[i] = TerrainData::GetTextureIdentifier(TT_MEADOW1);
    }
    
    MapUtility::Smooth(map);
    
    for (unsigned i = 0; i < width * height; i++)
    {
        // texture other than snow/mountain remain their z values
        BOOST_REQUIRE_EQUAL(map.z[i], z);
    }
}

/**
 * Tests the MapUtility::Smooth method to ensure that single textures which are surounded
 * by other textures are replaced properly.
 */
BOOST_AUTO_TEST_CASE(Smooth_SingleTexturesReplaced)
{
    const unsigned width = 16u;
    const unsigned height = 8u;
    
    Map map(width, height, "map", "author");
    
    for (unsigned i = 0; i < width * height; i++)
    {
        map.textureLsd[i] = TerrainData::GetTextureIdentifier(TT_MEADOW1);
        map.textureRsu[i] = TerrainData::GetTextureIdentifier(TT_MEADOW1);
    }
    map.textureRsu[0] = TT_SNOW;
    
    MapUtility::Smooth(map);
    
    BOOST_REQUIRE_EQUAL(map.textureRsu[0], TerrainData::GetTextureIdentifier(TT_MEADOW1));
}

/**
 * Tests the MapUtility::SetHarbor method to ensure harbor positions are available after
 * placing a harbor in a suitable position.
 */
BOOST_AUTO_TEST_CASE(SetHarbor_HarborPlaceAvailable)
{
    const unsigned width = 16u;
    const unsigned height = 8u;
    
    Map map(width, height, "map", "author");
    
    for (unsigned i = 0; i < width * height; i++)
    {
        if (i % width < width / 2)
        {
            // half of the map meadow
            map.textureLsd[i] = TerrainData::GetTextureIdentifier(TT_MEADOW1);
            map.textureRsu[i] = TerrainData::GetTextureIdentifier(TT_MEADOW1);
        }
        else
        {
            // half of the map water
            map.textureLsd[i] = TerrainData::GetTextureIdentifier(TT_WATER);
            map.textureRsu[i] = TerrainData::GetTextureIdentifier(TT_WATER);
        }
    }
    
    // place harbor in the center of the map
    const Point<int> center(width / 2 - 1, height / 2 - 1);
    MapUtility::SetHarbour(map, center, 0);
    
    int countHarbors = 0;
    for (unsigned i = 0; i < width * height; i++)
    {
        if ((map.textureLsd[i] & libsiedler2::HARBOR_MASK) &&
            (map.textureRsu[i] & libsiedler2::HARBOR_MASK))
        {
            countHarbors++;
        }
    }
    
    BOOST_REQUIRE_GT(countHarbors, 0);
}

/**
 * Tests the MapUtility::SetTree for an empty map. As a result the position for the new 
 * tree shouldn't be empty anymore.
 */
BOOST_FIXTURE_TEST_CASE(SetTree_EmptyTerrain, ObjGenFixture)
{
    const unsigned width = 16u;
    const unsigned height = 8u;
    
    Map map(width, height, "map", "author");
    
    Point<int> p(width/2, height/2);
    MapUtility::SetTree(map, objGen, p);

    BOOST_REQUIRE_NE(map.objectType[p.y * width + p.x], libsiedler2::OT_Empty);
    BOOST_REQUIRE_NE(map.objectInfo[p.y * width + p.x], libsiedler2::OI_Empty);
}

/**
 * Tests the MapUtility::SetTree for a desert map. As a result the position for the new
 * tree shouldn't be empty anymore.
 */
BOOST_FIXTURE_TEST_CASE(SetTree_DesertTerrain, ObjGenFixture)
{
    const unsigned width = 16u;
    const unsigned height = 8u;
    
    Map map(width, height, "map", "author");
    
    for (unsigned i = 0; i < width * height; i++)
    {
        map.textureLsd[i] = TT_DESERT;
        map.textureRsu[i] = TT_DESERT;
    }
    
    Point<int> p(width/2, height/2);
    MapUtility::SetTree(map, objGen, p);
    
    BOOST_REQUIRE_NE(map.objectType[p.y * width + p.x], libsiedler2::OT_Empty);
    BOOST_REQUIRE_NE(map.objectInfo[p.y * width + p.x], libsiedler2::OI_Empty);
}

/**
 * Tests the MapUtility::SetTree for a non-empty map. As a result the position for the new
 * tree shouldn't be replaced.
 */
BOOST_FIXTURE_TEST_CASE(SetTree_NonEmptyTerrain, ObjGenFixture)
{
    const unsigned width = 16u;
    const unsigned height = 8u;
    Point<int> p(width/2, height/2);
    const int index = p.y * width + p.x;
    
    Map map(width, height, "map", "author");
    
    map.objectType[index] = libsiedler2::OT_Stone_Begin;
    map.objectInfo[index] = libsiedler2::OI_Stone1;
    
    MapUtility::SetTree(map, objGen, p);
    
    BOOST_REQUIRE_EQUAL(map.objectType[index], libsiedler2::OT_Stone_Begin);
    BOOST_REQUIRE_EQUAL(map.objectInfo[index], libsiedler2::OI_Stone1);
}

/**
 * Tests the MapUtility::SetStone for an empty map. As a result the position for the new
 * tree shouldn't be empty anymore.
 */
BOOST_FIXTURE_TEST_CASE(SetStone_EmptyTerrain, ObjGenFixture)
{
    const unsigned width = 16u;
    const unsigned height = 8u;
    
    Map map(width, height, "map", "author");
    
    Point<int> p(width/2, height/2);
    MapUtility::SetStone(map, objGen, p);
    
    BOOST_REQUIRE_NE(map.objectType[p.y * width + p.x], libsiedler2::OT_Empty);
    BOOST_REQUIRE_NE(map.objectInfo[p.y * width + p.x], libsiedler2::OI_Empty);
}

/**
 * Tests the MapUtility::SetStone for a non-empty map. As a result the position for the new
 * stone pile shouldn't be replaced.
 */
BOOST_FIXTURE_TEST_CASE(SetStone_NonEmptyTerrain, ObjGenFixture)
{
    const unsigned width = 16u;
    const unsigned height = 8u;
    Point<int> p(width/2, height/2);
    const int index = p.y * width + p.x;
    
    Map map(width, height, "map", "author");
    
    map.objectType[index] = libsiedler2::OT_Tree1_Begin;
    map.objectInfo[index] = libsiedler2::OI_TreeOrPalm;
    
    MapUtility::SetStone(map, objGen, p);
    
    BOOST_REQUIRE_EQUAL(map.objectType[index], libsiedler2::OT_Tree1_Begin);
    BOOST_REQUIRE_EQUAL(map.objectInfo[index], libsiedler2::OI_TreeOrPalm);
}

/**
 * Tests the VertexUtility::ComputePointOnCircle method with fixed values around a circle.
 */
BOOST_AUTO_TEST_CASE(ComputePointOnCircle_FixedValues)
{
    Point<int> p1 = MapUtility::ComputePointOnCircle(0, 360, Point<int>(1,1), 1.0);

    BOOST_REQUIRE_EQUAL(p1.x, 0x2);
    BOOST_REQUIRE_EQUAL(p1.y, 0x1);

    Point<int> p2 = MapUtility::ComputePointOnCircle(90, 360, Point<int>(1,1), 1.0);

    BOOST_REQUIRE_EQUAL(p2.x, 0x1);
    BOOST_REQUIRE_EQUAL(p2.y, 0x2);
}

BOOST_AUTO_TEST_SUITE_END()

