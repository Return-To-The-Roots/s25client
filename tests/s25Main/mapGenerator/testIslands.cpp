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

#include "mapGenerator/Islands.h"
#include "mapGenerator/GridUtility.h"
#include "testHelper.hpp"

#include <boost/test/unit_test.hpp>

using namespace rttr::mapGenerator;

BOOST_AUTO_TEST_SUITE(IslandsTests)
/*
BOOST_AUTO_TEST_CASE(FindNextPosition_ForExcludedPositions_SkipsExlcudedPositions)
{
    MapExtent size(45, 56);
    std::vector<bool> excludedPositions(size.x * size.y, true);
    Position expectedResult(23, 43); // only non-excluded position
    excludedPositions[expectedResult.x + expectedResult.y * size.x] = false;
    
    auto result = FindNextPosition(excludedPositions, size);
    
    BOOST_REQUIRE(result.x == expectedResult.x && result.y == expectedResult.y);
}

BOOST_AUTO_TEST_CASE(FindIslands_ForAWaterMapWith3Islands_Returns3Islands)
{
    const int width = 16;
    const int height = 32;
    const int minimumTilesPerIsland = 2;
    
    MapExtent size(width, height);
    WaterMap waterMap(width * height, true);
    
    std::vector<Position> landTiles = {
        // 1st island
        Position(2,2), Position(3,2),

        // island (too small)
        Position(4,4),

        // 2nd island
        Position(6,6), Position(6,7), Position(7,6),
        
        // 3rd island
        Position(12,3), Position(12,4)
    };
    
    for (auto landTile : landTiles)
    {
        waterMap[landTile.x + landTile.y * width] = false;
    }
    
    Islands islands = FindIslands(waterMap, size, minimumTilesPerIsland);
    
    BOOST_REQUIRE(islands.size() == 3);
}

BOOST_AUTO_TEST_CASE(CreateWaterMap_ForHeightMap_ReturnsWaterMapEqualNumberOfEntries)
{
    HeightMap z { 13, 222, 17, 18, 45, 12, 123, 5 };

    WaterMap waterMap = CreateWaterMap(z, 17);
    
    BOOST_REQUIRE(z.size() == waterMap.size());
}

BOOST_AUTO_TEST_CASE(CreateWaterMap_ForSeaLevel_ReturnsWaterForHeightValuesBelowOrEqualToSeaLevel)
{
    HeightMap z { 13, 222, 17, 18, 45, 12, 123, 5 };
    Height seaLevel = 17;
    
    WaterMap waterMap = CreateWaterMap(z, seaLevel);
    
    for (int i = 0; i < static_cast<int>(z.size()); ++i)
    {
        if (z[i] <= seaLevel)
        {
            BOOST_REQUIRE(waterMap[i]);
        }
    }
}

BOOST_AUTO_TEST_CASE(CreateWaterMap_ForSeaLevel_ReturnsNoWaterForHeightValuesAboveSeaLevel)
{
    HeightMap z { 13, 222, 17, 18, 45, 12, 123, 5 };
    Height seaLevel = 17;
    
    WaterMap waterMap = CreateWaterMap(z, seaLevel);
    
    for (int i = 0; i < static_cast<int>(z.size()); ++i)
    {
        if (z[i] > seaLevel)
        {
            BOOST_REQUIRE(!waterMap[i]);
        }
    }
}

BOOST_AUTO_TEST_CASE(CreateWaterMap_ForSeaLevel_SetsHeightForWaterToSeaLevel)
{
    HeightMap z { 13, 222, 17, 18, 45, 12, 123, 5 };
    Height seaLevel = 17;
    
    WaterMap waterMap = CreateWaterMap(z, seaLevel);
    
    for (int i = 0; i < static_cast<int>(z.size()); ++i)
    {
        if (waterMap[i])
        {
            BOOST_REQUIRE(z[i] == seaLevel);
        }
    }
}

BOOST_AUTO_TEST_CASE(FindSuitableIslandLocation_ForMap_ReturnsPositionWithMaximumDistance)
{
    WorldDescription worldDesc;
    MapExtent size(8,8);
    Map map(worldDesc, size);
    
    std::vector<int> landDistance {
        0, 0, 0, 0, 0, 0, 0, 0,
        0, 1, 1, 1, 1, 1, 1, 0,
        0, 0, 1, 2, 2, 2, 1, 0,
        0, 0, 1, 2, 3, 2, 1, 0,
        0, 0, 1, 2, 2, 2, 1, 0,
        0, 0, 1, 2, 1, 2, 1, 0,
        0, 1, 1, 1, 0, 1, 1, 0,
        0, 0, 0, 0, 0, 0, 0, 0
    };
    
    auto position = FindSuitableIslandLocation(map, landDistance);
    
    BOOST_REQUIRE(position.x == 4 && position.y == 3);
}

BOOST_AUTO_TEST_CASE(PlaceIsland_ForCoverageAndMap_AddsCoastTexturesAtTheCoastline)
{
    MockTextureMapping mapping;
    
    Texture water = mapping.water;
    Texture coast = mapping.coast;
    
    WorldDescription worldDesc;
    Map map(worldDesc, MapExtent(8,8), 0, 10);

    map.sea = 0;
    map.textureLsd = Textures(64u, water);
    map.textureRsu = Textures(64u, water);
    
    std::set<Tile, TileCompare> coverage;
    auto island = {
        Position(4,4), Position(5,4), Position(6,4),
        Position(3,5), Position(4,5), Position(5,5),
        Position(3,6), Position(4,6), Position(5,6)
    };
    
    for (auto tile : island)
    {
        coverage.insert(Tile(tile));
    }
    
    PlaceIsland(coverage, map, mapping);
    
    for (auto tile : island)
    {
        int i = tile.x + tile.y * map.size.x;
        auto neighbors = GridNeighbors(tile, map.size);
        for (auto neighbor : neighbors)
        {
            int index = neighbor.x + neighbor.y * map.size.x;
            if (map.textureRsu[index] == water || map.textureLsd[index] == water)
            {
                BOOST_REQUIRE(map.textureRsu[i] == coast);
                BOOST_REQUIRE(map.textureLsd[i] == coast);
            }
        }
    }
}

BOOST_AUTO_TEST_CASE(PlaceIslands_ForCoverageAndMap_IncreasesHeightWithCoastDistance)
{
    MockTextureMapping mapping;
    
    Texture water = mapping.water;
    
    WorldDescription worldDesc;
    Map map(worldDesc, MapExtent(8,8), 0, 10);
    
    map.sea = 0;
    map.textureLsd = Textures(64u, water);
    map.textureRsu = Textures(64u, water);
    
    std::set<Tile, TileCompare> coverage;
    auto island = {
        Position(4,4), Position(5,4), Position(6,4),
        Position(3,5), Position(4,5), Position(5,5),
        Position(3,6), Position(4,6), Position(5,6)
    };
    
    for (auto tile : island)
    {
        coverage.insert(Tile(tile));
    }
    
    PlaceIsland(coverage, map, mapping);
    
    BOOST_REQUIRE(map.z[4 + 5 * 8] > map.z[4 + 4 * 8]);
    BOOST_REQUIRE(map.z[4 + 5 * 8] > map.z[5 + 4 * 8]);
    BOOST_REQUIRE(map.z[4 + 5 * 8] > map.z[6 + 4 * 8]);
    BOOST_REQUIRE(map.z[4 + 5 * 8] > map.z[3 + 5 * 8]);
    BOOST_REQUIRE(map.z[4 + 5 * 8] > map.z[5 + 5 * 8]);
    BOOST_REQUIRE(map.z[4 + 5 * 8] > map.z[3 + 6 * 8]);
    BOOST_REQUIRE(map.z[4 + 5 * 8] > map.z[4 + 6 * 8]);
    BOOST_REQUIRE(map.z[4 + 5 * 8] > map.z[5 + 6 * 8]);
}

BOOST_AUTO_TEST_CASE(CreateIsland_ForMapAndIslandSize_CreatesLandAreaOfSpecifiedSize)
{
    MockTextureMapping mapping;
    
    Texture water = mapping.water;
    
    WorldDescription worldDesc;
    Map map(worldDesc, MapExtent(8,8), 0, 10);
    
    map.textureLsd = Textures(64, water);
    map.textureRsu = Textures(64, water);
    map.sea = 0;
    
    const int islandSizeInNodes = 7;
    
    RandomUtility rnd(0);

    CreateIsland(rnd, islandSizeInNodes, 3, 1, map, mapping);
    
    int landTriangles = 0;
    
    for (int i = 0; i < 64; i++)
    {
        if (map.textureRsu[i] != water)
        {
            landTriangles++;
        }
        
        if (map.textureLsd[i] != water)
        {
            landTriangles++;
        }
    }
    
    BOOST_REQUIRE(landTriangles >= islandSizeInNodes * 2 &&
                  landTriangles <= islandSizeInNodes * 2 + 1);
}

BOOST_AUTO_TEST_CASE(CreateIsland_ForMinimumLandDistance_CreatesLandFurtherOrEqualFromOtherLand)
{
    MockTextureMapping mapping;
    
    Texture water = mapping.water;
    Texture coast = mapping.coast;
    
    WorldDescription worldDesc;
    MapExtent size(8,8);
    Map map(worldDesc, size, 0, 10);
    
    Textures textures {
        coast, coast, coast, coast, coast, coast, coast, coast,
        water, water, water, water, water, water, water, water,
        water, water, water, water, water, water, water, water,
        water, water, water, water, water, water, water, water,
        water, water, water, water, water, water, water, water,
        water, water, water, water, water, water, water, water,
        water, water, water, water, water, water, water, water,
        water, water, water, water, water, water, water, water
    };
    
    map.textureLsd = Textures(textures);
    map.textureRsu = Textures(textures);
    map.sea = 0;
    
    const int islandDistance = 2;
    
    RandomUtility rnd(0);
    
    CreateIsland(rnd, 4, 3, islandDistance, map, mapping);

    for (int i = 8; i < 64; i++) //skip initial land
    {
        // find island tiles (non-water)
        if (map.textureRsu[i] != water || map.textureLsd[i] != water)
        {
            // ensure that island tiles are not close to initial land
            auto p = GridPosition(i, size);
            auto distance = GridDistance(p, Position(p.x, 0), size);
            
            BOOST_REQUIRE(distance >= islandDistance);
        }
    }
}
*/

BOOST_AUTO_TEST_SUITE_END()
