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

#include "mapGenerator/MapUtility.h"
#include "mapGenerator/Map.h"
#include "gameData/TerrainData.h"
#include <boost/test/unit_test.hpp>
#include <vector>

BOOST_AUTO_TEST_SUITE(MapUtilityTest)

/**
 * Tests the MapUtility::ComputeWaterSize method for no water.
 */
BOOST_FIXTURE_TEST_CASE(ComputeWaterSize_NoWater, MapUtility)
{
    Map* map = new Map(64, 64, "map", "author");

    const int water = MapUtility::ComputeWaterSize(map, Vec2(10, 10), 100);
    
    BOOST_REQUIRE_EQUAL(water, 0);
    
    delete map;
}

/**
 * Tests the MapUtility::ComputeWaterSize method for a water map.
 */
BOOST_FIXTURE_TEST_CASE(ComputeWaterSize_Water, MapUtility)
{
    Map* map = new Map(16, 16, "map", "author");
    for (int i = 0; i < 256; i++) map->textureLsd[i] = TerrainData::GetTextureIdentifier(TT_WATER);
    for (int i = 0; i < 256; i++) map->textureRsu[i] = TerrainData::GetTextureIdentifier(TT_WATER);
    
    const int water = MapUtility::ComputeWaterSize(map, Vec2(10, 10), 300);
    
    BOOST_REQUIRE_EQUAL(water, 256);
    
    delete map;
}

/**
 * Tests the MapUtility::ComputeWaterSize method for a water map with maximum limit.
 */
BOOST_FIXTURE_TEST_CASE(ComputeWaterSize_Limit, MapUtility)
{
    Map* map = new Map(16, 16, "map", "author");
    for (int i = 0; i < 256; i++) map->textureLsd[i] = TerrainData::GetTextureIdentifier(TT_WATER);
    for (int i = 0; i < 256; i++) map->textureRsu[i] = TerrainData::GetTextureIdentifier(TT_WATER);
    
    const int water = MapUtility::ComputeWaterSize(map, Vec2(10, 10), 100);
    
    BOOST_REQUIRE_EQUAL(water, 100);
    
    delete map;
}

/**
 * Tests the MapUtility::SetHill method.
 */
BOOST_FIXTURE_TEST_CASE(SetHill_Height, MapUtility)
{
    Map* map = new Map(16, 16, "map", "author");
    
    MapUtility::SetHill(map, Vec2(0, 0), 0x4);
    
    BOOST_REQUIRE_EQUAL(map->z[0], 0x4);
    
    delete map;
}

/**
 * Tests the MapUtility::Smooth method to ensure mountain-meadow textures are
 * replaced by meadow if they have no neighboring mountain-textures.
 */
BOOST_FIXTURE_TEST_CASE(Smooth_MountainMeadowReplaced, MapUtility)
{
    Map* map = new Map(16, 16, "map", "author");
    
    for (int i = 0; i < map->width * map->height; i++)
    {
        map->textureLsd[i] = TT_MOUNTAINMEADOW;
        map->textureRsu[i] = TT_MOUNTAINMEADOW;
    }
    
    MapUtility::Smooth(map);

    for (int i = 0; i < map->width * map->height; i++)
    {
        BOOST_REQUIRE_EQUAL(map->textureLsd[i], TerrainData::GetTextureIdentifier(TT_MEADOW1));
        BOOST_REQUIRE_EQUAL(map->textureRsu[i], TerrainData::GetTextureIdentifier(TT_MEADOW1));
    }
    
    delete map;
}

/**
 * Tests the MapUtility::Smooth method to ensure mountain-meadow textures are
 * not replaced if they have neighboring mountain-textures.
 */
BOOST_FIXTURE_TEST_CASE(Smooth_MountainMeadowNotReplaced, MapUtility)
{
    Map* map = new Map(16, 16, "map", "author");
    
    for (int i = 0; i < map->width * map->height; i++)
    {
        if (i % map->width == 0) // mountain-meadow on the left
        {
            map->textureLsd[i] = TerrainData::GetTextureIdentifier(TT_MOUNTAINMEADOW);
            map->textureRsu[i] = TerrainData::GetTextureIdentifier(TT_MOUNTAINMEADOW);
        }
        else // everything else mountains
        {
            map->textureLsd[i] = TerrainData::GetTextureIdentifier(TT_MOUNTAIN1);
            map->textureRsu[i] = TerrainData::GetTextureIdentifier(TT_MOUNTAIN1);
        }
    }
    
    MapUtility::Smooth(map);
    
    for (int i = 0; i < map->width * map->height; i++)
    {
        if (i % map->width == 0)
        {
            BOOST_REQUIRE_EQUAL(map->textureLsd[i], TerrainData::GetTextureIdentifier(TT_MOUNTAINMEADOW));
            BOOST_REQUIRE_EQUAL(map->textureRsu[i], TerrainData::GetTextureIdentifier(TT_MOUNTAINMEADOW));
        }
    }
    
    delete map;
}

/**
 * Tests the MapUtility::Smooth method to ensure that height of mountain-textures
 * is increased.
 */
BOOST_FIXTURE_TEST_CASE(Smooth_MountainIncreased, MapUtility)
{
    Map* map = new Map(16, 16, "map", "author");
    
    for (int i = 0; i < map->width * map->height; i++)
    {
        map->z[i] = 0x0A;
        map->textureLsd[i] = TerrainData::GetTextureIdentifier(TT_MOUNTAIN1);
        map->textureRsu[i] = TerrainData::GetTextureIdentifier(TT_MOUNTAIN1);
    }
    
    MapUtility::Smooth(map);
    
    for (int i = 0; i < map->width * map->height; i++)
    {
        BOOST_REQUIRE_GT(map->z[i], 0x0A);
    }
    
    delete map;
}

/**
 * Tests the MapUtility::Smooth method to ensure that height of snow-textures
 * is increased.
 */
BOOST_FIXTURE_TEST_CASE(Smooth_SnowIncreased, MapUtility)
{
    Map* map = new Map(16, 16, "map", "author");
    
    for (int i = 0; i < map->width * map->height; i++)
    {
        map->z[i] = 0x0A;
        map->textureLsd[i] = TerrainData::GetTextureIdentifier(TT_SNOW);
        map->textureRsu[i] = TerrainData::GetTextureIdentifier(TT_SNOW);
    }
    
    MapUtility::Smooth(map);
    
    for (int i = 0; i < map->width * map->height; i++)
    {
        BOOST_REQUIRE_GT(map->z[i], 0x0A);
    }
    
    delete map;
}

/**
 * Tests the MapUtility::Smooth method to ensure that height of meadow-textures
 * are NOT increased.
 */
BOOST_FIXTURE_TEST_CASE(Smooth_MeadowNotIncreased, MapUtility)
{
    Map* map = new Map(16, 16, "map", "author");
    
    for (int i = 0; i < map->width * map->height; i++)
    {
        map->z[i] = 0x0A;
        map->textureLsd[i] = TerrainData::GetTextureIdentifier(TT_MEADOW1);
        map->textureRsu[i] = TerrainData::GetTextureIdentifier(TT_MEADOW1);
    }
    
    MapUtility::Smooth(map);
    
    for (int i = 0; i < map->width * map->height; i++)
    {
        BOOST_REQUIRE_EQUAL(map->z[i], 0x0A);
    }
    
    delete map;
}

/**
 * Tests the MapUtility::Smooth method to ensure that single textures which are surounded
 * by other textures are replaced properly.
 */
BOOST_FIXTURE_TEST_CASE(Smooth_SingleTexturesReplaced, MapUtility)
{
    Map* map = new Map(16, 16, "map", "author");
    
    for (int i = 0; i < map->width * map->height; i++)
    {
        map->textureLsd[i] = TerrainData::GetTextureIdentifier(TT_MEADOW1);
        map->textureRsu[i] = TerrainData::GetTextureIdentifier(TT_MEADOW1);
    }
    map->textureRsu[0] = TT_SNOW;
    
    MapUtility::Smooth(map);
    
    BOOST_REQUIRE_EQUAL(map->textureRsu[0], TerrainData::GetTextureIdentifier(TT_MEADOW1));
    
    delete map;
}

/**
 * Tests the MapUtility::SetHarbor method to ensure harbor positions are available after
 * placing a harbor at an apropriate place.
 */
BOOST_FIXTURE_TEST_CASE(SetHarbor_HarborPlaceAvailable, MapUtility)
{
    Map* map = new Map(16, 16, "map", "author");
    
    for (int i = 0; i < map->width * map->height; i++)
    {
        if (i % map->width < map->width / 2)
        {
            map->textureLsd[i] = TerrainData::GetTextureIdentifier(TT_MEADOW1); // half of the map meadow
            map->textureRsu[i] = TerrainData::GetTextureIdentifier(TT_MEADOW1);
        }
        else
        {
            map->textureLsd[i] = TerrainData::GetTextureIdentifier(TT_WATER); // half of the map water
            map->textureRsu[i] = TerrainData::GetTextureIdentifier(TT_WATER);
        }
    }
    
    MapUtility::SetHarbour(map, Vec2(7, 7), 0);
    
    int countHarbors = 0;
    for (int i = 0; i < map->width * map->height; i++)
    {
        if (map->textureLsd[i] == 0x40 && map->textureRsu[i] == 0x40)
        {
            countHarbors++;
        }
    }
    
    BOOST_REQUIRE_GT(countHarbors, 0);
    
    delete map;
}

/**
 * Tests the MapUtility::SetTree for an empty map. As a result the position for the new 
 * tree shouldn't be empty anymore.
 */
BOOST_FIXTURE_TEST_CASE(SetTree_EmptyTerrain, MapUtility)
{
    Map* map = new Map(16, 16, "map", "author");
    
    MapUtility::SetTree(map, Vec2(8,8));

    BOOST_REQUIRE_NE(map->objectType[8 * 16 + 8], 0x0);
    BOOST_REQUIRE_NE(map->objectInfo[8 * 16 + 8], 0x0);
    
    delete map;
}

/**
 * Tests the MapUtility::SetTree for a desert map. As a result the position for the new
 * tree shouldn't be empty anymore.
 */
BOOST_FIXTURE_TEST_CASE(SetTree_DesertTerrain, MapUtility)
{
    Map* map = new Map(16, 16, "map", "author");
    
    for (int i = 0; i < map->width * map->height; i++)
    {
        map->textureLsd[i] = TT_DESERT;
        map->textureRsu[i] = TT_DESERT;
    }
    
    MapUtility::SetTree(map, Vec2(8,8));
    
    BOOST_REQUIRE_NE(map->objectType[8 * 16 + 8], 0x0);
    BOOST_REQUIRE_NE(map->objectInfo[8 * 16 + 8], 0x0);
    
    delete map;
}

/**
 * Tests the MapUtility::SetTree for a non-empty map. As a result the position for the new
 * tree shouldn't be replaced.
 */
BOOST_FIXTURE_TEST_CASE(SetTree_NonEmptyTerrain, MapUtility)
{
    Map* map = new Map(16, 16, "map", "author");
    const int index = 8 * 16 + 8;
    
    map->objectType[index] = 0x1;
    map->objectInfo[index] = 0x1;
    
    MapUtility::SetTree(map, Vec2(8,8));
    
    BOOST_REQUIRE_EQUAL(map->objectType[index], 0x1);
    BOOST_REQUIRE_EQUAL(map->objectInfo[index], 0x1);
    
    delete map;
}

/**
 * Tests the MapUtility::SetStone for an empty map. As a result the position for the new
 * tree shouldn't be empty anymore.
 */
BOOST_FIXTURE_TEST_CASE(SetStone_EmptyTerrain, MapUtility)
{
    Map* map = new Map(16, 16, "map", "author");
    
    MapUtility::SetStone(map, Vec2(8,8));
    
    BOOST_REQUIRE_NE(map->objectType[8 * 16 + 8], 0x0);
    BOOST_REQUIRE_NE(map->objectInfo[8 * 16 + 8], 0x0);
    
    delete map;
}

/**
 * Tests the MapUtility::SetStone for a non-empty map. As a result the position for the new
 * stone pile shouldn't be replaced.
 */
BOOST_FIXTURE_TEST_CASE(SetStone_NonEmptyTerrain, MapUtility)
{
    Map* map = new Map(16, 16, "map", "author");
    const int index = 8 * 16 + 8;
    
    map->objectType[index] = 0x1;
    map->objectInfo[index] = 0x1;
    
    MapUtility::SetStone(map, Vec2(8,8));
    
    BOOST_REQUIRE_EQUAL(map->objectType[index], 0x1);
    BOOST_REQUIRE_EQUAL(map->objectInfo[index], 0x1);
    
    delete map;
}

/**
 * Tests the VertexUtility::ComputePointOnCircle method with fixed values around a circle.
 */
BOOST_FIXTURE_TEST_CASE(ComputePointOnCircle_FixedValues, MapUtility)
{
    const Vec2 p1 = MapUtility::ComputePointOnCircle(0, 360, Vec2(1,1), 1.0);

    BOOST_REQUIRE_EQUAL(p1.x, 0x2);
    BOOST_REQUIRE_EQUAL(p1.y, 0x1);

    const Vec2 p2 = MapUtility::ComputePointOnCircle(90, 360, Vec2(1,1), 1.0);

    BOOST_REQUIRE_EQUAL(p2.x, 0x1);
    BOOST_REQUIRE_EQUAL(p2.y, 0x2);
}

BOOST_AUTO_TEST_SUITE_END()

