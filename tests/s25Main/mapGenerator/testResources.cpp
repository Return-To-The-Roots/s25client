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

#include "mapGenerator/Resources.h"
#include "mapGenerator/Textures.h"
#include "testHelper.hpp"

#include <boost/test/unit_test.hpp>

using namespace rttr::mapGenerator;
using namespace libsiedler2;

BOOST_AUTO_TEST_SUITE(ResourcesTests)

BOOST_AUTO_TEST_CASE(IsHeadQuarterOrHarborPosition_ForHqIndex_ReturnsTrue)
{
    const int HqIndex = 4;
    
    WorldDescription worldDesc;
    MapExtent size(8,8);
    Map_ map(worldDesc, size);
    MapPoint hq(HqIndex,0);

    map.hqPositions.push_back(hq);
    
    BOOST_REQUIRE(IsHeadQuarterOrHarborPosition(map, HqIndex));
}

BOOST_AUTO_TEST_CASE(IsHeadQuarterOrHarborPosition_ForRsuHarborIndex_ReturnsTrue)
{
    const int HarborIndex = 3;
    
    WorldDescription worldDesc;
    MapExtent size(8,8);
    Map_ map(worldDesc, size);
    
    map.harborsRsu.insert(HarborIndex);
    
    BOOST_REQUIRE(IsHeadQuarterOrHarborPosition(map, HarborIndex));
}

BOOST_AUTO_TEST_CASE(IsHeadQuarterOrHarborPosition_ForLsdHarborIndex_ReturnsTrue)
{
    const int HarborIndex = 5;
    
    WorldDescription worldDesc;
    MapExtent size(8,8);
    Map_ map(worldDesc, size);
    
    map.harborsLsd.insert(HarborIndex);
    
    BOOST_REQUIRE(IsHeadQuarterOrHarborPosition(map, HarborIndex));
}

BOOST_AUTO_TEST_CASE(IsHeadQuarterOrHarborPosition_ForEmptyPosition_ReturnsFalse)
{
    WorldDescription worldDesc;
    MapExtent size(8,8);
    Map_ map(worldDesc, size);
    
    BOOST_REQUIRE(!IsHeadQuarterOrHarborPosition(map, 4));
}

BOOST_AUTO_TEST_CASE(PlaceResources_ForMap_DoesNotAlterTexturesAndHeightMap)
{
    MockTextureMapping_ mapping;
    
    Texture water = mapping.water;
    Texture coast = mapping.GetCoastTerrain();
    Texture grass = mapping.grassland;
    Texture lava = mapping.lava;
    
    WorldDescription worldDesc;
    MapExtent size(8,8);
    Map_ map(worldDesc, size);
    
    Textures originalTextures {
        water, water, water, water, water, water, water, water,
        water, water, water, water, water, water, water, water,
        water, water, water, water, water, water, water, water,
        coast, grass, grass, grass, grass, grass, grass, coast,
        coast, grass, grass, grass, grass, grass, grass, coast,
        coast, grass, grass, grass, grass, grass, grass, coast,
        coast, coast, coast, coast, coast, coast, coast, coast,
        lava,   lava,  lava,  lava,  lava,  lava,  lava,  lava
    };
    
    map.textureLsd = Textures(originalTextures);
    map.textureRsu = Textures(originalTextures);

    HeightMap originalZ {
        0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,
        5,5,5,5,5,5,5,5,
        5,5,5,5,5,5,5,5,
        5,5,5,5,5,5,5,5,
        5,5,5,5,5,5,5,5,
        5,5,5,5,5,5,5,5
    };
    
    map.z = HeightMap(originalZ);
    
    MapSettings settings;
    RandomUtility rnd(0);
    
    PlaceResources(map, rnd, mapping, settings);
    
    for (int i = 0; i < 64; i++)
    {
        BOOST_REQUIRE(map.textureRsu[i] == originalTextures[i]);
        BOOST_REQUIRE(map.textureLsd[i] == originalTextures[i]);
        BOOST_REQUIRE(map.z[i] == originalZ[i]);
    }
}

BOOST_AUTO_TEST_CASE(PlaceAnimals_ForMap_PlacesOnlyDucksInWater)
{
    MockTextureMapping_ mapping;
    
    Texture water = mapping.water;
    Texture coast = mapping.GetCoastTerrain();
    Texture grass = mapping.grassland;
    Texture lava = mapping.lava;
    
    WorldDescription worldDesc;
    MapExtent size(8,8);
    Map_ map(worldDesc, size);
    
    Textures originalTextures {
        water, water, water, water, water, water, water, water,
        water, water, water, water, water, water, water, water,
        water, water, water, water, water, water, water, water,
        coast, grass, grass, grass, grass, grass, grass, coast,
        coast, grass, grass, grass, grass, grass, grass, coast,
        coast, grass, grass, grass, grass, grass, grass, coast,
        coast, coast, coast, coast, coast, coast, coast, coast,
        lava,   lava,  lava,  lava,  lava,  lava,  lava,  lava
    };
    
    map.textureLsd = Textures(originalTextures);
    map.textureRsu = Textures(originalTextures);

    HeightMap originalZ {
        0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,
        5,5,5,5,5,5,5,5,
        5,5,5,5,5,5,5,5,
        5,5,5,5,5,5,5,5,
        5,5,5,5,5,5,5,5,
        5,5,5,5,5,5,5,5
    };
    
    map.z = HeightMap(originalZ);
    
    RandomUtility rnd(0);
    PlaceAnimals(map, rnd, mapping);
    
    for (int i = 0; i < 64; i++)
    {
        if (map.textureRsu[i] == water)
        {
            BOOST_REQUIRE(map.animal[i] == A_Duck ||
                          map.animal[i] == A_Duck2 ||
                          map.animal[i] == A_None);
        }
    }
}

BOOST_AUTO_TEST_CASE(PlaceMinesAndFish_ForMap_AddsFishToWater)
{
    MockTextureMapping_ mapping;
    
    Texture water = mapping.water;
    Texture coast = mapping.GetCoastTerrain();
    Texture grass = mapping.grassland;
    Texture lava = mapping.lava;
    
    WorldDescription worldDesc;
    MapExtent size(8,8);
    Map_ map(worldDesc, size);
    
    Textures originalTextures {
        water, water, water, water, water, water, water, water,
        water, water, water, water, water, water, water, water,
        water, water, water, water, water, water, water, water,
        coast, grass, grass, grass, grass, grass, grass, coast,
        coast, grass, grass, grass, grass, grass, grass, coast,
        coast, grass, grass, grass, grass, grass, grass, coast,
        coast, coast, coast, coast, coast, coast, coast, coast,
        lava,   lava,  lava,  lava,  lava,  lava,  lava,  lava
    };
    
    map.textureLsd = Textures(originalTextures);
    map.textureRsu = Textures(originalTextures);

    HeightMap originalZ {
        0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,
        5,5,5,5,5,5,5,5,
        5,5,5,5,5,5,5,5,
        5,5,5,5,5,5,5,5,
        5,5,5,5,5,5,5,5,
        5,5,5,5,5,5,5,5
    };
    
    map.z = HeightMap(originalZ);
    
    MapSettings settings;
    RandomUtility rnd(0);
    
    PlaceMinesAndFish(map, rnd, mapping, settings);
    
    for (int i = 0; i < 64; i++)
    {
        if (originalTextures[i] == water)
        {
            BOOST_REQUIRE(map.resource[i] == libsiedler2::R_Fish);
        }
    }
}

BOOST_AUTO_TEST_CASE(PlaceMinesAndFish_ForMap_AddsResourcesToMountains)
{
    MockTextureMapping_ mapping;
    
    Texture water = mapping.water;
    Texture coast = mapping.GetCoastTerrain();
    Texture mountain = mapping.mountain;
    Texture grass = mapping.grassland;
    Texture lava = mapping.lava;
    
    Textures originalTextures {
        water, water, water, water, water, water, water, water,
        water, water, water, water, water, water, water, water,
        water, water, water, water, water, water, water, water,
        mountain,mountain,mountain,mountain,mountain,grass,grass,grass,
        mountain,mountain,mountain,mountain,mountain,grass,grass,grass,
        coast, grass, grass, grass, grass, grass, grass, coast,
        coast, coast, coast, coast, coast, coast, coast, coast,
        lava, lava, lava, lava, lava, lava, lava, lava };

    HeightMap originalZ {
        0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,
        5,5,5,5,5,5,5,5,
        5,5,5,5,5,5,5,5,
        5,5,5,5,5,5,5,5,
        5,5,5,5,5,5,5,5,
        5,5,5,5,5,5,5,5 };
    
    WorldDescription worldDesc;
    MapExtent size(8,8);
    Map_ map(worldDesc, size);
    
    map.textureLsd = Textures(originalTextures);
    map.textureRsu = Textures(originalTextures);
    map.z = HeightMap(originalZ);
    
    MapSettings settings;
    RandomUtility rnd(0);
    
    PlaceMinesAndFish(map, rnd, mapping, settings);

    for (int i = 0; i < 64; i++)
    {
        if (mapping.IsMountain(originalTextures[i]))
        {
            bool isCoal = map.resource[i] >= libsiedler2::R_Coal &&
                          map.resource[i] <= libsiedler2::R_Coal + 8;
            bool isGold = map.resource[i] >= libsiedler2::R_Gold &&
                          map.resource[i] <= libsiedler2::R_Gold + 8;
            bool isIron = map.resource[i] >= libsiedler2::R_Iron &&
                          map.resource[i] <= libsiedler2::R_Iron + 8;
            bool isGranite = map.resource[i] >= libsiedler2::R_Granite &&
                             map.resource[i] <= libsiedler2::R_Granite + 8;

            BOOST_REQUIRE(isCoal || isGold || isIron || isGranite);
        }
    }
}


BOOST_AUTO_TEST_CASE(PlaceStones_ForMap_DoesNotPlaceAnyStoneOnWater)
{
    MockTextureMapping_ mapping;
    
    Texture water = mapping.water;
    Texture coast = mapping.GetCoastTerrain();
    Texture mountain = mapping.mountain;
    Texture grass = mapping.grassland;
    Texture lava = mapping.lava;
    
    Textures originalTextures {
        water, water, water, water, water, water, water, water,
        water, water, water, water, water, water, water, water,
        water, water, water, water, water, water, water, water,
        mountain,mountain,mountain,mountain,mountain,grass,grass,grass,
        mountain,mountain,mountain,mountain,mountain,grass,grass,grass,
        coast, grass, grass, grass, grass, grass, grass, coast,
        coast, coast, coast, coast, coast, coast, coast, coast,
        lava, lava, lava, lava, lava, lava, lava, lava };
    
    HeightMap originalZ {
        0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,
        5,5,5,5,5,5,5,5,
        5,5,5,5,5,5,5,5,
        5,5,5,5,5,5,5,5,
        5,5,5,5,5,5,5,5,
        5,5,5,5,5,5,5,5 };
    
    WorldDescription worldDesc;
    MapExtent size(8,8);
    Map_ map(worldDesc, size);
    
    map.textureLsd = Textures(originalTextures);
    map.textureRsu = Textures(originalTextures);
    map.z = HeightMap(originalZ);
    
    std::vector<int> freeZone(64, 11);
    RandomUtility rnd(0);
    
    PlaceStones(map, rnd, mapping, freeZone);
    
    for (int i = 0; i < 64; i++)
    {
        if (originalTextures[i] == water)
        {
            BOOST_REQUIRE(map.objectInfo[i] != 0xCC &&
                          map.objectInfo[i] != 0xCD);
        }
    }
}

BOOST_AUTO_TEST_CASE(PlaceTrees_ForMap_DoesNotPlaceAnyTreeOnWater)
{
    MockTextureMapping_ mapping;
    
    Texture water = mapping.water;
    Texture coast = mapping.GetCoastTerrain();
    Texture mountain = mapping.mountain;
    Texture grass = mapping.grassland;
    Texture lava = mapping.lava;
    
    Textures originalTextures {
        water, water, water, water, water, water, water, water,
        water, water, water, water, water, water, water, water,
        water, water, water, water, water, water, water, water,
        mountain,mountain,mountain,mountain,mountain,grass,grass,grass,
        mountain,mountain,mountain,mountain,mountain,grass,grass,grass,
        coast, grass, grass, grass, grass, grass, grass, coast,
        coast, coast, coast, coast, coast, coast, coast, coast,
        lava, lava, lava, lava, lava, lava, lava, lava };
    
    HeightMap originalZ = {
        0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,
        5,5,5,5,5,5,5,5,
        5,5,5,5,5,5,5,5,
        5,5,5,5,5,5,5,5,
        5,5,5,5,5,5,5,5,
        5,5,5,5,5,5,5,5 };
    
    WorldDescription worldDesc;
    MapExtent size(8,8);
    Map_ map(worldDesc, size);
    
    map.textureLsd = Textures(originalTextures);
    map.textureRsu = Textures(originalTextures);
    map.z = HeightMap(originalZ);
    
    std::vector<int> freeZone(64, 11);
    RandomUtility rnd(0);
    
    PlaceTrees(map, rnd, mapping, freeZone);
    
    for (int i = 0; i < 64; i++)
    {
        if (originalTextures[i] == water)
        {
            BOOST_REQUIRE(map.objectInfo[i] == 0x0 &&
                          map.objectType[i] == 0x0);
        }
    }
}

BOOST_AUTO_TEST_SUITE_END()
