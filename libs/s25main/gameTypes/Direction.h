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

#include "helpers/MaxEnumValue.h"
#include <cstdint>

/// "Enum" to represent one of the 6 directions from each node
enum class Direction : uint8_t
{
    WEST,      /// 0
    NORTHWEST, /// 1
    NORTHEAST, /// 2
    EAST,      /// 3
    SOUTHEAST, /// 4
    SOUTHWEST  /// 5
};

constexpr auto maxEnumValue(Direction)
{
    return Direction::SOUTHWEST;
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
