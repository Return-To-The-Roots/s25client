// Copyright (C) 2005 - 2025 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <type_traits>

namespace helpers {

/// Trait to query the maximum enum value
/// To opt-in for an enum `EnumT` with contiguous enumerators starting at zero, i.e. no manual values are specified,
/// define a constexpr function `auto maxEnumValue(EnumT)` in the namespace of EnumT that returns the last enumerator
template<class T>
struct MaxEnumValue
{
    static_assert(std::is_enum_v<T>, "T must be an enum");
    static constexpr T value = maxEnumValue(T{});
};

/// Return the highest enumerator of an enum type
template<class T_Enum>
inline constexpr T_Enum MaxEnumerator_v = MaxEnumValue<T_Enum>::value;

/// Return the maximum numeric value of an Enum
template<class T_Enum>
inline constexpr unsigned MaxEnumValue_v = static_cast<std::underlying_type_t<T_Enum>>(MaxEnumerator_v<T_Enum>);

/// Return the number of enumerators for an enum type
template<class T_Enum>
inline constexpr unsigned NumEnumValues_v = MaxEnumValue_v<T_Enum> + 1u;

} // namespace helpers
