// Copyright (c) 2019 - 2019 Settlers Freaks (sf-team at siedler25.org)
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
    using TargetType = std::conditional_t<typeAtLeastInt, T, std::conditional_t<std::is_signed<T>::value, int, unsigned>>;
    return std::to_string(static_cast<TargetType>(value));
}

template<typename T, std::enable_if_t<std::is_enum<T>::value, int> = 0>
std::string toString(const T value)
{
    return toString(static_cast<std::underlying_type_t<T>>(value));
}

} // namespace helpers
