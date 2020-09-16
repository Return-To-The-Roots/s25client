// Copyright (c) 2017 - 2020 Settlers Freaks (sf-team at siedler25.org)
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
#include "mapGenerator/RandomConfig.h"
#include "libsiedler2/enumTypes.h"
#include <boost/test/unit_test.hpp>

namespace {
class ObjGenFixture
{
protected:
    RandomConfig config;
    ObjectGenerator objGen;

public:
    ObjGenFixture() : objGen(config)
    {
        BOOST_REQUIRE(config.Init(MapStyle::Random, DescIdx<LandscapeDesc>(0), 0x1337));
    }
};
} // namespace

namespace libsiedler2 {
static std::ostream& boost_test_print_type(std::ostream& os, Animal animal)
{
    return os << static_cast<unsigned>(animal);
}
} // namespace libsiedler2

BOOST_AUTO_TEST_SUITE(ObjectGeneratorTest)

/**
 * Tests the ObjectGenerator::CreateDuck method with a likelihood of 100%. A duck should
 * be returned ;-).
 */
BOOST_FIXTURE_TEST_CASE(CreateDuck_FullLikelyhood, ObjGenFixture)
{
    BOOST_REQUIRE_EQUAL(objGen.CreateDuck(100), libsiedler2::Animal::Duck);
}

/**
 * Tests the ObjectGenerator::CreateDuck method with a likelihood of 0%. An empty object
 * should be returned.
 */
BOOST_FIXTURE_TEST_CASE(CreateDuck_ZeroLikelyhood, ObjGenFixture)
{
    BOOST_REQUIRE_EQUAL(objGen.CreateDuck(0), libsiedler2::Animal::None);
}

/**
 * Tests the ObjectGenerator::IsTree method for a tree object. Should return true.
 */
BOOST_AUTO_TEST_CASE(IsTree_TreeExists)
{
    Map map(MapExtent(16, 8), "name", "author");

    map.objectInfo[0] = libsiedler2::OI_Palm;

    BOOST_REQUIRE_EQUAL(ObjectGenerator::IsTree(map, 0), true);

    map.objectInfo[0] = libsiedler2::OI_TreeOrPalm;

    BOOST_REQUIRE_EQUAL(ObjectGenerator::IsTree(map, 0), true);
}

/**
 * Tests the ObjectGenerator::IsTree method for an empty tile. Should return false.
 */
BOOST_AUTO_TEST_CASE(IsTree_Empty)
{
    Map map(MapExtent(16, 8), "name", "author");

    BOOST_REQUIRE_EQUAL(ObjectGenerator::IsTree(map, 0), false);
}

BOOST_FIXTURE_TEST_CASE(CreateTexture_Harbor, ObjGenFixture)
{
    Map map(MapExtent(16, 8), "name", "author");

    DescIdx<TerrainDesc> t = config.FindTerrain(
      [](const auto& desc) { return desc.kind == TerrainKind::WATER && desc.Is(ETerrain::Shippable); });
    uint8_t water = config.worldDesc.get(t).s2Id;

    /**
     * Tests the ObjectGenerator::CreateTexture method without harbor.
     * The specified texture should be replaced.
     */
    objGen.CreateTexture(map, 0, t, false);

    BOOST_REQUIRE_EQUAL(map.textureRsu[0], water);
    BOOST_REQUIRE_EQUAL(map.textureLsd[0], water);

    /**
     * Tests the ObjectGenerator::CreateTexture method with harbor but non-harbor texture.
     * The specified texture should be replaced without a harbor texture.
     */
    objGen.CreateTexture(map, 0, t, true);

    BOOST_REQUIRE_EQUAL(map.textureRsu[0], water);
    BOOST_REQUIRE_EQUAL(map.textureLsd[0], water);

    t = config.FindTerrain(
      [](const auto& desc) { return desc.kind == TerrainKind::LAND && desc.Is(ETerrain::Buildable); });
    uint8_t meadow = config.worldDesc.get(t).s2Id;
    /**
     * Tests the ObjectGenerator::CreateTexture method with harbor.
     * The specified texture should be replaced with a harbor texture.
     */
    objGen.CreateTexture(map, 0, t, false);

    BOOST_REQUIRE_EQUAL(map.textureRsu[0], meadow);
    BOOST_REQUIRE_EQUAL(map.textureLsd[0], meadow);

    objGen.CreateTexture(map, 0, t, true);

    BOOST_REQUIRE_EQUAL(map.textureRsu[0], (meadow | libsiedler2::HARBOR_MASK));
    BOOST_REQUIRE_EQUAL(map.textureLsd[0], (meadow | libsiedler2::HARBOR_MASK));
}

BOOST_AUTO_TEST_SUITE_END()
