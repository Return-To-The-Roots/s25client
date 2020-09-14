// Copyright (c) 2015 - 2019 Settlers Freaks (sf-team at siedler25.org)
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

#include <type_traits>

namespace helpers {
/// Trait to specify the maximum enum value
template<class T_Enum>
struct MaxEnumValue;

/// Return the maximum value of an Enum
/// Needs to have the MaxEnumValue trait specialized, e.g. via DEFINE_MAX_ENUM_VALUE
template<class T_Enum>
constexpr unsigned MaxEnumValue_v = MaxEnumValue<T_Enum>::value; // NOLINT

/// Return the number of enumerators for an enum type
/// Assumes enumerators are contiguous and start at zero, i.e. no manual values are specified
template<class T_Enum>
constexpr unsigned NumEnumValues_v = MaxEnumValue<T_Enum>::value + 1u; // NOLINT

namespace detail {
    template<class T>
    constexpr std::enable_if_t<std::is_enum<T>::value, unsigned> castPotentialEnumToValue(T value)
    {
        return static_cast<unsigned>(value);
    }
    template<class T>
    constexpr std::enable_if_t<std::is_same<T, unsigned>::value, unsigned> castPotentialEnumToValue(T value)
    {
        return value;
    }
} // namespace detail
} // namespace helpers

// Helper macro to specialize the trait
#define DEFINE_MAX_ENUM_VALUE(EnumType, maxValue)                                         \
    namespace helpers {                                                                   \
        template<>                                                                        \
        struct MaxEnumValue<EnumType>                                                     \
        {                                                                                 \
            static constexpr unsigned value = detail::castPotentialEnumToValue(maxValue); \
        };                                                                                \
    } // namespace helpers
