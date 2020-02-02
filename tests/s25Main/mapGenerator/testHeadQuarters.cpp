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

#include "rttrDefines.h"

#include "mapGenerator/HeadQuarters.h"
#include "mapGenerator/GridUtility.h"
#include "testHelper.hpp"

#include <boost/test/unit_test.hpp>

using namespace rttr::mapGenerator;

BOOST_AUTO_TEST_SUITE(HeadQuartersTests)

// NEW WORLD

BOOST_AUTO_TEST_CASE(FindHqPositions_WithoutSuitablePosition_ReturnsEmpty)
{
    Texture texture;
    DescIdx<LandscapeDesc> landscape;
    MockTextureMapping mapping(landscape);
    MapExtent size(8, 8);
    Map map(mapping, landscape, 0x1, size);
    map.textures.Resize(size, TexturePair(texture));

    mapping.expectedTextures = { texture };
    
    auto positions = FindHqPositions(map, {});
    
    BOOST_REQUIRE(positions.empty());
}

BOOST_AUTO_TEST_CASE(FindHqPositions_ForSinglePlayer_ReturnsSuitablePositions)
{
    Texture texture1(0x1);
    Texture texture2(0x2);
    
    DescIdx<LandscapeDesc> landscape;
    MockTextureMapping mapping(landscape);
    
    MapExtent size(8, 8);
    Map map(mapping, landscape, 0x1, size);
    MapPoint obstacle(0, 0);
    
    map.textures.Resize(size, TexturePair(texture1));
    map.textures[obstacle] = TexturePair(texture2);

    mapping.expectedTextures = { texture2 };
    
    std::vector<MapPoint> positions = FindHqPositions(map, {});
    std::vector<MapPoint> expectedPositions { MapPoint(4,4), MapPoint(4,5) };
    
    BOOST_REQUIRE(positions.size() == expectedPositions.size());
    BOOST_REQUIRE(positions[0] == expectedPositions[0]);
    BOOST_REQUIRE(positions[1] == expectedPositions[1]);
}

BOOST_AUTO_TEST_CASE(PlaceHeadQuarter_ForAreaWithSuitablePosition_ReturnsTrue)
{
    Texture texture1(0x1);
    Texture texture2(0x2);
    
    DescIdx<LandscapeDesc> landscape;
    MockTextureMapping mapping(landscape);
    
    MapExtent size(8, 8);
    Map map(mapping, landscape, 0x1, size);
    MapPoint obstacle(0, 0);
    MapPoint hq(4, 4);

    map.textures.Resize(size, TexturePair(texture1));
    map.textures[obstacle] = TexturePair(texture2);

    mapping.expectedTextures = { texture2 };
    
    auto success = PlaceHeadQuarter(map, 0, { hq });
    
    BOOST_REQUIRE(success);
}

BOOST_AUTO_TEST_CASE(PlaceHeadQuarter_ForAreaWithSuitablePosition_PlacesHqOnMap)
{
    Texture texture1(0x1);
    Texture texture2(0x2);
    
    DescIdx<LandscapeDesc> landscape;
    MockTextureMapping mapping(landscape);
    
    MapExtent size(8, 8);
    Map map(mapping, landscape, 0x1, size);
    MapPoint obstacle(0, 0);
    MapPoint hq(4, 4);
    
    map.textures.Resize(size, TexturePair(texture1));
    map.textures[obstacle] = TexturePair(texture2);

    mapping.expectedTextures = { texture2 };
    
    PlaceHeadQuarter(map, 3, { hq });
    
    BOOST_REQUIRE(map.objectInfos[hq] == libsiedler2::OI_HeadquarterMask);
    BOOST_REQUIRE(map.objectTypes[hq] == libsiedler2::ObjectType(3));
}

BOOST_AUTO_TEST_CASE(PlaceHeadQuarters_ForNPlayersOnEmptyMap_ReturnsTrue)
{
    RandomUtility rnd(0);
    
    Texture texture(0x1);
    DescIdx<LandscapeDesc> landscape;
    MockTextureMapping mapping(landscape);
    MapExtent size(64, 64);

    for (int players = 1; players < 8; players++)
    {
        Map map(mapping, landscape, 0x1, size);
        map.textures.Resize(size, TexturePair(texture));
        
        auto success = PlaceHeadQuarters(map, rnd, players);
        
        BOOST_REQUIRE(success);
    }
}

BOOST_AUTO_TEST_CASE(PlaceHeadQuarters_ForNPlayersOnEmptyMap_PlacesNHqs)
{
    RandomUtility rnd(0);
    
    Texture texture(0x1);
    DescIdx<LandscapeDesc> landscape;
    MockTextureMapping mapping(landscape);
    MapExtent size(64, 64);

    for (int players = 1; players < 8; players++)
    {
        Map map(mapping, landscape, 0x1, size);
        map.textures.Resize(size, TexturePair(texture));
        
        PlaceHeadQuarters(map, rnd, players);
        
        int hqs = 0;
        
        RTTR_FOREACH_PT(MapPoint, size)
        {
            if (map.objectInfos[pt] == libsiedler2::OI_HeadquarterMask)
            {
                hqs++;
            }
        }
        
        BOOST_REQUIRE(hqs == players);
    }
}

// OLD WORLD

BOOST_AUTO_TEST_CASE(FindHeadQuarterPosition_For1stHq_ReturnsCenterOfLargestBuildableArea)
{
    MockTextureMapping_ mapping;
    
    Texture water = mapping.water;
    Texture grass = mapping.grassland;
    Texture coast = mapping.coast;
    Texture lava = mapping.lava;

    WorldDescription worldDesc;
    MapExtent size(8,8);
    Map_ map(worldDesc, size);
    
    Textures textures = {
        water,  water, water, water, water, water, water, water,
        water,  water, water, water, water, water, water, water,
        water,  water, water, water, water, water, water, water,
        coast,  coast, coast, coast, coast, coast, coast, coast,
        coast,  grass, grass, grass, coast, coast, coast, coast,
        coast,  grass, grass, grass, coast, coast, coast, coast,
        coast,  grass, grass, grass, coast, coast, coast, coast,
        lava,   lava,  lava,  lava,  lava,  lava,  lava,  lava
    };
    map.textureLsd = textures;
    map.textureRsu = Textures(textures);
    map.z = {
        0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,
        5,5,5,5,5,5,5,5,
        5,5,5,5,5,5,5,5,
        5,5,5,5,5,5,5,5,
        5,5,5,5,5,5,5,5,
        5,5,5,5,5,5,5,5
    };
    
    auto area = GridPositions(size);
    auto hq = FindHeadQuarterPosition(map, mapping, area);
    
    BOOST_REQUIRE(hq.x == 2 && hq.y == 5);
}

BOOST_AUTO_TEST_CASE(FindHeadQuarterPosition_For2ndHq_ReturnsMaxDistFrom1stHqInBuildableArea)
{
    MockTextureMapping_ mapping;
    
    Texture water = mapping.water;
    Texture coast = mapping.GetCoastTerrain();
    Texture grass = mapping.grassland;
    Texture lava = mapping.lava;
    
    WorldDescription worldDesc;
    MapExtent size(8,8);
    Map_ map(worldDesc, size);
    
    Textures textures {
        water, water, water, water, water, water, water, water,
        water, water, water, water, water, water, water, water,
        water, water, water, water, water, water, water, water,
        coast, grass, grass, grass, grass, grass, grass, coast,
        coast, grass, grass, grass, grass, grass, grass, coast,
        coast, grass, grass, grass, grass, grass, grass, coast,
        coast, coast, coast, coast, coast, coast, coast, coast,
        lava,   lava,  lava,  lava,  lava,  lava,  lava,  lava
    };
    
    map.textureLsd = textures;
    map.textureRsu = Textures(textures);

    map.z = {
        0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,
        5,5,5,5,5,5,5,5,
        5,5,5,5,5,5,5,5,
        5,5,5,5,5,5,5,5,
        5,5,5,5,5,5,5,5,
        5,5,5,5,5,5,5,5
    };
    
    map.hqPositions.push_back(MapPoint(2,4));
    
    auto area = GridPositions(size);
    auto hq = FindHeadQuarterPosition(map, mapping, area);
    
    BOOST_REQUIRE(hq.x == 5 && hq.y == 4);
}


BOOST_AUTO_TEST_CASE(PlaceHeadQuarters_ForMap_CreatesExpectedNumberOfHqs)
{
    MockTextureMapping_ mapping;
    
    Texture water = mapping.water;
    Texture coast = mapping.GetCoastTerrain();
    Texture grass = mapping.grassland;
    Texture lava = mapping.lava;
    
    WorldDescription worldDesc;
    MapExtent size(8,8);
    Map_ map(worldDesc, size);
    
    Textures textures {
        water, water, water, water, water, water, water, water,
        water, water, water, water, water, water, water, water,
        water, water, water, water, water, water, water, water,
        coast, grass, grass, grass, grass, grass, grass, coast,
        coast, grass, grass, grass, grass, grass, grass, coast,
        coast, grass, grass, grass, grass, grass, grass, coast,
        coast, coast, coast, coast, coast, coast, coast, coast,
        lava,   lava,  lava,  lava,  lava,  lava,  lava,  lava
    };
    
    map.textureLsd = textures;
    map.textureRsu = Textures(textures);

    map.z = {
        0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,
        5,5,5,5,5,5,5,5,
        5,5,5,5,5,5,5,5,
        5,5,5,5,5,5,5,5,
        5,5,5,5,5,5,5,5,
        5,5,5,5,5,5,5,5
    };
    
    RandomUtility rnd(0);
    PlaceHeadQuarters(map, mapping, rnd, 2);
    
    for (int i = 0; i < 2; i++)
    {
        BOOST_REQUIRE(map.hqPositions[i].isValid());
        
        int index = map.hqPositions[i].x + map.hqPositions[i].y * size.x;
        
        BOOST_REQUIRE(map.objectType[index] == i);
        BOOST_REQUIRE(map.objectInfo[index] == libsiedler2::OI_HeadquarterMask);
    }
}

BOOST_AUTO_TEST_CASE(PlaceHeadQuarters_ForOneHqInAreaOnMap_PlacesOneHqInSpecifiedArea)
{
    MockTextureMapping_ mapping;
    
    Texture water = mapping.water;
    Texture coast = mapping.GetCoastTerrain();
    Texture grass = mapping.grassland;
    Texture lava = mapping.lava;
    
    WorldDescription worldDesc;
    MapExtent size(8,8);
    Map_ map(worldDesc, size);
    
    Textures textures {
        water, water, water, water, water, water, water, water,
        water, water, water, water, water, water, water, water,
        water, water, water, water, water, water, water, water,
        coast, grass, grass, grass, grass, grass, grass, coast,
        coast, grass, grass, grass, grass, grass, grass, coast,
        coast, grass, grass, grass, grass, grass, grass, coast,
        coast, coast, coast, coast, coast, coast, coast, coast,
        lava,   lava,  lava,  lava,  lava,  lava,  lava,  lava
    };
    
    map.textureLsd = textures;
    map.textureRsu = Textures(textures);

    map.z = {
        0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,
        5,5,5,5,5,5,5,5,
        5,5,5,5,5,5,5,5,
        5,5,5,5,5,5,5,5,
        5,5,5,5,5,5,5,5,
        5,5,5,5,5,5,5,5
    };
    
    std::vector<Position> area = {
        Position(1,3),
        Position(1,4),
        Position(1,5)
    };
    
    PlaceHeadQuarter(map, mapping, 1, area);
    
    auto hqPosition = map.hqPositions[1];
    
    BOOST_REQUIRE(hqPosition.isValid());
 
    int index = hqPosition.x + hqPosition.y * size.x;
        
    BOOST_REQUIRE(map.objectType[index] == 1);
    BOOST_REQUIRE(map.objectInfo[index] == libsiedler2::OI_HeadquarterMask);
    BOOST_REQUIRE(hqPosition.x == 1);
    BOOST_REQUIRE(hqPosition.y == 3 || hqPosition.y == 4 || hqPosition.y == 5);
}

BOOST_AUTO_TEST_SUITE_END()
