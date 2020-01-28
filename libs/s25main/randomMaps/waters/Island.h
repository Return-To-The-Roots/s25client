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

#ifndef Island_h__
#define Island_h__

#include "randomMaps/Map.h"
#include "randomMaps/algorithm/RandomUtility.h"
#include "randomMaps/algorithm/Tile.h"
#include "randomMaps/waters/IslandPlacer.h"

class Island
{
private:
    
    RandomUtility& rnd_;
    IslandPlacer& placer_;
    
    int size_;
    int length_;
    int dist_;
    
    bool HasLandNeibghor(Map* map, const Tile& tile);
    static bool IsLand (const Map* map, int index);

public:
        
    Island(RandomUtility& rnd, IslandPlacer& placer);

    Island& OfSize(int size);

    Island& OfLength(int length);
    
    Island& OfDistance(int dist);
    
    std::vector<Position> Place(Map* map, unsigned char seaLevel);
};

#endif // Island_h__
