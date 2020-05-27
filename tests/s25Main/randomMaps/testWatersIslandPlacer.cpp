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

#include "rttrDefines.h" // IWYU pragma: keep
#include "randomMaps/waters/IslandPlacer.h"

#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_SUITE(IslandPlacerTests)

Map GenerateTestMap()
{
    MapExtent size(8,8);
    Map map(size, "test", "test");
    
    std::vector<TextureType> textures = {
        Water, Water, Water, Water, Water, Water, Water, Water,
        Water, Water, Water, Water, Water, Water, Water, Water,
        Water, Water, Water, Water, Water, Water, Water, Water,
        Water, Water, Water, Water, Water, Water, Water, Water,
        Water, Water, Water, Water, /**/Water, Water, Water, Water,
        Water, Water, Water, Water, Water, Water, Water, Water,
        Water, Water, Water, Water, Water, Water, Water, Water,
        Water, Water, Water, Water, Water, Water, Water, Water
    };
    
    map.textureLsd = std::vector<TextureType>(textures);
    map.textureRsu = std::vector<TextureType>(textures);
    map.objectInfo.resize(64u, 0x0);
    map.objectType.resize(64u, 0x0);
    map.z.resize(64, 0x0);
    
    return map;
}

std::vector<Position> CreateIsland()
{
    return {
        Position(4,4), Position(5,4), Position(6,4),
        Position(3,5), Position(4,5), Position(5,5),
        Position(3,6), Position(4,6), Position(5,6)
    };
}

BOOST_AUTO_TEST_CASE(CoastlineHasCoastTextures)
{
    Map map = GenerateTestMap();
    HeightSettings settings(0u, 10u);
    IslandPlacer placer(settings);
    
    std::set<Tile, TileCompare> coverage;
    auto island = CreateIsland();
    
    for (auto tile : island)
    {
        coverage.insert(Tile(tile));
    }
    
    placer.Place(coverage, &map, 0u);
    
    for (auto tile : island)
    {
        int i = tile.x + tile.y * map.size.x;
        auto neighbors = GridUtility::Neighbors(tile, map.size);
        for (auto neighbor : neighbors)
        {
            int index = neighbor.x + neighbor.y * map.size.x;
            if (map.textureRsu[index] == Water || map.textureLsd[index] == Water)
            {
                BOOST_REQUIRE(map.textureRsu[i] == Coast);
                BOOST_REQUIRE(map.textureLsd[i] == Coast);
            }
        }
    }
}

BOOST_AUTO_TEST_CASE(IslandCenterIsHigherThanCoastline)
{
    Map map = GenerateTestMap();
    HeightSettings settings(0u, 10u);
    IslandPlacer placer(settings);
    
    std::set<Tile, TileCompare> coverage;
    auto island = CreateIsland();
    
    for (auto tile : island)
    {
        coverage.insert(Tile(tile));
    }
    
    placer.Place(coverage, &map, 0u);
    
    BOOST_REQUIRE(map.z[4 + 5 * 8] > map.z[4 + 4 * 8]);
    BOOST_REQUIRE(map.z[4 + 5 * 8] > map.z[5 + 4 * 8]);
    BOOST_REQUIRE(map.z[4 + 5 * 8] > map.z[6 + 4 * 8]);
    BOOST_REQUIRE(map.z[4 + 5 * 8] > map.z[3 + 5 * 8]);
    BOOST_REQUIRE(map.z[4 + 5 * 8] > map.z[5 + 5 * 8]);
    BOOST_REQUIRE(map.z[4 + 5 * 8] > map.z[3 + 6 * 8]);
    BOOST_REQUIRE(map.z[4 + 5 * 8] > map.z[4 + 6 * 8]);
    BOOST_REQUIRE(map.z[4 + 5 * 8] > map.z[5 + 6 * 8]);
}

BOOST_AUTO_TEST_SUITE_END()
