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

#ifndef IslandCollector_h__
#define IslandCollector_h__

#include "randomMaps/waters/Islands.h"

class IslandCollector
{
private:
    
    bool HasLandTile(const std::vector<bool>& excludedTiles);
    
    Position NextLandTile(const std::vector<bool>& excludedTiles,
                          const MapExtent& size);
    
public:
    
     Islands Collect(const std::vector<bool>& waterMap,
                     const MapExtent& size,
                     const int unsigned minimumTilesPerIsland);
};

#endif // IslandCollector_h__
