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
#include "randomMaps/objects/HarborGenerator.h"
#include "randomMaps/algorithm/GridUtility.h"

#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_SUITE(HarborGeneratorTests)

BOOST_AUTO_TEST_CASE(FlattensAreaAroundNeighborPosition)
{
    MapExtent size(8,8);
    Map map(size, "test", "test");
    
    map.textureLsd = {
        Water, Water, Water, Water, Water, Water, Water, Water,
        Water, Water, Water, Water, Water, Water, Water, Water,
        Water, Water, Water, Water, Water, Water, Water, Water,
        Coast, Coast, Coast, Coast, Coast, Coast, Coast, Coast,
        Coast, Coast, Coast, Coast, Coast, Coast, Coast, Coast,
        Coast, Coast, Coast, Coast, Coast, Coast, Coast, Coast,
        Coast, Coast, Coast, Coast, Coast, Coast, Coast, Coast,
        Coast, Coast, Coast, Coast, Coast, Coast, Coast, Coast
    };
    map.textureRsu = {
        Water, Water, Water, Water, Water, Water, Water, Water,
        Water, Water, Water, Water, Water, Water, Water, Water,
        Water, Water, Water, Water, Water, Water, Water, Water,
        Coast, Coast, Coast, Coast, Coast, Coast, Coast, Coast,
        Coast, Coast, Coast, Coast, Coast, Coast, Coast, Coast,
        Coast, Coast, Coast, Coast, Coast, Coast, Coast, Coast,
        Coast, Coast, Coast, Coast, Coast, Coast, Coast, Coast,
        Coast, Coast, Coast, Coast, Coast, Coast, Coast, Coast
    };

    map.objectInfo.resize(64u, 0x0);
    map.objectType.resize(64u, 0x0);
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

    Position harborPosition(3,3);
    CoastTile coastTile(harborPosition, Position(3,2));
    HarborGenerator harbor;
    
    harbor.Build(map, coastTile);
    
    auto area = GridUtility::Collect(harborPosition, size, 3.0);
    
    for (auto point : area)
    {
        int index = point.x + point.y * size.x;
        BOOST_REQUIRE(map.z[index] >= 0 && map.z[index] <= 2);
    }
}

BOOST_AUTO_TEST_SUITE_END()
