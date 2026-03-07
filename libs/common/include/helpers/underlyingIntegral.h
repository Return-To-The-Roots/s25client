// Copyright (C) 2005 - 2026 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "serializeEnums.h"
#include <type_traits>

namespace helpers {

/// Get underlying type, i.e. the "storage" type of T
template<typename T, typename = void>
struct underlyingIntegral;
template<typename T>
struct underlyingIntegral<T, std::void_t<typename T::underlying_t>>
{
    using type = typename T::underlying_t;
};
template<class T>
struct underlyingIntegral<T, std::enable_if_t<std::is_enum_v<T>>>
{
    using type = std::underlying_type_t<T>;
};
template<class T>
struct underlyingIntegral<T, std::enable_if_t<std::is_integral_v<T>>>
{
    using type = T;
};
template<typename T>
using underlyingIntegral_t = typename underlyingIntegral<T>::type;

/// Detect if the type is an integral, or has an underlying integral
template<typename T, typename = void>
struct IsIntegralLike : std::false_type
{};

template<typename T>
struct IsIntegralLike<T, std::void_t<underlyingIntegral_t<T>>> : std::true_type
{};

template<typename T>
constexpr bool IsIntegralLike_v = IsIntegralLike<T>::value;

} // namespace helpers