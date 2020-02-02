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

#include "mapGenerator/Harbors.h"
#include "mapGenerator/Textures.h"
#include "mapGenerator/GridUtility.h"
#include "testHelper.hpp"

#include <boost/test/unit_test.hpp>

using namespace rttr::mapGenerator;

BOOST_AUTO_TEST_SUITE(HarborsTests)

// NEW WORLD

BOOST_AUTO_TEST_CASE(FindIslands_ForIslandBelowMinimumSize_ReturnsNoIsland)
{
    Texture water(0x1);
    Texture land(0x2);
    
    DescIdx<LandscapeDesc> landscape;
    MockTextureMapping mapping(landscape);
    MapExtent size(16,16);
    MapPoint island(3,3);
    Map map(mapping, landscape, 0x1, size);

    mapping.expectedTextures = { land };
    map.textures.Resize(size, water);
    map.textures[island] = TexturePair(land); // 4 nodes with land
    
    auto islands = FindIslands(map, 5);
    
    BOOST_REQUIRE(islands.empty());
}

BOOST_AUTO_TEST_CASE(FindIslands_ForMultipleIslands_ReturnsIslandsWithMinimumSize)
{
    Texture water(0x1);
    Texture land(0x2);
    
    DescIdx<LandscapeDesc> landscape;
    MockTextureMapping mapping(landscape);
    MapExtent size(16,16);
    Map map(mapping, landscape, 0x1, size);

    mapping.expectedTextures = { land };
    map.textures.Resize(size, water);

    MapPoint tinyIsland(3, 3);
    std::vector<MapPoint> otherIslands {
        MapPoint(7, 7), MapPoint(11, 11)
    };
    
    map.textures[tinyIsland] = TexturePair(land); // 4 nodes with land
    map.textures[MapPoint(3,3)] = TexturePair(land);
    
    for (auto island : otherIslands)
    {
        auto neighbors = map.textures.GetNeighbours(island);
        for (auto pt : neighbors)
        {
            map.textures[pt] = TexturePair(land);
        }
        map.textures[island] = TexturePair(land);
    }

    auto islands = FindIslands(map, 5);
    
    BOOST_REQUIRE(islands.size() == otherIslands.size());
    
    for (auto actualIsland : islands)
    {
        BOOST_REQUIRE(!helpers::contains(actualIsland, tinyIsland));
    }
}

BOOST_AUTO_TEST_CASE(FindCoastland_CoastOutsideOfArea_ReturnsNoCoast)
{
    // setup texture mapping
    Texture expectedTexture(0x2);
    DescIdx<LandscapeDesc> landscape;
    MockTextureMapping mapping(landscape);
    
    mapping.expectedTextures.push_back(expectedTexture);

    // setup map
    MapExtent size(8,8);
    Map map(mapping, landscape, 0x1, size);
    map.textures.Resize(size, TexturePair(expectedTexture));

    // run actual test for emtpy area
    std::vector<MapPoint> area {};

    auto coast = FindCoastland(map, area);
    
    BOOST_REQUIRE(coast.empty());
}

BOOST_AUTO_TEST_CASE(FindCoastland_CoastSubsetOfArea_ReturnsCoast)
{
    // setup texture mapping
    Texture defaultTexture(0x1);
    Texture expectedTexture(0x2);
    DescIdx<LandscapeDesc> landscape;
    MockTextureMapping mapping(landscape);
    
    mapping.expectedTextures.push_back(expectedTexture);

    // setup map
    MapExtent size(8,8);
    Map map(mapping, landscape, 0x1, size);
    map.textures.Resize(size, TexturePair(defaultTexture));

    // run actual test for emtpy area
    std::vector<MapPoint> area {
        MapPoint(0,0), MapPoint(1,0), MapPoint(2,0),
        MapPoint(0,1), MapPoint(1,1), MapPoint(2,1),
        MapPoint(0,2), MapPoint(1,2), MapPoint(2,2)
    };

    // LSD triangle (0/0) is water & land
    // -> 3 neighbors are considered coast
    map.textures[area[0]].lsd = expectedTexture;

    auto coast = FindCoastland(map, area);
    
    BOOST_REQUIRE(coast.size() == 3);
}

BOOST_AUTO_TEST_CASE(FindCoastland_CoastInAndOutsideOfArea_ReturnsCoastInsideOfArea)
{
    // setup texture mapping
    Texture expectedTexture(0x2);
    DescIdx<LandscapeDesc> landscape;
    MockTextureMapping mapping(landscape);
    
    mapping.expectedTextures.push_back(expectedTexture);

    // setup map
    MapExtent size(8,8);
    Map map(mapping, landscape, 0x1, size);
    map.textures.Resize(size, TexturePair(expectedTexture));

    // run actual test for emtpy area
    std::vector<MapPoint> area {
        MapPoint(0,0), MapPoint(1,0), MapPoint(2,0),
        MapPoint(0,1), MapPoint(1,1), MapPoint(2,1),
        MapPoint(0,2), MapPoint(1,2), MapPoint(2,2)
    };
    
    auto coast = FindCoastland(map, area);
    
    BOOST_REQUIRE(coast.size() == area.size());
}

BOOST_AUTO_TEST_CASE(PlaceHarborPosition_FlattensGroundInNeighborhood)
{
    MapPoint position(3,4);
    
    // setup texture mapping
    DescIdx<LandscapeDesc> landscape;
    MockTextureMapping mapping(landscape);

    // setup map
    MapExtent size(8,8);
    Map map(mapping, landscape, 0x1, size);
    map.z.Resize(size, map.height.max);

    const unsigned minHeight = map.height.min;
    const auto neighbors = map.z.GetNeighbours(position);
    
    for (unsigned i = 0; i < neighbors.size(); i++)
    {
        map.z[neighbors[i]] = minHeight + i;
    }
    
    // run actual test
    PlaceHarborPosition(map, position);

    BOOST_REQUIRE(map.z[position] == minHeight);
    for (auto neighbor : neighbors)
    {
        BOOST_REQUIRE(map.z[neighbor] == minHeight);
    }
}

BOOST_AUTO_TEST_CASE(PlaceHarborPosition_AppliesBuildableTerrainAroundPosition)
{
    MapPoint position(3,4);
    
    // setup texture mapping
    Texture defaultTexture(0x1);
    Texture expectedTexture(0x2);
    Textures textures { defaultTexture, expectedTexture };
    DescIdx<LandscapeDesc> landscape;
    MockTextureMapping mapping(landscape, textures);

    mapping.expectedTextures.push_back(expectedTexture);

    // setup map
    MapExtent size(8,8);
    Map map(mapping, landscape, 0x1, size);
    map.textures.Resize(size, TexturePair(defaultTexture));

    // run actual test
    PlaceHarborPosition(map, position);

    auto triangles = GetTriangles(position, size);
    for (auto triangle : triangles)
    {
        if (triangle.rsu)
        {
            BOOST_REQUIRE(map.textures[triangle.position].rsu == expectedTexture);
        }
        else
        {
            BOOST_REQUIRE(map.textures[triangle.position].lsd == expectedTexture);
        }
    }
}

// OLD WORLD

BOOST_AUTO_TEST_CASE(PlacesHarbors_ForAMapWithCoast_AddsAtLeastOneHarborPosition)
{
    MockTextureMapping_ mapping;
    
    Texture water = mapping.water;
    Texture coast = mapping.GetCoastTerrain();
    
    WorldDescription worldDesc;
    MapExtent size(8,8);
    Map_ map(worldDesc, size);
    Textures textures {
                                    water, water, water, water, water, water, water, water,
                             water, water, water, water, water, water, water, water,
                             water, water, water, water, water, water, water, water,
                      coast, coast, coast, coast, coast, coast, coast, coast,
                      coast, coast, coast, coast, coast, coast, coast, coast,
               coast, coast, coast, coast, coast, coast, coast, coast,
               coast, coast, coast, coast, coast, coast, coast, coast,
        coast, coast, coast, coast, coast, coast, coast, coast
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
        5,5,5,5,5,5,5,5 };
    
    PlaceHarbors(10, 5, 1, map, mapping);
    
    BOOST_REQUIRE(!map.harborsLsd.empty() || !map.harborsRsu.empty());
}

BOOST_AUTO_TEST_CASE(PlaceHarbors_ForMapWithCoast_AddsHarborPositionsOnlyOnBuildableTerrain)
{
    MockTextureMapping_ mapping;
    
    Texture water = mapping.water;
    Texture coast = mapping.GetCoastTerrain();
    Texture coast2 = mapping.GetCoastTerrain(1);
    Texture coast3 = mapping.GetCoastTerrain(2);

    WorldDescription worldDesc;
    MapExtent size(8,8);
    Map_ map(worldDesc, size);
    Textures textures {
                                    water, water, water, water, water, water, water, water,
                             water, water, water, water, water, water, water, water,
                             water, water, water, water, water, water, water, water,
                      coast, coast, coast, coast, coast, coast, coast, coast,
                      coast, coast, coast, coast, coast, coast, coast, coast,
               coast, coast, coast, coast, coast, coast, coast, coast,
               coast, coast, coast, coast, coast, coast, coast, coast,
        coast, coast, coast, coast, coast, coast, coast, coast
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
        5,5,5,5,5,5,5,5 };
    
    PlaceHarbors(10, 5, 1, map, mapping);
    
    for (auto index : map.harborsLsd)
    {
        BOOST_REQUIRE(map.textureLsd[index] == coast2 ||
                      map.textureLsd[index] == coast3);
    }

    for (auto index : map.harborsRsu)
    {
        BOOST_REQUIRE(map.textureRsu[index] == coast2 ||
                      map.textureRsu[index] == coast3);
    }
}

BOOST_AUTO_TEST_CASE(PlaceHarbors_ForMapWithCoast_AddsHarborPositionsNearWater)
{
    MockTextureMapping_ mapping;
    
    Texture water = mapping.water;
    Texture coast = mapping.GetCoastTerrain();

    WorldDescription worldDesc;
    MapExtent size(8,8);
    Map_ map(worldDesc, size);
    Textures textures {
                                    water, water, water, water, water, water, water, water,
                             water, water, water, water, water, water, water, water,
                             water, water, water, water, water, water, water, water,
                      coast, coast, coast, coast, coast, coast, coast, coast,
                      coast, coast, coast, coast, coast, coast, coast, coast,
               coast, coast, coast, coast, coast, coast, coast, coast,
               coast, coast, coast, coast, coast, coast, coast, coast,
        coast, coast, coast, coast, coast, coast, coast, coast
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
        5,5,5,5,5,5,5,5 };
    
    PlaceHarbors(10, 5, 1, map, mapping);

    for (auto index : map.harborsLsd)
    {
        auto harbor = GridPosition(index, size);
        auto neighbors = GridCollect(harbor, size, 3.0);
        auto waterNeighbor = false;

        for (auto neighbor : neighbors)
        {
            int neighborIndex = neighbor.x + neighbor.y * size.x;
            if (map.textureRsu[neighborIndex] == water ||
                map.textureLsd[neighborIndex] == water)
            {
                waterNeighbor = true;
                break;
            }
        }
        
        BOOST_REQUIRE(waterNeighbor);
    }

    for (auto index : map.harborsRsu)
    {
        auto harbor = GridPosition(index, size);
        auto neighbors = GridCollect(harbor, size, 3.0);
        auto waterNeighbor = false;

        for (auto neighbor : neighbors)
        {
            int neighborIndex = neighbor.x + neighbor.y * size.x;
            if (map.textureRsu[neighborIndex] == water ||
                map.textureLsd[neighborIndex] == water)
            {
                waterNeighbor = true;
                break;
            }
        }
        
        BOOST_REQUIRE(waterNeighbor);
    }
}

BOOST_AUTO_TEST_CASE(PlaceHarborPosition_ForMapAtPosition_FlattensAreaAroundHarborPosition)
{
    MockTextureMapping_ mapping;
    
    Texture water = mapping.water;
    Texture coast = mapping.GetCoastTerrain();

    WorldDescription worldDesc;
    MapExtent size(8,8);
    Map_ map(worldDesc, size);
    Textures textures {
                                    water, water, water, water, water, water, water, water,
                             water, water, water, water, water, water, water, water,
                             water, water, water, water, water, water, water, water,
                      coast, coast, coast, coast, coast, coast, coast, coast,
                      coast, coast, coast, coast, coast, coast, coast, coast,
               coast, coast, coast, coast, coast, coast, coast, coast,
               coast, coast, coast, coast, coast, coast, coast, coast,
        coast, coast, coast, coast, coast, coast, coast, coast
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
        5,5,5,5,5,5,5,5 };

    Position harborPosition(3,3);
    
    PlaceHarborPosition(map, mapping, CoastNode(harborPosition, Position(3,2)));
    
    auto area = GridCollect(harborPosition, size, 2.0);
    
    for (auto point : area)
    {
        auto index = point.x + point.y * size.x;
        auto height = map.z[index];
        
        BOOST_REQUIRE(height <= 2u);
    }
}

BOOST_AUTO_TEST_SUITE_END()
