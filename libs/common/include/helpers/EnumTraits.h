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

#ifndef EnumTraits_h__
#define EnumTraits_h__

#include <type_traits>

namespace helpers {
// Traits which allow "faking" enums, e.g. struct types with a nested enum that behave like scoped enums

template<class T>
struct is_enum : std::is_enum<T>
{};

/// Return the wrapped/nested enum "Type" for fake enums, else T
template<class T, bool = std::is_enum<T>::value>
struct wrapped_enum;
template<class T>
struct wrapped_enum<T, true>
{
    using type = T;
};
template<class T>
struct wrapped_enum<T, false>
{
    using type = typename T::Type;
    static_assert(std::is_enum<type>::value, "Unexpected subtype 'Type'. Must be an enum!");
};
template<class T>
using wrapped_enum_t = typename wrapped_enum<T>::type;

} // namespace helpers

#endif // EnumTraits_h__