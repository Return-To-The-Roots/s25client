// Copyright (c) 2005 - 2017 Settlers Freaks (sf-team at siedler25.org)
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

#ifndef CoastlineCalculator_h__
#define CoastlineCalculator_h__

#include "randomMaps/terrain/CoastTile.h"
#include "randomMaps/algorithm/GridUtility.h"

typedef std::vector<CoastTile> Coastline;

class CoastlineCalculator {
    
    /**
     * Checks whether or not the specified position is surounded by water within a 4-tile
     * radius. If there's less than 40% water, the specified position can be considered non-
     * coastline, because it's either near a tiny river or just not close to water at all.
     * @param position position to check for water neighborhood
     * @param water vectors of boolean values specifying whether or not a position for
     * the specified index is covered by water or not.
     * @param size size of the map
     * @return true if at least 40% of positions within a 4-tile radius are covered by water, false otherwise.
     */
    bool Has40PercentWaterTilesInNeighborhood(const Position& position,
                                              const std::vector<bool>& water,
                                              const MapExtent& size);
    
public:
    
    /**
     * Computes the coastline for the speicifed island.
     * @param island all positions covered by the island
     * @param water vectors of boolean values specifying whether or not a position for
     * the specified index is covered by water or not.
     * @param size size of the map
     * @return the coastline of the island.
     */
    Coastline For(const std::vector<Position>& island,
                  const std::vector<bool>& water,
                  const MapExtent& size);
};

#endif // CoastlineCalculator_h__
