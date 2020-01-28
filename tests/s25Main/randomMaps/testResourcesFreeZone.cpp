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
#include "randomMaps/resources/FreeZone.h"
#include "randomMaps/algorithm/GridUtility.h"

#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_SUITE(FreeZoneTests)

BOOST_AUTO_TEST_CASE(FreeZoneReturnsDistanceToHarborAndHeadQuarterPositions)
{
    std::vector<TextureType> originalTextures = {
        Water,  Water,  Water,  Water,  Water,  Water,  Water,  Water,
        Water,  Water,  Water,  Water,  Water,  Water,  Water,  Water,
        Water,  Water,  Water,  Water,  Water,  Water,  Water,  Water,
        Coast,  Grass1, Grass1, Grass1, Grass1, Grass1, Grass1, Coast,
        Coast,  Grass1, Grass1, Grass1, Grass1, Grass1, Grass1, Coast,
        Coast,  Grass1, Grass1, Grass1, Grass1, Grass1, Grass1, Coast,
        Coast,  Coast,  Coast,  Coast,  Coast,  Coast,  Coast,  Coast,
        Lava,   Lava,   Lava,   Lava,   Lava,   Lava,   Lava,   Lava
    };
    
    std::vector<unsigned char> originalZ = {
        0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,
        5,5,5,5,5,5,5,5,
        5,5,5,5,5,5,5,5,
        5,5,5,5,5,5,5,5,
        5,5,5,5,5,5,5,5,
        5,5,5,5,5,5,5,5
    };
    
    MapExtent size(8,8);
    Map map(size, "test", "test");
    
    map.textureLsd = std::vector<TextureType>(originalTextures);
    map.textureRsu = std::vector<TextureType>(originalTextures);
    map.objectInfo.resize(64u, 0x0);
    map.objectType.resize(64u, 0x0);
    map.z = std::vector<unsigned char>(originalZ);
    
    auto hqIndex = 25;
    auto hq = GridUtility::GetPosition(hqIndex, size);
    
    map.hqPositions[0] = MapPoint(hq.x, hq.y);
    
    auto harborIndex = 30;
    
    map.harborsLsd.insert(harborIndex);
    map.harborsRsu.insert(harborIndex);

    auto result = FreeZone().ComputeDistance(map);
    
    BOOST_REQUIRE(result[hqIndex] == 0);
    BOOST_REQUIRE(result[harborIndex] == 0);

    BOOST_REQUIRE(result[hqIndex + 1] == 1);
    BOOST_REQUIRE(result[harborIndex - 1] == 1);
}

BOOST_AUTO_TEST_SUITE_END()
