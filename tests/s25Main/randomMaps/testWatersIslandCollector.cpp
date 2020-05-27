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
#include "randomMaps/waters/IslandCollector.h"

#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_SUITE(IslandCollectorTest)

BOOST_AUTO_TEST_CASE(CollectsCorrectNumberOfIslandsForMinimumTilesPerIsland)
{
    const int width = 16;
    const int height = 32;
    const int minimumTilesPerIsland = 2;
    
    MapExtent size(width, height);
    IslandCollector islandCollector;
    
    std::vector<bool> waterMap(width * height, true);
    
    std::vector<Position> landTiles = {
        // 1st island
        Position(2,2), Position(3,2),

        // island (too small)
        Position(4,4),

        // 2nd island
        Position(6,6), Position(6,7), Position(7,6),
        
        // 3rd island
        Position(12,3), Position(12,4)
    };
    
    for (auto landTile = landTiles.begin(); landTile != landTiles.end(); landTile++)
    {
        waterMap[landTile->x + landTile->y * width] = false;
    }
    
    Islands islands = islandCollector.Collect(waterMap, size, minimumTilesPerIsland);
    
    BOOST_REQUIRE(islands.size() == 3);
}

BOOST_AUTO_TEST_SUITE_END()
