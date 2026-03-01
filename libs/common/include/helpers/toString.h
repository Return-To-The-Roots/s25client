// Copyright (C) 2005 - 2026 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "underlyingIntegral.h"
#include <string>
#include <type_traits>

namespace helpers {

/// Wrapper around std::to_string handling correct casts for small ints and enums
template<typename T, std::enable_if_t<std::is_floating_point_v<T>, int> = 0>
std::string toString(const T value)
{
    return std::to_string(value);
}

template<typename T, std::enable_if_t<IsIntegralLike_v<T>, int> = 0>
std::string toString(const T value)
{
    using U = underlyingIntegral_t<T>;
    constexpr bool typeAtLeastInt = sizeof(U) >= sizeof(int);
    using TargetType = std::conditional_t<typeAtLeastInt, U, std::conditional_t<std::is_signed_v<U>, int, unsigned>>;
    return std::to_string(static_cast<TargetType>(static_cast<U>(value)));
}

} // namespace helpers
