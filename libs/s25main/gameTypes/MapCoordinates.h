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

#pragma once

#ifndef MapCoordinates_h__
#define MapCoordinates_h__

#include "Point.h"

/// Data type for map coordinates (unsigned, as we can't have negative map coordinates)
using MapCoord = unsigned short;
/// Point on map
using MapPoint = Point<MapCoord>;
/// Extent/Size of maps
using MapExtent = Point<MapCoord>;

/// Ordering operator for MapPoints: Sort by y, then x with less-than
struct MapPointLess
{
    constexpr bool operator()(const MapPoint& lhs, const MapPoint& rhs) const
    {
        return (lhs.y < rhs.y) || ((lhs.y == rhs.y) && (lhs.x < rhs.x));
    }
};

// Surpress warnings for pass by value of those (small) types
//-V:MapPoint:801
//-V:MapPoint:813
//-V:MapExtent:801
//-V:MapExtent:813

#endif // MapCoordinates_h__
