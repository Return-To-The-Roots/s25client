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

#include "mapGenerator/RandomConfig.h"
#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_SUITE(RandomConfigTest)

/**
 * Tests the RandomConfig::CreateRandom method to ensure the maximum height of an area does
 * not exceed the number of textures.
 */
BOOST_FIXTURE_TEST_CASE(CreateRandom_MaxHeightBelowTextureCount, RandomConfig)
{
    RandomConfig config = RandomConfig::CreateRandom();
    for (std::vector<AreaDesc>::iterator it = config.areas.begin(); it != config.areas.end(); ++it)
    {
        BOOST_REQUIRE_LT((unsigned)it->maxElevation, config.textures.size());
    }
}

/**
 * Tests the RandomConfig::CreateIslands method to ensure the maximum height of an area does
 * not exceed the number of textures.
 */
BOOST_FIXTURE_TEST_CASE(CreateIslands_MaxHeightBelowTextureCount, RandomConfig)
{
    RandomConfig config = RandomConfig::CreateIslands();
    for (std::vector<AreaDesc>::iterator it = config.areas.begin(); it != config.areas.end(); ++it)
    {
        BOOST_REQUIRE_LT((unsigned)it->maxElevation, config.textures.size());
    }
}

/**
 * Tests the RandomConfig::CreateRingland method to ensure the maximum height of an area does
 * not exceed the number of textures.
 */
BOOST_FIXTURE_TEST_CASE(CreateRingland_MaxHeightBelowTextureCount, RandomConfig)
{
    RandomConfig config = RandomConfig::CreateRingland();
    for (std::vector<AreaDesc>::iterator it = config.areas.begin(); it != config.areas.end(); ++it)
    {
        BOOST_REQUIRE_LT((unsigned)it->maxElevation, config.textures.size());
    }
}

/**
 * Tests the RandomConfig::CreateContinent method to ensure the maximum height of an area does
 * not exceed the number of textures.
 */
BOOST_FIXTURE_TEST_CASE(CreateContinent_MaxHeightBelowTextureCount, RandomConfig)
{
    RandomConfig config = RandomConfig::CreateContinent();
    for (std::vector<AreaDesc>::iterator it = config.areas.begin(); it != config.areas.end(); ++it)
    {
        BOOST_REQUIRE_LT((unsigned)it->maxElevation, config.textures.size());
    }
}

/**
 * Tests the RandomConfig::CreateGreenland method to ensure the maximum height of an area does
 * not exceed the number of textures.
 */
BOOST_FIXTURE_TEST_CASE(CreateGreenland_MaxHeightBelowTextureCount, RandomConfig)
{
    RandomConfig config = RandomConfig::CreateGreenland();
    for (std::vector<AreaDesc>::iterator it = config.areas.begin(); it != config.areas.end(); ++it)
    {
        BOOST_REQUIRE_LT((unsigned)it->maxElevation, config.textures.size());
    }
}

/**
 * Tests the RandomConfig::CreateMigration method to ensure the maximum height of an area does
 * not exceed the number of textures.
 */
BOOST_FIXTURE_TEST_CASE(CreateMigration_MaxHeightBelowTextureCount, RandomConfig)
{
    RandomConfig config = RandomConfig::CreateMigration();
    for (std::vector<AreaDesc>::iterator it = config.areas.begin(); it != config.areas.end(); ++it)
    {
        BOOST_REQUIRE_LT((unsigned)it->maxElevation, config.textures.size());
    }
}

/**
 * Tests the RandomConfig::CreateRiverland method to ensure the maximum height of an area does
 * not exceed the number of textures.
 */
BOOST_FIXTURE_TEST_CASE(CreateRiverland_MaxHeightBelowTextureCount, RandomConfig)
{
    RandomConfig config = RandomConfig::CreateRiverland();
    for (std::vector<AreaDesc>::iterator it = config.areas.begin(); it != config.areas.end(); ++it)
    {
        BOOST_REQUIRE_LT((unsigned)it->maxElevation, config.textures.size());
    }
}

BOOST_AUTO_TEST_SUITE_END()

