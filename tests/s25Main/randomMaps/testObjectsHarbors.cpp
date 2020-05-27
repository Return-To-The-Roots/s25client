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
#include "randomMaps/objects/Harbors.h"

#include <boost/test/unit_test.hpp>
#include <iostream>

BOOST_AUTO_TEST_SUITE(HarborsTests)

BOOST_AUTO_TEST_CASE(PlacesHarborsOnAMapWithCoast)
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
    
    Harbors harbors(10, 5, 1);

    harbors.Place(map);
    
    BOOST_REQUIRE(map.harborsLsd.size() > 0 || map.harborsRsu.size() > 0);
}

BOOST_AUTO_TEST_CASE(HarborsArePlacedOnBuildableTerrain)
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
    
    Harbors harbors(10, 5, 1);

    harbors.Place(map);
    
    for (auto index : map.harborsLsd)
    {
        BOOST_REQUIRE(Texture::IsBuildable(map.textureLsd[index]));
    }

    for (auto index : map.harborsRsu)
    {
        BOOST_REQUIRE(Texture::IsBuildable(map.textureLsd[index]));
    }
}

BOOST_AUTO_TEST_CASE(HarborsArePlacedNextToWater)
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
    
    Harbors harbors(10, 5, 1);

    harbors.Place(map);
    
    for (auto index : map.harborsLsd)
    {
        auto harbor = GridUtility::GetPosition(index, size);
        auto neighbors = GridUtility::Collect(harbor, size, 2.0);
        auto waterNeighbor = false;

        for (auto neighbor : neighbors)
        {
            int neighborIndex = neighbor.x + neighbor.y * size.x;
            if (Texture::IsWater(map.textureRsu[neighborIndex]) ||
                Texture::IsWater(map.textureLsd[neighborIndex]))
            {
                waterNeighbor = true;
                break;
            }
        }
        
        BOOST_REQUIRE(waterNeighbor);
    }

    for (auto index : map.harborsRsu)
    {
        auto harbor = GridUtility::GetPosition(index, size);
        auto neighbors = GridUtility::Collect(harbor, size, 2.0);
        auto waterNeighbor = false;

        for (auto neighbor : neighbors)
        {
            int neighborIndex = neighbor.x + neighbor.y * size.x;
            if (Texture::IsWater(map.textureRsu[neighborIndex]) ||
                Texture::IsWater(map.textureLsd[neighborIndex]))
            {
                waterNeighbor = true;
                break;
            }
        }
        
        BOOST_REQUIRE(waterNeighbor);
    }
}

BOOST_AUTO_TEST_SUITE_END()
