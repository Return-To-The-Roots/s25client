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
#include "randomMaps/terrain/CoastlineCalculator.h"

bool CoastlineCalculator::Has40PercentWaterTilesInNeighborhood(const Position& position,
                                                               const std::vector<bool>& water,
                                                               const MapExtent& size)
{
    auto neighbors = GridUtility::Collect(position, size, 4.0);
    auto waterNeighbors = 0;
    
    for (auto neighbor: neighbors)
    {
        if (water[neighbor.x + neighbor.y * size.x])
        {
            waterNeighbors++;
        }
    }
    
    return (double) waterNeighbors / neighbors.size() >= 0.4;
}

Coastline CoastlineCalculator::For(const std::vector<Position>& island,
                                   const std::vector<bool>& water,
                                   const MapExtent& size)
{
    Coastline coastline;
    
    for (auto vertex: island)
    {
        auto neighbors = GridUtility::Neighbors(vertex, size);
        auto calculatedWaterNeighborhood = false;
        auto has40PercentWaterNeighbors = false;
        
        for (auto neighbor: neighbors)
        {
            if (water[neighbor.x + neighbor.y * size.x])
            {
                if (!calculatedWaterNeighborhood)
                {
                    has40PercentWaterNeighbors = Has40PercentWaterTilesInNeighborhood(vertex, water, size);
                    calculatedWaterNeighborhood = true;
                }
                
                if (has40PercentWaterNeighbors)
                {
                    coastline.push_back(CoastTile(vertex,neighbor));
                }
                
                break;
            }
        }
    }
    
    return coastline;
}
