// Copyright (c) 2005 - 2015 Settlers Freaks (sf-team at siedler25.org)
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

#include "defines.h" // IWYU pragma: keep
#include "world/MapGeometry.h"
#include <cmath>
#include <algorithm>
#include <cassert>
#include <cstdlib>

// Include last!
#include "DebugNew.h" // IWYU pragma: keep

Point<int> GetPointAround(const Point<int>& p, unsigned dir)
{
    switch(dir)
    {
        case 0:
            return Point<int>(p.x - 1, p.y);
        case 1:
            return Point<int>(p.x - !(p.y&1), p.y-1);
        case 2:
            return Point<int>(p.x + (p.y&1), p.y-1);
        case 3:
            return Point<int>(p.x + 1, p.y);
        case 4:
            return Point<int>(p.x + (p.y&1), p.y+1);
        default:
            RTTR_Assert(dir == 5);
            return Point<int>(p.x - !(p.y & 1), p.y + 1);
    }
}

