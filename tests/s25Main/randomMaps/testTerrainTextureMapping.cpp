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
#include "randomMaps/terrain/TextureMapping.h"

#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_SUITE(TextureMappingTests)

BOOST_AUTO_TEST_CASE(GreenlandMappingReturnsValidValues)
{
    GreenlandMapping mapping;
    unsigned char texture;
    
    texture = mapping.Get(Water);
    BOOST_REQUIRE(texture >= 0x0 && texture <= 0x22);
    texture = mapping.Get(Coast);
    BOOST_REQUIRE(texture >= 0x0 && texture <= 0x22);
    texture = mapping.Get(CoastToGreen1);
    BOOST_REQUIRE(texture >= 0x0 && texture <= 0x22);
    texture = mapping.Get(CoastToGreen2);
    BOOST_REQUIRE(texture >= 0x0 && texture <= 0x22);
    texture = mapping.Get(Grass1);
    BOOST_REQUIRE(texture >= 0x0 && texture <= 0x22);
    texture = mapping.Get(Grass2);
    BOOST_REQUIRE(texture >= 0x0 && texture <= 0x22);
    texture = mapping.Get(Grass3);
    BOOST_REQUIRE(texture >= 0x0 && texture <= 0x22);
    texture = mapping.Get(GrassFlower);
    BOOST_REQUIRE(texture >= 0x0 && texture <= 0x22);
    texture = mapping.Get(GrassToMountain);
    BOOST_REQUIRE(texture >= 0x0 && texture <= 0x22);
    texture = mapping.Get(Mountain1);
    BOOST_REQUIRE(texture >= 0x0 && texture <= 0x22);
    texture = mapping.Get(Mountain2);
    BOOST_REQUIRE(texture >= 0x0 && texture <= 0x22);
    texture = mapping.Get(Mountain3);
    BOOST_REQUIRE(texture >= 0x0 && texture <= 0x22);
    texture = mapping.Get(Mountain4);
    BOOST_REQUIRE(texture >= 0x0 && texture <= 0x22);
    texture = mapping.Get(MountainPeak);
    BOOST_REQUIRE(texture >= 0x0 && texture <= 0x22);
    texture = mapping.Get(Lava);
    BOOST_REQUIRE(texture >= 0x0 && texture <= 0x22);
}

BOOST_AUTO_TEST_CASE(WastelandMappingReturnsValidValues)
{
    WastelandMapping mapping;
    unsigned char texture;
    
    texture = mapping.Get(Water);
    BOOST_REQUIRE(texture >= 0x0 && texture <= 0x22);
    texture = mapping.Get(Coast);
    BOOST_REQUIRE(texture >= 0x0 && texture <= 0x22);
    texture = mapping.Get(CoastToGreen1);
    BOOST_REQUIRE(texture >= 0x0 && texture <= 0x22);
    texture = mapping.Get(CoastToGreen2);
    BOOST_REQUIRE(texture >= 0x0 && texture <= 0x22);
    texture = mapping.Get(Grass1);
    BOOST_REQUIRE(texture >= 0x0 && texture <= 0x22);
    texture = mapping.Get(Grass2);
    BOOST_REQUIRE(texture >= 0x0 && texture <= 0x22);
    texture = mapping.Get(Grass3);
    BOOST_REQUIRE(texture >= 0x0 && texture <= 0x22);
    texture = mapping.Get(GrassFlower);
    BOOST_REQUIRE(texture >= 0x0 && texture <= 0x22);
    texture = mapping.Get(GrassToMountain);
    BOOST_REQUIRE(texture >= 0x0 && texture <= 0x22);
    texture = mapping.Get(Mountain1);
    BOOST_REQUIRE(texture >= 0x0 && texture <= 0x22);
    texture = mapping.Get(Mountain2);
    BOOST_REQUIRE(texture >= 0x0 && texture <= 0x22);
    texture = mapping.Get(Mountain3);
    BOOST_REQUIRE(texture >= 0x0 && texture <= 0x22);
    texture = mapping.Get(Mountain4);
    BOOST_REQUIRE(texture >= 0x0 && texture <= 0x22);
    texture = mapping.Get(MountainPeak);
    BOOST_REQUIRE(texture >= 0x0 && texture <= 0x22);
    texture = mapping.Get(Lava);
    BOOST_REQUIRE(texture >= 0x0 && texture <= 0x22);
}

BOOST_AUTO_TEST_CASE(WinterMappingReturnsValidValues)
{
    WinterMapping mapping;
    unsigned char texture;
    
    texture = mapping.Get(Water);
    BOOST_REQUIRE(texture >= 0x0 && texture <= 0x22);
    texture = mapping.Get(Coast);
    BOOST_REQUIRE(texture >= 0x0 && texture <= 0x22);
    texture = mapping.Get(CoastToGreen1);
    BOOST_REQUIRE(texture >= 0x0 && texture <= 0x22);
    texture = mapping.Get(CoastToGreen2);
    BOOST_REQUIRE(texture >= 0x0 && texture <= 0x22);
    texture = mapping.Get(Grass1);
    BOOST_REQUIRE(texture >= 0x0 && texture <= 0x22);
    texture = mapping.Get(Grass2);
    BOOST_REQUIRE(texture >= 0x0 && texture <= 0x22);
    texture = mapping.Get(Grass3);
    BOOST_REQUIRE(texture >= 0x0 && texture <= 0x22);
    texture = mapping.Get(GrassFlower);
    BOOST_REQUIRE(texture >= 0x0 && texture <= 0x22);
    texture = mapping.Get(GrassToMountain);
    BOOST_REQUIRE(texture >= 0x0 && texture <= 0x22);
    texture = mapping.Get(Mountain1);
    BOOST_REQUIRE(texture >= 0x0 && texture <= 0x22);
    texture = mapping.Get(Mountain2);
    BOOST_REQUIRE(texture >= 0x0 && texture <= 0x22);
    texture = mapping.Get(Mountain3);
    BOOST_REQUIRE(texture >= 0x0 && texture <= 0x22);
    texture = mapping.Get(Mountain4);
    BOOST_REQUIRE(texture >= 0x0 && texture <= 0x22);
    texture = mapping.Get(MountainPeak);
    BOOST_REQUIRE(texture >= 0x0 && texture <= 0x22);
    texture = mapping.Get(Lava);
    BOOST_REQUIRE(texture >= 0x0 && texture <= 0x22);
}

BOOST_AUTO_TEST_SUITE_END()
