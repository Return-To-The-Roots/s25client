// Copyright (c) 2020 - 2020 Settlers Freaks (sf-team at siedler25.org)
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

#include "RTTR_Assert.h"
#include "gameTypes/Direction.h"
#include <cstdint>

/// This is either Ship, None or convertible to Direction
enum class RoadPathDirection : uint8_t
{
    West = Direction::WEST,
    NorthWest = Direction::NORTHWEST,
    NorthEast = Direction::NORTHEAST,
    East = Direction::EAST,
    SouthEast = Direction::SOUTHEAST,
    SouthWest = Direction::SOUTHWEST,
    Ship = helpers::NumEnumValues_v<Direction>,
    None
};
constexpr auto maxEnumValue(RoadPathDirection)
{
    return RoadPathDirection::None;
}

inline RoadPathDirection toRoadPathDirection(const Direction dir) noexcept
{
    return RoadPathDirection(static_cast<uint8_t>(dir));
}
inline Direction toDirection(const RoadPathDirection dir) noexcept
{
    RTTR_Assert(static_cast<uint8_t>(dir) <= helpers::MaxEnumValue_v<Direction>);
    return Direction::fromInt(static_cast<uint8_t>(dir));
}
