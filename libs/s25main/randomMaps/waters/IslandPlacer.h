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

#ifndef IslandPlacer_h__
#define IslandPlacer_h__

#include "randomMaps/elevation/HeightSettings.h"
#include "randomMaps/algorithm/Tile.h"
#include "randomMaps/Map.h"
#include <set>

class IslandPlacer
{
private:
    HeightSettings& height_;
    
    TextureType TextureFor(unsigned char z, unsigned char sea, unsigned char mountain);
    static bool IsCoastLand(const Map* map, int index);
    
public:
    IslandPlacer(HeightSettings& height);
    
    void Place(const std::set<Tile, TileCompare>& coverage, Map* map, unsigned char seaLevel);
};

#endif // IslandPlacer_h__
