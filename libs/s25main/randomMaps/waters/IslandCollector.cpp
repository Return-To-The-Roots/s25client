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
#include "randomMaps/algorithm/GridUtility.h"

Position IslandCollector::NextLandTile(const std::vector<bool>& excludedTiles,
                                       const MapExtent& size)
{
    for (auto i = 0; i < size.x * size.y; i++)
    {
        if (!excludedTiles[i])
        {
            return GridUtility::GetPosition(i, size);
        }
    }
    return Position::Invalid();
}

Islands IslandCollector::Collect(const std::vector<bool>& waterMap,
                                 const MapExtent& size,
                                 const unsigned int minimumTilesPerIsland)
{
    Islands islands;
    
    auto excludedTiles(waterMap);
    auto currentTile = NextLandTile(excludedTiles, size);
    
    while (currentTile.isValid())
    {
        auto island = GridUtility::Collect(currentTile, size, waterMap);

        for (auto tile = island.begin(); tile != island.end(); tile++)
        {
            excludedTiles[tile->x + tile->y * size.x] = true;
        }
        
        currentTile = NextLandTile(excludedTiles, size);
        
        if (island.size() >= minimumTilesPerIsland)
        {
            islands.push_back(island);
        }
    }
    
    return islands;
}
