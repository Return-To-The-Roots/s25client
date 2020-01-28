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

#ifndef RandomRiver_h__
#define RandomRiver_h__

#include "randomMaps/Map.h"
#include "randomMaps/waters/RiverBrush.h"
#include "randomMaps/algorithm/RandomUtility.h"

class RandomRiver
{
    
private:

    RandomUtility rnd_;
    RiverBrush brush_;
    
public:

    RandomRiver(RandomUtility rnd, RiverBrush brush);
    
    /**
     * Places the current configuration of the river on the specified map.
     * @param map space to place the river into (e.g. applying water texture and erosion)
     * @param direction initial direction of the river's stream
     * @param source location of the river's source
     * @param length length of the river in tiles
     * @param seaLevel the sea level defines the lowest height value the river can have
     */
    void Create(Map* map, Tile source, int direction, int length, unsigned char seaLevel);
};

#endif // RandomRiver_h__
