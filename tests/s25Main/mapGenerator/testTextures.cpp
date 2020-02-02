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
#include "mapGenerator/Textures.h"
#include "testHelper.hpp"
#include <boost/test/unit_test.hpp>

using namespace rttr::mapGenerator;

BOOST_AUTO_TEST_SUITE(TexturesTests)

// NEW WORLD

BOOST_AUTO_TEST_CASE(IncreaseMountains_ForMap_IncreasesHeightForMountainsOnly)
{
    Texture mountain(0x1);
    Texture other(0x2);

    DescIdx<LandscapeDesc> landscape;
    MockTextureMapping mapping(landscape);
    MapExtent size(8, 8);
    Map map(mapping, landscape, 0x1, size);
    MapPoint mountainPoint(3,4);
    
    map.textures.Resize(size, TexturePair(other));
    map.textures[mountainPoint].rsu = mountain;
    
    mapping.expectedTextures = { mountain };

    IncreaseMountains(map);

    BOOST_REQUIRE(map.z[mountainPoint] == 1);

    RTTR_FOREACH_PT(MapPoint, size)
    {
        if (pt != mountainPoint)
        {
            BOOST_REQUIRE(map.z[pt] == 0);
        }
    }
}

// OLD WORLD

BOOST_AUTO_TEST_CASE(FindCoast_ForIslandAndWaterMapReturnsCoastOfExpectedSize)
{
    MapExtent size(8,8);
    std::vector<Position> island = {
        Position(3,3), Position(4,3), Position(5,3),
        Position(3,4), Position(4,4), Position(5,4),
        Position(3,5), Position(4,5), Position(5,5)
    };
    
    std::vector<bool> water = {
        true, true, true, true, true, true, true, true,
        true, true, true, true, true, true, true, true,
        true, true, true, true, true, true, true, true,
        true, true, true,false,false,false, true, true,
        true, true, true,false,false,false, true, true,
        true, true, true,false,false,false, true, true,
        true, true, true, true, true, true, true, true,
        true, true, true, true, true, true, true, true
    };
    
    
    auto coastLine = FindCoast(island, water, size);
    
    BOOST_REQUIRE(coastLine.size() == 8);
}

BOOST_AUTO_TEST_CASE(FindCoast_ForRiverBank_ReturnsNoElements)
{
    MapExtent size(8,8);
    std::vector<Position> island = {
        Position(3,0), Position(4,0), Position(5,0), Position(6,0), Position(7,0),
        Position(3,1), Position(4,1), Position(5,1), Position(6,1), Position(7,1),
        Position(3,2), Position(4,2), Position(5,2), Position(6,2), Position(7,2),
        Position(3,3), Position(4,3), Position(5,3), Position(6,3), Position(7,3),
        Position(3,4), Position(4,4), Position(5,4), Position(6,4), Position(7,4),
        Position(3,5), Position(4,5), Position(5,5), Position(6,5), Position(7,5),
        // ================= r i v e r ==========================================
        Position(3,7), Position(4,7), Position(5,6), Position(6,7), Position(7,7)
    };
    
    std::vector<bool> water = {
        false,false,false,false,false,false,false,false,
        false,false,false,false,false,false,false,false,
        false,false,false,false,false,false,false,false,
        false,false,false,false,false,false,false,false,
        false,false,false,false,false,false,false,false,
        false,false,false,false,false,false,false,false,
        true, true, true, true, true, true, true, true,
        false,false,false,false,false,false,false,false
    };
    
    auto coastLine = FindCoast(island, water, size);
    
    BOOST_REQUIRE(coastLine.empty());
}

BOOST_AUTO_TEST_CASE(SmoothTextures_ForTextureMap_FlipsTrianglesToAvoidSharpCorners)
{
    MockTextureMapping_ mapping;
    
    Texture water = mapping.water;
    Texture lava = mapping.lava;
    Texture grass = mapping.grassland;
    Texture coast = mapping.coast;
    
    Textures rsu {
        water, lava /**/,water, water, water, water, water, water,
        water, water,    water, water, water, water, water, water,
        water, water,    water, water, water, water, water, water,
        coast, grass,    grass, grass, grass, grass, grass, coast,
        coast, grass,    grass, grass, grass, grass, grass, coast,
        coast, grass,    grass, grass, grass, grass, grass, coast,
        coast, coast,    coast, coast, coast, coast, coast, coast,
        lava,   lava,    lava,   lava,  lava,  lava,  lava,  lava
    };
    
    /**/
    //should be replaced by water

    Textures lsd {
        water, water,    water, water, water, water, water, water,
        water, water,    water, water, water, water, water, water,
        water, water,    water, water, water, water, water, water,
        coast, grass,    grass, grass, grass, grass, grass, coast,
        coast, grass,    grass, grass, grass, grass, grass, coast,
        coast, grass,    grass, grass, grass, grass, grass, coast,
        coast, coast,    coast, coast, coast, coast, coast, coast,
        lava,   lava,    lava,   lava,  lava,  lava,  lava,  lava
    };

    WorldDescription desc;
    MapExtent size(8,8);
    Map_ map(desc, size);
    
    map.textureRsu = rsu;
    map.textureLsd = lsd;
    
    SmoothTextures(map, mapping);
    
    BOOST_REQUIRE(map.textureRsu[1]  == water);
    BOOST_REQUIRE(map.textureLsd[34] == grass);
}

BOOST_AUTO_TEST_SUITE_END()
