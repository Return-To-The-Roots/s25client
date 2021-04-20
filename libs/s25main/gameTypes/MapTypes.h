// Copyright (c) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "FlagType.h"
#include <cstdint>

/// Visibility of a node
enum class Visibility : uint8_t
{
    Invisible,
    FogOfWar,
    Visible
};
constexpr auto maxEnumValue(Visibility)
{
    return Visibility::Visible;
}

/// Direction from a point where a road can go. Opposites are stored in neighbors
enum class RoadDir
{
    East,
    SouthEast,
    SouthWest
};
constexpr auto maxEnumValue(RoadDir)
{
    return RoadDir::SouthWest;
}

/// Type of the road "owned" by a point
enum class PointRoad : unsigned char
{
    None,
    Normal,
    Donkey,
    Boat
};
constexpr auto maxEnumValue(PointRoad)
{
    return PointRoad::Boat;
}
