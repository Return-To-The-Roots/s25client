// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "helpers/MaxEnumValue.h"
#include <cstdint>

/// "Enum" to represent one of the 6 directions from each node
enum class Direction : uint8_t
{
    West,      /// 0
    NorthWest, /// 1
    NorthEast, /// 2
    East,      /// 3
    SouthEast, /// 4
    SouthWest  /// 5
};

constexpr auto maxEnumValue(Direction)
{
    return Direction::SouthWest;
}

/// Convert an UInt safely to a Direction
constexpr Direction convertToDirection(unsigned t)
{
    return static_cast<Direction>(t % helpers::NumEnumValues_v<Direction>);
}

//////////////////////////////////////////////////////////////////////////
// Implementation
//////////////////////////////////////////////////////////////////////////

// Disallow int operators
Direction& operator+=(Direction&, int i) = delete;
Direction& operator-=(Direction&, int i) = delete;

constexpr Direction& operator+=(Direction& dir, unsigned i)
{
    dir = convertToDirection(static_cast<uint8_t>(dir) + i);
    return dir;
}

constexpr Direction& operator-=(Direction& dir, unsigned i)
{
    // Convert to addition
    const unsigned addValue = helpers::NumEnumValues_v<Direction> - (i % helpers::NumEnumValues_v<Direction>);
    const unsigned newDir = static_cast<uint8_t>(dir) + addValue;
    dir = static_cast<Direction>(
      newDir < helpers::NumEnumValues_v<Direction> ? newDir : newDir - helpers::NumEnumValues_v<Direction>);
    return dir;
}

constexpr Direction operator+(Direction dir, unsigned i)
{
    return dir += i;
}

constexpr Direction operator-(Direction dir, unsigned i)
{
    return dir -= i;
}
