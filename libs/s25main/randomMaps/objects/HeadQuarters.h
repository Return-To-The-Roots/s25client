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

#ifndef HeadQuarters_h__
#define HeadQuarters_h__

#include "randomMaps/Map.h"
#include "randomMaps/algorithm/RandomUtility.h"

class HeadQuarters
{
private:
    
    RandomUtility& rnd_;
    
public:

    HeadQuarters(RandomUtility& rnd) : rnd_(rnd) {}
    
    /**
     * Resets all HQ positions.
     * @param map reference to the map to remove HQ positions from
     */
    void Reset(Map& map);

    /**
     * Places a number of headquarters.
     * @param map map to place the HQs on
     * @param number number of HQs to place - equal to the number of players
     * @param remainingTries number of remaining tries to place valid HQs on this map
     * @return false if no valid HQ position was found for at least one player, true otherwise
     */
    bool Place(Map& map, int number, int remainingTries = 10);

    /**
     * Places the header quater for a single player within the specified area.
     * @param map map to place the HQ on
     * @param index index of the HQ which correlates with the player number
     * @param area area of possible HQ positions
     * @return false if no valid HQ position was found, true otherwise
     */
    bool Place(Map& map, int index, const std::vector<Position>& area);
};

#endif // HeadQuarters_h__
