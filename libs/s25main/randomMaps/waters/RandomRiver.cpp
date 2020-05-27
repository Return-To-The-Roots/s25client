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
#include "randomMaps/waters/RandomRiver.h"
#include "randomMaps/waters/River.h"

RandomRiver::RandomRiver(RandomUtility rnd, RiverBrush brush)
    : rnd_(rnd), brush_(brush)
{
    
}

void RandomRiver::Create(Map* map, Tile source, int direction, int length, unsigned char seaLevel)
{
    MapExtent size(map->size);
    River river(RiverMotion(rnd_), brush_, direction, source);
    
    // 10 times chance of split
    int splitIndex = length / 10;
    
    // 5 times chance of changing direction
    int steerIndex = length / 5;
    
    for (int i = 1; i <= length; i++)
    {
        if (splitIndex > 1 && i % splitIndex == 0)
        {
            // 20% chance to actually perform a split of the river
            if (rnd_.ByChance(20))
            {
                // 50% chance that the new river stream goes into
                // clockwise direction compared to the current river's
                // direction. Otherwise counter-clockwise.
                bool clockwise = rnd_.ByChance(50);

                // 10% chance to recursively split all streams of the
                // river, not just the main river.
                bool recursive = rnd_.ByChance(10);
                
                river.Split(clockwise, recursive);
            }
        }
        
        if (steerIndex > 1 && i % steerIndex == 0)
        {
            // 20% chance to change direction of the river
            if (rnd_.ByChance(20))
            {
                // 50% chance to change direction clockwise otherwise
                // into counter-clockwise direction.
                bool clockwise = rnd_.ByChance(50);
                
                // 50% chance to swap direction of child streams of the river
                bool swap = rnd_.ByChance(50);
                
                river.Steer(clockwise, swap);
            }
        }
        
        river.ExtendBy(1, size);
    }
    
    river.Create(map, seaLevel);
}
