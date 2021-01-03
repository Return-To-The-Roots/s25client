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
