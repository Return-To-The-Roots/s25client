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
#include "randomMaps/Continent.h"
#include "randomMaps/algorithm/GridUtility.h"

#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_SUITE(ContinentTests)

BOOST_AUTO_TEST_CASE(ContinentGeneratesValidMaps)
{
    RandomUtility rnd(0);
    MapSettings settings;
    
    settings.author = "test";
    settings.name = "test";
    settings.size = MapExtent(64,64);
    settings.numPlayers = 7u;
    
    Continent generator(rnd);
    
    Map* map = generator.Create(settings);
    
    // ==========================
    // = Validation of map size =
    // ==========================
    BOOST_REQUIRE(map != NULL);
    BOOST_REQUIRE(map->size.x == settings.size.x);
    BOOST_REQUIRE(map->size.y == settings.size.y);
    auto gridSize = (unsigned)settings.size.x * settings.size.y;
    
    // ==============================
    // = Validation of HQ positions =
    // ==============================
    for (int i = 0; i < int(settings.numPlayers); i++)
    {
        BOOST_REQUIRE(map->hqPositions[i].isValid());
    }

    auto hqs = map->hqPositions;
    
    for (int i = 0; i < 7; i++)
    {
        for (int k = i + 1; k < 7; k++)
        {
            BOOST_REQUIRE(hqs[i].x != hqs[k].x || hqs[i].y != hqs[k].y);
        }
    }
    
    // ================================
    // = Validation of map properties =
    // ================================
    BOOST_REQUIRE(map->z.size() == gridSize);
    BOOST_REQUIRE(map->textureRsu.size() == gridSize);
    BOOST_REQUIRE(map->textureLsd.size() == gridSize);
    BOOST_REQUIRE(map->build.size() == gridSize);
    BOOST_REQUIRE(map->shading.size() == gridSize);
    BOOST_REQUIRE(map->resource.size() == gridSize);
    BOOST_REQUIRE(map->road.size() == gridSize);
    BOOST_REQUIRE(map->objectType.size() == gridSize);
    BOOST_REQUIRE(map->objectInfo.size() == gridSize);
    BOOST_REQUIRE(map->animal.size() == gridSize);
    BOOST_REQUIRE(map->unknown1.size() == gridSize);
    BOOST_REQUIRE(map->unknown2.size() == gridSize);
    BOOST_REQUIRE(map->unknown3.size() == gridSize);
    BOOST_REQUIRE(map->unknown5.size() == gridSize);
}

BOOST_AUTO_TEST_SUITE_END()
