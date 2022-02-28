// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

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
    switch(dir)
    {
        case Direction::West: // -1|0   -1|0
            res.x = ((pt.x == 0) ? size_.x : pt.x) - 1;
            res.y = pt.y;
            break;
        case Direction::NorthWest: // -1|-1   0|-1
            res.x = (pt.y & 1) ? pt.x : (((pt.x == 0) ? size_.x : pt.x) - 1);
            res.y = ((pt.y == 0) ? size_.y : pt.y) - 1;
            break;
        case Direction::NorthEast: // 0|-1  -1|-1
            res.x = (!(pt.y & 1)) ? pt.x : ((pt.x == size_.x - 1) ? 0 : pt.x + 1);
            res.y = ((pt.y == 0) ? size_.y : pt.y) - 1;
            break;
        case Direction::East: // 1|0    1|0
            res.x = pt.x + 1;
            if(res.x == size_.x)
                res.x = 0;
            res.y = pt.y;
            break;
        case Direction::SouthEast: // 1|1    0|1
            res.x = (!(pt.y & 1)) ? pt.x : ((pt.x == size_.x - 1) ? 0 : pt.x + 1);
            res.y = pt.y + 1;
            if(res.y == size_.y)
                res.y = 0;
            break;
        default:
            RTTR_Assert(dir == Direction::SouthWest);                         // 0|1   -1|1
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

helpers::EnumArray<MapPoint, Direction> MapBase::GetNeighbours(const MapPoint pt) const
{
    const MapCoord yplus1 = pt.y == size_.y - 1 ? 0 : pt.y + 1;
    const MapCoord yminus1 = (pt.y == 0 ? size_.y : pt.y) - 1;
    const MapCoord xplus1 = pt.x == size_.x - 1 ? 0 : pt.x + 1;
    const MapCoord xminus1 = (pt.x == 0 ? size_.x : pt.x) - 1;
    const bool isEvenRow = (pt.y & 1) == 0;

    return {MapPoint(xminus1, pt.y),
            MapPoint(!isEvenRow ? pt.x : xminus1, yminus1),
            MapPoint(isEvenRow ? pt.x : xplus1, yminus1),
            MapPoint(xplus1, pt.y),
            MapPoint(isEvenRow ? pt.x : xplus1, yplus1),
            MapPoint(!isEvenRow ? pt.x : xminus1, yplus1)};
}

unsigned MapBase::CalcDistance(const Position& p1, const Position& p2) const
{
    int dx = ((p1.x - p2.x) * 2) + (p1.y & 1) - (p2.y & 1);
    int dy = absDiff(p1.y, p2.y) * 2;

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

unsigned MapBase::CalcMaxDistance() const
{
    return CalcDistance(MapPoint::all(0), MapPoint(size_.x / 2, size_.y / 2));
}

ShipDirection MapBase::GetShipDir(MapPoint fromPt, MapPoint toPt) const
{
    // First divide into North/South by only looking at the y-Difference. On equal we choose South
    // Then choose between main dir (S/N) or partial E/W:
    //     6 directions -> 60deg covered per direction, mainDir +- 30deg
    //     -> Switching at an angle of 60deg compared to x-axis
    //     hence: |dy/dx| > tan(60deg) -> main dir, else add E or W

    unsigned dy = absDiff(fromPt.y, toPt.y);
    unsigned dx = absDiff(fromPt.x, toPt.x);
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
            return ShipDirection::North;
        else if(toPt.x < fromPt.x)
            return ShipDirection::NorthWest;
        else
            return ShipDirection::NorthEast;
    } else
    {
        // South
        if(isMainDir)
            return ShipDirection::South;
        else if(toPt.x < fromPt.x)
            return ShipDirection::SouthWest;
        else
            return ShipDirection::SouthEast;
    }
}

MapPoint MapBase::MakeMapPoint(Position pt) const
{
    return ::MakeMapPoint(pt, size_);
}

unsigned MapBase::numPointsRadius(unsigned radius, bool includePt) const {
    // For every additional radius we get 6 * curRadius more points. Hence we have 6 * sum(1..radius) points + the
    // center point if requested This can be reduced via the gauss formula to the following:
    return (radius * radius + radius) * 3u + (includePt ? 1u : 0u);
}
