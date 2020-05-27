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
#include "randomMaps/terrain/TextureTranslator.h"

#include <iostream>
#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_SUITE(TextureTranslatorTests)

BOOST_AUTO_TEST_CASE(GetTextureReturnsValidValues)
{
    unsigned char sea = 0;
    unsigned char mountain = 10;
    
    HeightSettings height(0, 23);
    TextureTranslator translator(height);
    
    for (unsigned char h = height.minimum; h <= height.maximum; ++h)
    {
        auto value = translator.GetTexture(h, sea, mountain);
        BOOST_REQUIRE_GE(value, Water);
        BOOST_REQUIRE_LE(value, Lava);

    }
}

BOOST_AUTO_TEST_CASE(TexturesBelowOrEqualToSeaLevel)
{
    unsigned char sea = 5;
    unsigned char mountain = 15;
    
    HeightSettings height(0, 23);
    TextureTranslator translator(height);
    
    for (unsigned char h = height.minimum; h < sea; ++h)
    {
        auto value = translator.GetTexture(h, sea, mountain);
        BOOST_REQUIRE_EQUAL(Water, value);
    }
}

BOOST_AUTO_TEST_CASE(TexturesAboveSeaAndBelowMountains)
{
    unsigned char sea = 5;
    unsigned char mountain = 15;
    
    HeightSettings height(0, 23);
    TextureTranslator translator(height);
    
    for (unsigned char h = sea + 1; h < mountain; ++h)
    {
        auto value = translator.GetTexture(h, sea, mountain);
        BOOST_REQUIRE_GE(value, Coast);
        BOOST_REQUIRE_LE(value, GrassToMountain);
    }
}

BOOST_AUTO_TEST_CASE(TexturesAtMountainLevel)
{
    unsigned char sea = 5;
    unsigned char mountain = 15;
    
    HeightSettings height(0, 23);
    TextureTranslator translator(height);
    
    auto value = translator.GetTexture(mountain, sea, mountain);
    BOOST_REQUIRE_EQUAL(value, Mountain1);
}

BOOST_AUTO_TEST_CASE(TexturesAboveMountainLevel)
{
    unsigned char sea = 5;
    unsigned char mountain = 15;
    
    HeightSettings height(0, 23);
    TextureTranslator translator(height);
    
    for (unsigned char h = mountain + 1; h < height.maximum; ++h)
    {
        auto value = translator.GetTexture(h, sea, mountain);
        BOOST_REQUIRE_GE(value, Mountain1);
        BOOST_REQUIRE_LE(value, Lava);

    }
}

BOOST_AUTO_TEST_SUITE_END()
