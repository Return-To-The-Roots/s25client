// Copyright (c) 2015 - 2017 Settlers Freaks (sf-team at siedler25.org)
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

#ifndef multiArray_h__
#define multiArray_h__

#include <array>
#include <type_traits>

namespace helpers {

namespace detail {
    template<typename T, size_t T_n1, size_t... T_n>
    struct GetMultiArrayType
    {
        using type = std::array<typename GetMultiArrayType<T, T_n...>::type, T_n1>;
    };
    template<typename T, size_t T_n>
    struct GetMultiArrayType<T, T_n>
    {
        using type = std::array<T, T_n>;
    };
} // namespace detail

/// Wrapper around std::array to allow multi-dimensional fixed-size arrays
/// The declaration 'FooBar myVar[N1][N2][N3]' can be replaced with 'MultiArray<FooBar, N1, N2, N3> myVar'
template<typename T, size_t... T_n>
using MultiArray = typename detail::GetMultiArrayType<T, T_n...>::type;

} // namespace helpers

#endif // multiArray_h__
