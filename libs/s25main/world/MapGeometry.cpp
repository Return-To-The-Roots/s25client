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

#include "rttrDefines.h" // IWYU pragma: keep
#include "world/MapGeometry.h"
#include "gameData/MapConsts.h"
#include <stdexcept>

Position GetNeighbour(const Position& p, const Direction dir)
{
    /*  Note that every 2nd row is shifted by half a triangle to the left, therefore:
    Modifications for the dirs:
    current row:    Even    Odd
                 W  -1|0   -1|0
    D           NW  -1|-1   0|-1
    I           NE   0|-1   1|-1
    R            E   1|0    1|0
                SE   0|1    1|1
                SW  -1|1    0|1
    */
    switch(dir.native_value())
    {
        case Direction::WEST: return Position(p.x - 1, p.y);
        case Direction::NORTHWEST: return Position(p.x - !(p.y & 1), p.y - 1);
        case Direction::NORTHEAST: return Position(p.x + (p.y & 1), p.y - 1);
        case Direction::EAST: return Position(p.x + 1, p.y);
        case Direction::SOUTHEAST: return Position(p.x + (p.y & 1), p.y + 1);
        default: RTTR_Assert(dir == Direction::SOUTHWEST); return Position(p.x - !(p.y & 1), p.y + 1);
    }
}

Position GetNeighbour2(Position pt, unsigned dir)
{
    if(dir >= 12)
        throw std::logic_error("Invalid direction!");

    static const int ADD_Y[12] = {0, -1, -2, -2, -2, -1, 0, 1, 2, 2, 2, 1};

    switch(dir)
    {
        case 0: pt.x -= 2; break;
        case 1: pt.x -= 2 - ((pt.y & 1) ? 1 : 0); break;
        case 2: pt.x -= 1; break;
        case 3: break;
        case 4: pt.x += 1; break;
        case 5: pt.x += 2 - ((pt.y & 1) ? 0 : 1); break;
        case 6: pt.x += 2; break;
        case 7: pt.x += 2 - ((pt.y & 1) ? 0 : 1); break;
        case 8: pt.x += 1; break;
        case 9: break;
        case 10: pt.x -= 1; break;
        default: pt.x -= 2 - ((pt.y & 1) ? 1 : 0); break;
    }
    pt.y += ADD_Y[dir];
    return pt;
}

MapPoint MakeMapPoint(Position pt, const MapExtent& size)
{
    // Shift into range
    pt.x %= size.x;
    pt.y %= size.y;
    // Handle negative values (sign is implementation defined, but |value| < width)
    if(pt.x < 0)
        pt.x += size.x;
    if(pt.y < 0)
        pt.y += size.y;
    RTTR_Assert(pt.x >= 0 && pt.y >= 0);
    RTTR_Assert(static_cast<unsigned>(pt.x) < size.x && static_cast<unsigned>(pt.y) < size.y);
    return MapPoint(pt);
}

Position GetNodePos(MapPoint pt)
{
    return GetNodePos(Position(pt));
}

Position GetNodePos(Position pt)
{
    Position result = pt * Position(TR_W, TR_H);
    if(pt.y & 1)
        result.x += TR_W / 2;
    return result;
}

Position GetNodePos(MapPoint pt, uint8_t height)
{
    Position result = GetNodePos(pt);
    result.y -= HEIGHT_FACTOR * height;
    return result;
}
