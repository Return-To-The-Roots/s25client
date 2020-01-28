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
#include "randomMaps/waters/RiverMotion.h"

RiverMotion::RiverMotion(RandomUtility rnd) : rnd_(rnd)
{
    
}

RiverMotion RiverMotion::Clone()
{
    return RiverMotion(rnd_);
}

Tile RiverMotion::Next(Tile& tile, int direction, const MapExtent& size)
{
    auto neighbors = tile.Neighbors(size);
    auto index = direction + rnd_.Rand(-1, 1);
    
    return index < 0 ? neighbors[7] : neighbors[index % 8];
}
