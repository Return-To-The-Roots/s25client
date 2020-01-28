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
#include "randomMaps/waters/RiverBrush.h"

#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_SUITE(RiverBrushTests)

BOOST_AUTO_TEST_CASE(RiverBanksAreTexturedWithMountainOrCoast)
{
    MapExtent size(8,8);
    Map map(size, "test", "test");
    
    std::vector<TextureType> textures = {
        Grass1, Grass1, Grass1, Grass1, Grass1, Grass1, Grass1, Grass1,
        Grass1, Grass1, Grass1, Grass1, Grass1, Grass1, Grass1, Grass1,
        Grass1, Grass1, Grass1, Grass1, Grass1, Grass1, Grass1, Grass1,
        Grass1, Grass1, Grass1, Grass1, Grass1, Grass1, Grass1, Grass1,
        Grass1, Grass1, Grass1, Grass1, Grass1, Grass1, Grass1, Grass1,
        Grass1, Grass1, Grass1, Grass1, Mountain1, Mountain1, Mountain1, Mountain1,
        Grass1, Grass1, Grass1, Grass1, Grass1, Grass1, Grass1, Grass1,
        Grass1, Grass1, Grass1, Grass1, Grass1, Grass1, Grass1, Grass1
    };
    
    map.textureLsd = std::vector<TextureType>(textures);
    map.textureRsu = std::vector<TextureType>(textures);
    map.objectInfo.resize(64u, 0x0);
    map.objectType.resize(64u, 0x0);
    map.z.resize(64, 0x0);
    
    RiverBrush brush;
    
    std::vector<Tile> river = {
        Tile(Position(4,4)),
        Tile(Position(5,4)),
        Tile(Position(6,4)),
        Tile(Position(7,4))
    };
    
    brush.Paint(river, &map, 0u);
    
    for (int x = 4; x < 8; x++)
    {
        BOOST_REQUIRE(map.textureRsu[x + 5 * 8] == Mountain1);
        BOOST_REQUIRE(map.textureLsd[x + 5 * 8] == Mountain1);
        BOOST_REQUIRE(map.textureRsu[x + 3 * 8] == Coast);
        BOOST_REQUIRE(map.textureLsd[x + 3 * 8] == Coast);
    }
}

BOOST_AUTO_TEST_SUITE_END()
