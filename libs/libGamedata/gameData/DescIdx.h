// Copyright (c) 2017 - 2020 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <cstdint>

/// Type safe index for a description
//-V:DescIdx:801
template<class T>
struct DescIdx
{
    /// Invalid index
    static constexpr uint8_t INVALID = 0xFF;
    uint8_t value;
    explicit constexpr DescIdx(uint8_t value = INVALID) noexcept : value(value) {}
    constexpr bool operator!() const noexcept { return value == INVALID; }
    constexpr bool operator==(DescIdx rhs) const noexcept { return value == rhs.value; }
    constexpr bool operator!=(DescIdx rhs) const noexcept { return value != rhs.value; }
    constexpr bool operator<(DescIdx rhs) const noexcept { return value < rhs.value; }
    constexpr bool operator>(DescIdx rhs) const noexcept { return value > rhs.value; }
    constexpr bool operator<=(DescIdx rhs) const noexcept { return value <= rhs.value; }
    constexpr bool operator>=(DescIdx rhs) const noexcept { return value >= rhs.value; }
};
