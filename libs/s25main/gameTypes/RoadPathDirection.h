// Copyright (C) 2020 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "RTTR_Assert.h"
#include "gameTypes/Direction.h"
#include <cstdint>

/// This is either Ship, None or convertible to Direction
enum class RoadPathDirection : uint8_t
{
    West = static_cast<uint8_t>(Direction::West),
    NorthWest = static_cast<uint8_t>(Direction::NorthWest),
    NorthEast = static_cast<uint8_t>(Direction::NorthEast),
    East = static_cast<uint8_t>(Direction::East),
    SouthEast = static_cast<uint8_t>(Direction::SouthEast),
    SouthWest = static_cast<uint8_t>(Direction::SouthWest),
    Ship = helpers::NumEnumValues_v<Direction>,
    None
};
constexpr auto maxEnumValue(RoadPathDirection)
{
    return RoadPathDirection::None;
}

constexpr RoadPathDirection toRoadPathDirection(const Direction dir) noexcept
{
    return RoadPathDirection(static_cast<uint8_t>(dir));
}
inline Direction toDirection(const RoadPathDirection dir) noexcept
{
    RTTR_Assert(static_cast<uint8_t>(dir) <= helpers::MaxEnumValue_v<Direction>);
    return Direction(static_cast<uint8_t>(dir));
}
