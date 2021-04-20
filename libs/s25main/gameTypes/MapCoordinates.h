// Copyright (c) 2005 - 2017 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

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
