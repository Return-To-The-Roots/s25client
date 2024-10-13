// Copyright (C) 2005 - 2024 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <cstdint>

/// Type safe index for a description
//-V:DescIdx:801
template<class T>
struct DescIdx
{
    using value_type = uint8_t;
    using index_type = T;

    /// Invalid index
    static constexpr value_type INVALID = 0xFF;

    value_type value = INVALID;

    constexpr DescIdx() noexcept = default;
    explicit constexpr DescIdx(value_type value) noexcept : value(value) {}
    constexpr explicit operator bool() const noexcept { return value != INVALID; }
    constexpr bool operator==(DescIdx rhs) const noexcept { return value == rhs.value; }
    constexpr bool operator!=(DescIdx rhs) const noexcept { return value != rhs.value; }
    constexpr bool operator<(DescIdx rhs) const noexcept { return value < rhs.value; }
    constexpr bool operator>(DescIdx rhs) const noexcept { return value > rhs.value; }
    constexpr bool operator<=(DescIdx rhs) const noexcept { return value <= rhs.value; }
    constexpr bool operator>=(DescIdx rhs) const noexcept { return value >= rhs.value; }
};
