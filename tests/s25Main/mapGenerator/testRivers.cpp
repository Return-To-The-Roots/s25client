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

#include "mapGenerator/Rivers.h"
#include "testHelper.hpp"

#include <boost/test/unit_test.hpp>

using namespace rttr::mapGenerator;

BOOST_AUTO_TEST_SUITE(RiversTests)

BOOST_AUTO_TEST_CASE(RiverBrushPaint_ForRiver_TexturesRiverBanksWithMountainOrCoast)
{
    MockTextureMapping_ mapping;
    
    Texture grassland = mapping.grassland;
    Texture mountain = mapping.mountain;
    Texture coast = mapping.coast;
    
    WorldDescription worldDesc;
    Map_ map(worldDesc, MapExtent(8,8));
    
    Textures textures {
        grassland, grassland, grassland, grassland, grassland, grassland, grassland, grassland,
        grassland, grassland, grassland, grassland, grassland, grassland, grassland, grassland,
        grassland, grassland, grassland, grassland, grassland, grassland, grassland, grassland,
        grassland, grassland, grassland, grassland, grassland, grassland, grassland, grassland,
        grassland, grassland, grassland, grassland, grassland, grassland, grassland, grassland,
        grassland, grassland, grassland, grassland, mountain,  mountain,  mountain,  mountain,
        grassland, grassland, grassland, grassland, grassland, grassland, grassland, grassland,
        grassland, grassland, grassland, grassland, grassland, grassland, grassland, grassland };

    map.textureLsd = Textures(textures);
    map.textureRsu = Textures(textures);
    map.sea = 0;
    
    std::vector<Tile> river = {
        Tile(Position(4,4)),
        Tile(Position(5,4)),
        Tile(Position(6,4)),
        Tile(Position(7,4)) };
    
    RiverBrush(mapping).Paint(river, map);
    
    for (int x = 4; x < 8; x++)
    {
        BOOST_REQUIRE(map.textureRsu[x + 5 * 8] == mountain);
        BOOST_REQUIRE(map.textureLsd[x + 5 * 8] == mountain);
        BOOST_REQUIRE(map.textureRsu[x + 3 * 8] == coast);
        BOOST_REQUIRE(map.textureLsd[x + 3 * 8] == coast);
    }
}

BOOST_AUTO_TEST_CASE(CreateRivers_ForMap_PlacesAConnectedAreaOfWater)
{
    MockTextureMapping_ mapping;
    
    Texture grass = mapping.grassland;
    Texture water = mapping.water;
    
    WorldDescription worldDesc;
    MapExtent size(8,8);
    Map_ map(worldDesc, size);
    
    Textures textures {
        grass, grass, grass, grass, grass, grass, grass, grass,
        grass, grass, grass, grass, grass, grass, grass, grass,
        grass, grass, grass, grass, grass, grass, grass, grass,
        grass, grass, grass, grass, grass, grass, grass, grass,
        grass, grass, grass, grass, grass, grass, grass, grass,
        grass, grass, grass, grass, grass, grass, grass, grass,
        grass, grass, grass, grass, grass, grass, grass, grass,
        grass, grass, grass, grass, grass, grass, grass, grass
    };
    
    map.textureLsd = Textures(textures);
    map.textureRsu = Textures(textures);
    map.sea = 0;
    
    Tile source(Position(4,4));
    RandomUtility rnd(0);
    RiverBrush brush(mapping);

    const int riverTiles = 5;
    
    River(brush, 0, source).ExtendBy(rnd, riverTiles, size).Create(map);
     
    auto waterTile = 0;
    auto isWater = true;
    auto current = source;
    
    if (map.textureRsu[current.IndexRsu(size)] == water &&
        map.textureLsd[current.IndexLsd(size)] == water)
    {
        waterTile++;
    }
    
    while (isWater && waterTile < riverTiles)
    {
        auto neighbors = current.Neighbors(size);
        auto foundWater = false;
        
        for (auto neighbor : neighbors)
        {
            if (map.textureRsu[neighbor.IndexRsu(size)] == water &&
                map.textureLsd[neighbor.IndexLsd(size)] == water)
            {
                waterTile++;
                current = neighbor;
                foundWater = true;
                break;
            }
        }
        
        isWater = foundWater;
    }
    
    BOOST_REQUIRE(waterTile == riverTiles);
}

BOOST_AUTO_TEST_SUITE_END()
