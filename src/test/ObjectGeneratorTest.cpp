// Copyright (c) 2005 - 2015 Settlers Freaks (sf-team at siedler25.org)
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

#include "mapGenerator/ObjectGenerator.h"
#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_SUITE(ObjectGeneratorTest)

/**
 * Tests the ObjectGenerator::GetTextureId method to ensure the method returns correct
 * values for all terrain types.
 */
BOOST_FIXTURE_TEST_CASE(GetTextureId_TerrainType, ObjectGenerator)
{
    BOOST_REQUIRE_EQUAL(ObjectGenerator::GetTextureId(TT_SNOW),             0x02);
    BOOST_REQUIRE_EQUAL(ObjectGenerator::GetTextureId(TT_LAVA),             0x10);
    BOOST_REQUIRE_EQUAL(ObjectGenerator::GetTextureId(TT_LAVA2),            0x10);
    BOOST_REQUIRE_EQUAL(ObjectGenerator::GetTextureId(TT_LAVA3),            0x10);
    BOOST_REQUIRE_EQUAL(ObjectGenerator::GetTextureId(TT_LAVA4),            0x10);
    BOOST_REQUIRE_EQUAL(ObjectGenerator::GetTextureId(TT_WATER),            0x05);
    BOOST_REQUIRE_EQUAL(ObjectGenerator::GetTextureId(TT_WATER_NOSHIP),     0x06);
    BOOST_REQUIRE_EQUAL(ObjectGenerator::GetTextureId(TT_DESERT),           0x04);
    BOOST_REQUIRE_EQUAL(ObjectGenerator::GetTextureId(TT_MOUNTAIN1),        0x01);
    BOOST_REQUIRE_EQUAL(ObjectGenerator::GetTextureId(TT_MOUNTAIN2),        0x0B);
    BOOST_REQUIRE_EQUAL(ObjectGenerator::GetTextureId(TT_MOUNTAIN3),        0x0C);
    BOOST_REQUIRE_EQUAL(ObjectGenerator::GetTextureId(TT_MOUNTAIN4),        0x0D);
    BOOST_REQUIRE_EQUAL(ObjectGenerator::GetTextureId(TT_SWAMPLAND),        0x03);
    BOOST_REQUIRE_EQUAL(ObjectGenerator::GetTextureId(TT_BUILDABLE_WATER),  0x13);
    BOOST_REQUIRE_EQUAL(ObjectGenerator::GetTextureId(TT_STEPPE),           0x0E);
    BOOST_REQUIRE_EQUAL(ObjectGenerator::GetTextureId(TT_SAVANNAH),         0x00);
    BOOST_REQUIRE_EQUAL(ObjectGenerator::GetTextureId(TT_MEADOW1),          0x08);
    BOOST_REQUIRE_EQUAL(ObjectGenerator::GetTextureId(TT_MEADOW2),          0x09);
    BOOST_REQUIRE_EQUAL(ObjectGenerator::GetTextureId(TT_MEADOW3),          0x0A);
    BOOST_REQUIRE_EQUAL(ObjectGenerator::GetTextureId(TT_MEADOW_FLOWERS),   0x0F);
    BOOST_REQUIRE_EQUAL(ObjectGenerator::GetTextureId(TT_MOUNTAINMEADOW),   0x12);
}

/**
 * Tests the ObjectGenerator::IsHarborAllowed method to ensure the method returns correct
 * values for all terrain types.
 */
BOOST_FIXTURE_TEST_CASE(IsHarborAllowed_TerrainType, ObjectGenerator)
{
    BOOST_REQUIRE_EQUAL(ObjectGenerator::IsHarborAllowed(TT_SNOW),              false);
    BOOST_REQUIRE_EQUAL(ObjectGenerator::IsHarborAllowed(TT_LAVA),              false);
    BOOST_REQUIRE_EQUAL(ObjectGenerator::IsHarborAllowed(TT_LAVA2),             false);
    BOOST_REQUIRE_EQUAL(ObjectGenerator::IsHarborAllowed(TT_LAVA3),             false);
    BOOST_REQUIRE_EQUAL(ObjectGenerator::IsHarborAllowed(TT_LAVA4),             false);
    BOOST_REQUIRE_EQUAL(ObjectGenerator::IsHarborAllowed(TT_WATER),             false);
    BOOST_REQUIRE_EQUAL(ObjectGenerator::IsHarborAllowed(TT_WATER_NOSHIP),      false);
    BOOST_REQUIRE_EQUAL(ObjectGenerator::IsHarborAllowed(TT_DESERT),            false);
    BOOST_REQUIRE_EQUAL(ObjectGenerator::IsHarborAllowed(TT_MOUNTAIN1),         false);
    BOOST_REQUIRE_EQUAL(ObjectGenerator::IsHarborAllowed(TT_MOUNTAIN2),         false);
    BOOST_REQUIRE_EQUAL(ObjectGenerator::IsHarborAllowed(TT_MOUNTAIN3),         false);
    BOOST_REQUIRE_EQUAL(ObjectGenerator::IsHarborAllowed(TT_MOUNTAIN4),         false);
    BOOST_REQUIRE_EQUAL(ObjectGenerator::IsHarborAllowed(TT_SWAMPLAND),         false);
    BOOST_REQUIRE_EQUAL(ObjectGenerator::IsHarborAllowed(TT_BUILDABLE_WATER),   false);
    BOOST_REQUIRE_EQUAL(ObjectGenerator::IsHarborAllowed(TT_STEPPE),            true);
    BOOST_REQUIRE_EQUAL(ObjectGenerator::IsHarborAllowed(TT_SAVANNAH),          true);
    BOOST_REQUIRE_EQUAL(ObjectGenerator::IsHarborAllowed(TT_MEADOW1),           true);
    BOOST_REQUIRE_EQUAL(ObjectGenerator::IsHarborAllowed(TT_MEADOW2),           true);
    BOOST_REQUIRE_EQUAL(ObjectGenerator::IsHarborAllowed(TT_MEADOW3),           true);
    BOOST_REQUIRE_EQUAL(ObjectGenerator::IsHarborAllowed(TT_MEADOW_FLOWERS),    true);
    BOOST_REQUIRE_EQUAL(ObjectGenerator::IsHarborAllowed(TT_MOUNTAINMEADOW),    true);
}

BOOST_AUTO_TEST_SUITE_END()

