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

#include "world/MapBase.h"
#include "commonDefines.h"
#include "world/MapGeometry.h"
#include "gameData/MapConsts.h"
#include <stdexcept>
#include <string>

unsigned MapBase::CreateGUIID(MapPoint pt)
{
    return pt.y * MAX_MAP_SIZE + pt.x;
}

MapBase::MapBase() : size_(MapExtent::all(0)) {}

void MapBase::Resize(const MapExtent& newSize)
{
    // Odd heights make the map impossible (map wraps around so start and end must match)
    // For width it is technically possible to have odd sizes so we don't check this
    if(newSize.y & 1)
        throw std::invalid_argument("The map height must be even!");
    if(newSize.x > MAX_MAP_SIZE || newSize.y > MAX_MAP_SIZE)
        throw std::invalid_argument("Can't load a map bigger than " + std::to_string(MAX_MAP_SIZE)
                                    + " nodes per direction");
    size_ = newSize;
}

MapPoint MapBase::GetNeighbour(const MapPoint pt, const Direction dir) const
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

    MapPoint res;
    switch(dir.native_value())
    {
        case Direction::WEST: // -1|0   -1|0
            res.x = ((pt.x == 0) ? size_.x : pt.x) - 1;
            res.y = pt.y;
            break;
        case Direction::NORTHWEST: // -1|-1   0|-1
            res.x = (pt.y & 1) ? pt.x : (((pt.x == 0) ? size_.x : pt.x) - 1);
            res.y = ((pt.y == 0) ? size_.y : pt.y) - 1;
            break;
        case Direction::NORTHEAST: // 0|-1  -1|-1
            res.x = (!(pt.y & 1)) ? pt.x : ((pt.x == size_.x - 1) ? 0 : pt.x + 1);
            res.y = ((pt.y == 0) ? size_.y : pt.y) - 1;
            break;
        case Direction::EAST: // 1|0    1|0
            res.x = pt.x + 1;
            if(res.x == size_.x)
                res.x = 0;
            res.y = pt.y;
            break;
        case Direction::SOUTHEAST: // 1|1    0|1
            res.x = (!(pt.y & 1)) ? pt.x : ((pt.x == size_.x - 1) ? 0 : pt.x + 1);
            res.y = pt.y + 1;
            if(res.y == size_.y)
                res.y = 0;
            break;
        default:
            RTTR_Assert(dir == Direction::SOUTHWEST);                         // 0|1   -1|1
            res.x = (pt.y & 1) ? pt.x : (((pt.x == 0) ? size_.x : pt.x) - 1); //-V537
            res.y = pt.y + 1;
            if(res.y == size_.y)
                res.y = 0;
            break;
    }

    // This should be the same, but faster
    // RTTR_Assert(res == MakeMapPoint(::GetNeighbour(Position(pt), dir)));
    return res;
}

MapPoint MapBase::GetNeighbour2(const MapPoint pt, unsigned dir) const
{
    return MakeMapPoint(::GetNeighbour2(Position(pt), dir));
}

std::vector<MapPoint> MapBase::GetNeighbours(const MapPoint& pt) const
{
    unsigned yplus1 = pt.y == size_.y - 1 ? 0 : pt.y + 1;
    unsigned yminus1 = (pt.y == 0 ? size_.y : pt.y) - 1;
    unsigned xplus1 = pt.x == size_.x - 1 ? 0 : pt.x + 1;
    unsigned xminus1 = (pt.x == 0 ? size_.x : pt.x) - 1;

    return {MapPoint(xminus1, pt.y), MapPoint((pt.y & 1) ? pt.x : xminus1, yminus1),  MapPoint((!(pt.y & 1)) ? pt.x : xplus1, yminus1),
            MapPoint(xplus1, pt.y),  MapPoint((!(pt.y & 1)) ? pt.x : xplus1, yplus1), MapPoint((pt.y & 1) ? pt.x : xminus1, yplus1)};
}

unsigned MapBase::CalcDistance(const Position& p1, const Position& p2) const
{
    int dx = ((p1.x - p2.x) * 2) + (p1.y & 1) - (p2.y & 1);
    int dy = safeDiff(p1.y, p2.y) * 2;

    if(dx < 0)
        dx = -dx;

    if(dy > size_.y)
    {
        dy = (size_.y * 2) - dy;
    }

    if(dx > size_.x)
    {
        dx = (size_.x * 2) - dx;
    }

    dx -= dy / 2;

    return ((dy + (dx > 0 ? dx : 0)) / 2);
}

ShipDirection MapBase::GetShipDir(MapPoint fromPt, MapPoint toPt) const
{
    // First divide into NORTH/SOUTH by only looking at the y-Difference. On equal we choose SOUTH
    // Then choose between main dir (S/N) or partial E/W:
    //     6 directions -> 60deg covered per direction, mainDir +- 30deg
    //     -> Switching at an angle of 60deg compared to x-axis
    //     hence: |dy/dx| > tan(60deg) -> main dir, else add E or W

    unsigned dy = safeDiff(fromPt.y, toPt.y);
    unsigned dx = safeDiff(fromPt.x, toPt.x);
    // Handle wrapping. Also swap coordinates when wrapping (we reverse the direction)
    if(dy > size_.y / 2u)
    {
        dy = size_.y - dy;
        using std::swap;
        swap(fromPt.y, toPt.y);
    }
    if(dx > size_.x / 2u)
    {
        dx = size_.x - dx;
        using std::swap;
        swap(fromPt.x, toPt.x);
    }
    // tan(60deg) ~= 1.73205080757. Divider at |dy| > |dx| * 1.732 using fixed point math
    // (floating point math may lead to different results among configurations/platforms)
    bool isMainDir = dy * 1000 > dx * 1732;
    if(toPt.y < fromPt.y)
    {
        // North
        if(isMainDir)
            return ShipDirection::NORTH;
        else if(toPt.x < fromPt.x)
            return ShipDirection::NORTHWEST;
        else
            return ShipDirection::NORTHEAST;
    } else
    {
        // South
        if(isMainDir)
            return ShipDirection::SOUTH;
        else if(toPt.x < fromPt.x)
            return ShipDirection::SOUTHWEST;
        else
            return ShipDirection::SOUTHEAST;
    }
}

MapPoint MapBase::MakeMapPoint(Position pt) const
{
    return ::MakeMapPoint(pt, size_);
}
