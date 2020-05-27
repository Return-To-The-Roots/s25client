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
#include "randomMaps/terrain/TextureMap.h"

#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_SUITE(TextureMapTests)

BOOST_AUTO_TEST_CASE(TextureMapCreatesATextureValueForEachTile)
{
    unsigned char seaLevel = 5;
    unsigned char mountainLevel = 15;
    
    std::vector<unsigned char> heightMap { 0, 1, 2, 23 };
    std::vector<bool> waterMap { true, false, false, false };

    MapExtent size(2, 2);
    HeightSettings settings(0, 23);
    TextureTranslator translator(settings);
    TextureMap textureMap(translator);
    
    auto result = textureMap.Create(heightMap, waterMap, size, seaLevel, mountainLevel);
    
    BOOST_REQUIRE_EQUAL(result.size(), size.x * size.y);
}

BOOST_AUTO_TEST_SUITE_END()
