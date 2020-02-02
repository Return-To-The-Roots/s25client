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

#ifndef Types_h__
#define Types_h__

#include "gameTypes/MapCoordinates.h"
#include "gameData/WorldDescription.h"

#include <vector>

namespace rttr {
namespace mapGenerator {

struct Range
{
    Range(int min, int max) : min(min), max(max) {}
    
    const int min;
    const int max;
    
    int Difference() const
    {
        return max - min;
    }
};

struct CoastNode
{
    CoastNode(Position land, Position water) : land(land), water(water) {}
    
    Position land;
    Position water;
};

using Points     = std::vector<MapPoint>;

using Animal     = uint8_t;
using Animals    = std::vector<Animal>;

using Height     = uint8_t;
using HeightMap  = std::vector<Height>;

using Texture    = DescIdx<TerrainDesc>;
using Textures   = std::vector<Texture>;

struct TexturePair
{
    Texture rsu;
    Texture lsd;
    
    TexturePair() : rsu(Texture::INVALID), lsd(Texture::INVALID) {}
    TexturePair(Texture texture) : rsu(texture), lsd(texture) {}
};

using Coast      = std::vector<CoastNode>;
using Island     = std::vector<Position>;
using Islands    = std::vector<Island>;
using WaterMap   = std::vector<bool>;

}}

#endif // Types_h__
