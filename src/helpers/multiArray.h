// Copyright (c) 2005 - 2015 Settlers Freaks (sf-team at siedler25.org)
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

#include <boost/array.hpp>
#include <boost/static_assert.hpp>

namespace helpers{

    /// Wrapper around boost::array to allow multi-dimensional fixed-size arrays
    /// The declaration 'FooBar myVar[N1][N2][N3]' can be replaced with 'MultiArray<FooBar, N1, N2, N3> myVar'
    /// The given dimensions must be greater than 0
    template<typename T, size_t T_n1, size_t T_n2 = 0, size_t T_n3 = 0, size_t T_n4 = 0, size_t T_n5 = 0>
    struct MultiArray: public boost::array<MultiArray<T, T_n2, T_n3, T_n4, T_n5>, T_n1>
    {
        BOOST_STATIC_ASSERT(T_n1 > 0);
    };

    /* Specialisations for MultiArray
     * A recursive definition is used: T[N1][N2][N3] 
     *          -> MultiArray<T, N1, N2, N3> = boost::array<T[N2][N3], N1>
     *          -> boost::array<MultiArray<T, N2, N3>, N1> = boost::array<boost::array<T[N3], N2>, N1>
     *          -> boost::array<boost::array<MultiArray<T, N3>, N2>, N1> = boost::array<boost::array<boost::array<T, N3>, N2>, N1>
     */
    template<typename T, size_t T_n1>
    struct MultiArray<T, T_n1, 0, 0, 0, 0>: public boost::array<T, T_n1>
    {
        BOOST_STATIC_ASSERT(T_n1 > 0);
    };

    template<typename T, size_t T_n1, size_t T_n2>
    struct MultiArray<T, T_n1, T_n2, 0, 0, 0>: public boost::array<MultiArray<T, T_n2>, T_n1>
    {
        BOOST_STATIC_ASSERT(T_n1 > 0);
    };

    template<typename T, size_t T_n1, size_t T_n2, size_t T_n3>
    struct MultiArray<T, T_n1, T_n2, T_n3, 0, 0>: public boost::array<MultiArray<T, T_n2, T_n3>, T_n1>
    {
        BOOST_STATIC_ASSERT(T_n1 > 0);
    };

    template<typename T, size_t T_n1, size_t T_n2, size_t T_n3, size_t T_n4>
    struct MultiArray<T, T_n1, T_n2, T_n3, T_n4, 0>: public boost::array<MultiArray<T, T_n2, T_n3, T_n4>, T_n1>
    {
        BOOST_STATIC_ASSERT(T_n1 > 0);
    };

}
