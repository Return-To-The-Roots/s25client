// Copyright (c) 2015 - 2017 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <cstdint>

/// "Enum" to represent one of the 6 directions a ship can go
enum class ShipDirection : uint8_t
{
    North,     // 0
    NorthEast, // 1
    SouthEast, // 2
    South,     // 3
    SouthWest, // 4
    NorthWest  // 5
};

constexpr auto maxEnumValue(ShipDirection)
{
    return ShipDirection::NorthWest;
}
