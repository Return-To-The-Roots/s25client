// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <string>
#include <type_traits>

namespace helpers {

/// Wrapper around std::to_string handling correct casts for small ints and enums
template<typename T, std::enable_if_t<std::is_floating_point<T>::value, int> = 0>
std::string toString(const T value)
{
    return std::to_string(value);
}

template<typename T, std::enable_if_t<std::is_integral<T>::value, int> = 0>
std::string toString(const T value)
{
    constexpr bool typeAtLeastInt = sizeof(T) >= sizeof(int);
    using TargetType =
      std::conditional_t<typeAtLeastInt, T, std::conditional_t<std::is_signed<T>::value, int, unsigned>>;
    return std::to_string(static_cast<TargetType>(value));
}

template<typename T, std::enable_if_t<std::is_enum<T>::value, int> = 0>
std::string toString(const T value)
{
    return toString(static_cast<std::underlying_type_t<T>>(value));
}

} // namespace helpers
