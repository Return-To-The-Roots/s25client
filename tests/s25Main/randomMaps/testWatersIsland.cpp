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
#include "randomMaps/waters/Island.h"

#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_SUITE(IslandTests)

BOOST_AUTO_TEST_CASE(IslandGeneratesLandAreaOfSpecifiedSize)
{
    MapExtent size(8,8);
    Map map(size, "test", "test");
    
    map.textureLsd = std::vector<TextureType>(64, Water);
    map.textureRsu = std::vector<TextureType>(64, Water);
    map.objectInfo.resize(64u, 0x0);
    map.objectType.resize(64u, 0x0);
    map.z = std::vector<unsigned char>(64, 0x0);
    
    HeightSettings settings(0, 10);
    RandomUtility rnd(0);
    IslandPlacer placer(settings);
    Island(rnd, placer)
        .OfDistance(1)
        .OfLength(3)
        .OfSize(7)
        .Place(&map, 0);
    
    int landArea = 0;
    
    for (int i = 0; i < 64; i++)
    {
        if (map.textureRsu[i] != Water || map.textureLsd[i] != Water)
        {
            landArea++;
        }
    }
    
    BOOST_REQUIRE(landArea >= 6 && landArea <= 8);
}

BOOST_AUTO_TEST_CASE(IslandHasExpectedDistanceToLand)
{
    MapExtent size(8,8);
    Map map(size, "test", "test");
    
    std::vector<TextureType> textures = {
        Coast, Coast, Coast, Coast, Coast, Coast, Coast, Coast,
        Water, Water, Water, Water, Water, Water, Water, Water,
        Water, Water, Water, Water, Water, Water, Water, Water,
        Water, Water, Water, Water, Water, Water, Water, Water,
        Water, Water, Water, Water, Water, Water, Water, Water,
        Water, Water, Water, Water, Water, Water, Water, Water,
        Water, Water, Water, Water, Water, Water, Water, Water,
        Water, Water, Water, Water, Water, Water, Water, Water
    };
    
    map.textureLsd = std::vector<TextureType>(textures);
    map.textureRsu = std::vector<TextureType>(textures);
    map.objectInfo.resize(64u, 0x0);
    map.objectType.resize(64u, 0x0);
    map.z.resize(64, 0x0);
    
    int islandDistance = 2;
    
    HeightSettings settings(0, 10);
    RandomUtility rnd(0);
    IslandPlacer placer(settings);
    Island(rnd, placer)
        .OfDistance(islandDistance)
        .OfLength(3)
        .OfSize(4)
        .Place(&map, 0);

    for (int i = 8 /*skip initial land*/; i < 64; i++)
    {
        // find island tiles (non-water)
        if (map.textureRsu[i] != Water || map.textureLsd[i] != Water)
        {
            // ensure that island tiles are not close to initial land
            auto p = GridUtility::GetPosition(i, size);
            auto distance = GridUtility::Distance(p, Position(p.x, 0), size);
            
            BOOST_REQUIRE(distance >= islandDistance);
        }
    }
}

BOOST_AUTO_TEST_SUITE_END()
