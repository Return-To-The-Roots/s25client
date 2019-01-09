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

namespace helpers {

/// Wrapper around std::array to allow multi-dimensional fixed-size arrays
/// The declaration 'FooBar myVar[N1][N2][N3]' can be replaced with 'MultiArray<FooBar, N1, N2, N3> myVar'
/// The given dimensions must be greater than 0
template<typename T, size_t T_n1, size_t T_n2 = 0, size_t T_n3 = 0, size_t T_n4 = 0, size_t T_n5 = 0>
struct MultiArray : public std::array<MultiArray<T, T_n2, T_n3, T_n4, T_n5>, T_n1>
{
    static_assert(T_n1 > 0, "");
};

/* Specialisations for MultiArray
 * A recursive definition is used: T[N1][N2][N3]
 *          -> MultiArray<T, N1, N2, N3> = std::array<T[N2][N3], N1>
 *          -> std::array<MultiArray<T, N2, N3>, N1> = std::array<std::array<T[N3], N2>, N1>
 *          -> std::array<std::array<MultiArray<T, N3>, N2>, N1> = std::array<std::array<std::array<T, N3>, N2>, N1>
 */
template<typename T, size_t T_n1>
struct MultiArray<T, T_n1, 0, 0, 0, 0> : public std::array<T, T_n1>
{
    static_assert(T_n1 > 0, "");
};

template<typename T, size_t T_n1, size_t T_n2>
struct MultiArray<T, T_n1, T_n2, 0, 0, 0> : public std::array<MultiArray<T, T_n2>, T_n1>
{
    static_assert(T_n1 > 0, "");
};

template<typename T, size_t T_n1, size_t T_n2, size_t T_n3>
struct MultiArray<T, T_n1, T_n2, T_n3, 0, 0> : public std::array<MultiArray<T, T_n2, T_n3>, T_n1>
{
    static_assert(T_n1 > 0, "");
};

template<typename T, size_t T_n1, size_t T_n2, size_t T_n3, size_t T_n4>
struct MultiArray<T, T_n1, T_n2, T_n3, T_n4, 0> : public std::array<MultiArray<T, T_n2, T_n3, T_n4>, T_n1>
{
    static_assert(T_n1 > 0, "");
};

} // namespace helpers

#endif // multiArray_h__
