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

#include "EnumTraits.h"
#include <type_traits>

namespace helpers {

/// Trait to query the maximum enum value
/// To opt-in for an enum `EnumT` with contiguous enumerators starting at zero, i.e. no manual values are specified,
/// define a constexpr function `auto maxEnumValue(EnumT)` in the namespace of EnumT that returns the last enumerator
template<class T>
struct MaxEnumValue
{
    static_assert(helpers::is_enum<T>::value, "T must be an enum");
    static constexpr T value = maxEnumValue(T{});
};

/// Return the maximum value of an Enum
template<class T_Enum>
constexpr unsigned MaxEnumValue_v = static_cast<helpers::underlying_type_t<T_Enum>>(MaxEnumValue<T_Enum>::value); // NOLINT

/// Return the number of enumerators for an enum type
template<class T_Enum>
constexpr unsigned NumEnumValues_v = MaxEnumValue_v<T_Enum> + 1u; // NOLINT

} // namespace helpers
