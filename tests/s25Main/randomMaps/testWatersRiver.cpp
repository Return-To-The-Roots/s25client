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
#include "randomMaps/waters/River.h"

#include <boost/test/unit_test.hpp>

#include <iostream>

BOOST_AUTO_TEST_SUITE(RiverTests)

BOOST_AUTO_TEST_CASE(RiversAreAConnectedAreaCoveredByWater)
{
    MapExtent size(8,8);
    Map map(size, "test", "test");
    
    std::vector<TextureType> textures = {
        Grass1, Grass1, Grass1, Grass1, Grass1, Grass1, Grass1, Grass1,
        Grass1, Grass1, Grass1, Grass1, Grass1, Grass1, Grass1, Grass1,
        Grass1, Grass1, Grass1, Grass1, Grass1, Grass1, Grass1, Grass1,
        Grass1, Grass1, Grass1, Grass1, Grass1, Grass1, Grass1, Grass1,
        Grass1, Grass1, Grass1, Grass1, Grass1, Grass1, Grass1, Grass1,
        Grass1, Grass1, Grass1, Grass1, Grass1, Grass1, Grass1, Grass1,
        Grass1, Grass1, Grass1, Grass1, Grass1, Grass1, Grass1, Grass1,
        Grass1, Grass1, Grass1, Grass1, Grass1, Grass1, Grass1, Grass1
    };
    
    map.textureLsd = std::vector<TextureType>(textures);
    map.textureRsu = std::vector<TextureType>(textures);
    map.objectInfo.resize(64u, 0x0);
    map.objectType.resize(64u, 0x0);
    map.z.resize(64, 0x0);
    
    Tile source(Position(4,4));
    RandomUtility rnd(0);
    RiverBrush brush;
    RiverMotion motion(rnd);

    const int riverTiles = 5;
    
    River(motion, brush, 0, source).ExtendBy(riverTiles, size).Create(&map, 0u);
     
    auto waterTile = 0;
    auto water = true;
    auto current = source;
    
    if (map.textureRsu[current.IndexRsu(size)] == Water &&
        map.textureLsd[current.IndexLsd(size)] == Water)
    {
        waterTile++;
    }
    
    while (water && waterTile < riverTiles)
    {
        auto neighbors = current.Neighbors(size);
        auto foundWater = false;
        
        for (auto neighbor : neighbors)
        {
            if (map.textureRsu[neighbor.IndexRsu(size)] == Water &&
                map.textureLsd[neighbor.IndexLsd(size)] == Water)
            {
                waterTile++;
                current = neighbor;
                foundWater = true;
                break;
            }
        }
        
        water = foundWater;
    }
    
    BOOST_REQUIRE(waterTile == riverTiles);
}

BOOST_AUTO_TEST_SUITE_END()
