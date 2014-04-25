// $Id: MapGeometry.cpp 3120
//
// Copyright (c) 2005 - 2011 Settlers Freaks (sf-team at siedler25.org)
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

#include "main.h"
#include "MapGeometry.h"
#include <cmath>
#include <algorithm>
#include <cassert>




unsigned CalcRawDistance(const int x1, const int y1,
                         const int x2, const int y2)
{
    int dx = std::abs((2 * int(x1) + (y1 & 1)) - (2 * int(x2) + (y2 & 1))), dy = std::abs(2 * int(y1) - 2 * int(y2));
    return (dy + std::max(0, dx - dy / 2)) / 2;

    //int dx = std::abs(int(x1)-int(x2)), dy = std::abs(int(y1)-int(y2));
    //return dy + std::max(0,dx+((dy&1)-dy)/2);
}

Point<int> GetPointAround(Point<int> p, unsigned dir)
{
    switch(dir)
    {
        case 0: return Point<int>(p.x - 1, p.y);
        case 1: return Point<int>(p.x - !(p.y&1), p.y-1);
        case 2: return Point<int>(p.x + (p.y&1), p.y-1); break;
        case 3: return Point<int>(p.x + 1, p.y); break;
        case 4: return Point<int>(p.x + (p.y&1), p.y+1); break;
        case 5: return Point<int>(p.x - !(p.y&1), p.y+1); break;
        default: assert(false); return Point<int>(0xffffff, 0xffffff);
    }
}

