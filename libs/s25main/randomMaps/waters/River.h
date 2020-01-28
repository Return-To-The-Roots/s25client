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

#ifndef River_h__
#define River_h__

#include "randomMaps/Map.h"
#include "randomMaps/waters/RiverMotion.h"
#include "randomMaps/waters/RiverBrush.h"

class River
{
    
private:

    RiverMotion motion_;
    RiverBrush brush_;
    
    int direction_;
    Tile location_;
    
    std::vector<Tile> coverage_;
    std::vector<River> streams_;
    
public:

    River(RiverMotion motion, RiverBrush brush, int direction, Tile source);
    
    /**
     * Extends the river along the gradient of the current landscape by the specified length (in tiles).
     * @param length length of the river extension in tiles
     * @param size actual map size in x- and y-direction
     * @return the same instance of river only updated by the specified length
     */
    River& ExtendBy(int length, const MapExtent& size);

    /**
     * Changes the direction of the river.
     * @param clockwise whether to steer the river in clockwise or counter-clockwise direction
     * @param swap whether or not to apply a opposite steering direction to child-streams of the river
     * @return the same instance of river with updated direction
     */
    River& Steer(bool clockwise, bool swap);

    /**
     * Splits up the river into another separate stream starting at the current location.
     * To see the effect of the split at least another extension by calling River::ExtendBy is required.
     * @param clockwise direction of the new stream
     * @param recursive whether or not to recursively split sub-streams of the river
     * @return the same instance of river with an additional stream starting at the current location
     */
    River& Split(bool clockwise, bool recursive);

    /**
     * Places the current configuration of the river on the specified map.
     * @param map space to place the river into (e.g. applying water texture and erosion)
     * @param seaLevel the sea level defines the lowest height value the river can have
     */
    void Create(Map* map, unsigned char seaLevel);
};

#endif // River_h__
