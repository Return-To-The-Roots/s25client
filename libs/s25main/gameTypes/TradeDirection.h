// Copyright (C) 2020 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "RTTR_Assert.h"
#include "helpers/MaxEnumValue.h"
#include "gameTypes/Direction.h"
#include <cstdint>

/// This is either ReachedGoal or convertible to Direction
enum class TradeDirection : uint8_t
{
    West = static_cast<uint8_t>(Direction::West),
    NorthWest = static_cast<uint8_t>(Direction::NorthWest),
    NorthEast = static_cast<uint8_t>(Direction::NorthEast),
    East = static_cast<uint8_t>(Direction::East),
    SouthEast = static_cast<uint8_t>(Direction::SouthEast),
    SouthWest = static_cast<uint8_t>(Direction::SouthWest),
    ReachedGoal = helpers::NumEnumValues_v<Direction>
};

constexpr auto maxEnumValue(TradeDirection)
{
    return TradeDirection::ReachedGoal;
}

constexpr TradeDirection toTradeDirection(const Direction dir) noexcept
{
    return TradeDirection(static_cast<uint8_t>(dir));
}
inline Direction toDirection(const TradeDirection dir) noexcept
{
    RTTR_Assert(dir != TradeDirection::ReachedGoal);
    return Direction(static_cast<uint8_t>(dir));
}
