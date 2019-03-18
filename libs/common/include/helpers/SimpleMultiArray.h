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

#ifndef SimpleMultiArray_h__
#define SimpleMultiArray_h__

#include <type_traits>

#ifndef RTTR_Assert_Msg
#define RTTR_Assert_Msg(cond, msg) RTTR_Assert((cond) && (msg))
#endif

namespace helpers {

/// Wrapper around a regular C-Array with multiple dimensions which provides range checking with assertions
/// The declaration 'FooBar myVar[N1][N2][N3]' can be replaced with 'SimpleMultiArray<FooBar, N1, N2, N3> myVar'
/// The given dimensions must be greater than 0
/// This can be initialized with a standard initializer list: ... myVar = { {{i11, i12, i13}, {i21, i22, i23}} }
template<typename T, size_t T_n1, size_t T_n2, size_t T_n3 = 0, size_t T_n4 = 0, size_t T_n5 = 0>
struct SimpleMultiArray;

namespace detail {
    template<typename T, size_t T_n1, size_t T_n2 = 0, size_t T_n3 = 0, size_t T_n4 = 0>
    struct MultiArrayRef
    {
        static constexpr bool is1D = T_n2 == 0;
        using reference = std::conditional_t<is1D, T&, MultiArrayRef<T, T_n2, T_n3, T_n4>>;
        using const_reference = std::add_const_t<reference>;
        static constexpr size_t stride = (T_n2 == 0 ? 1 : T_n2) * (T_n3 == 0 ? 1 : T_n3) * (T_n4 == 0 ? 1 : T_n4);

        T* elems;
        BOOST_FORCEINLINE explicit MultiArrayRef(T* elems) : elems(elems) {}
        BOOST_FORCEINLINE explicit MultiArrayRef(T& elems) : elems(&elems) {}
        size_t size() const { return T_n1; }

        BOOST_FORCEINLINE reference operator[](size_t i)
        {
            RTTR_Assert_Msg(i < T_n1, "out of range");
            return reference(elems[i * stride]);
        }

        BOOST_FORCEINLINE const_reference operator[](size_t i) const
        {
            RTTR_Assert_Msg(i < T_n1, "out of range");
            return const_reference(elems[i * stride]);
        }
    };

} // namespace detail

template<typename T, size_t T_n1, size_t T_n2, size_t T_n3, size_t T_n4, size_t T_n5>
struct SimpleMultiArray
{
    static_assert(T_n1 > 0 && T_n2 > 0 && T_n3 > 0 && T_n4 > 0 && T_n5 > 0, "");
    using reference = detail::MultiArrayRef<T, T_n2, T_n3, T_n4, T_n5>;
    using const_reference = detail::MultiArrayRef<const T, T_n2, T_n3, T_n4, T_n5>;

    T elems[T_n1][T_n2][T_n3][T_n4][T_n5];

    size_t size() const { return T_n1; }

    reference operator[](size_t i)
    {
        RTTR_Assert_Msg(i < T_n1, "out of range");
        return reference(reinterpret_cast<T*>(elems[i]));
    }

    const_reference operator[](size_t i) const
    {
        RTTR_Assert_Msg(i < T_n1, "out of range");
        return const_reference(reinterpret_cast<const T*>(elems[i]));
    }
};

template<typename T, size_t T_n1, size_t T_n2, size_t T_n3, size_t T_n4>
struct SimpleMultiArray<T, T_n1, T_n2, T_n3, T_n4>
{
    static_assert(T_n1 > 0 && T_n2 > 0 && T_n3 > 0 && T_n4 > 0, "");
    using reference = detail::MultiArrayRef<T, T_n2, T_n3, T_n4>;
    using const_reference = detail::MultiArrayRef<const T, T_n2, T_n3, T_n4>;

    T elems[T_n1][T_n2][T_n3][T_n4];

    size_t size() const { return T_n1; }

    reference operator[](size_t i)
    {
        RTTR_Assert_Msg(i < T_n1, "out of range");
        return reference(reinterpret_cast<T*>(elems[i]));
    }

    const_reference operator[](size_t i) const
    {
        RTTR_Assert_Msg(i < T_n1, "out of range");
        return const_reference(reinterpret_cast<const T*>(elems[i]));
    }
};

template<typename T, size_t T_n1, size_t T_n2, size_t T_n3>
struct SimpleMultiArray<T, T_n1, T_n2, T_n3>
{
    static_assert(T_n1 > 0 && T_n2 > 0 && T_n3 > 0, "");
    using reference = detail::MultiArrayRef<T, T_n2, T_n3>;
    using const_reference = detail::MultiArrayRef<const T, T_n2, T_n3>;

    T elems[T_n1][T_n2][T_n3];

    size_t size() const { return T_n1; }

    reference operator[](size_t i)
    {
        RTTR_Assert_Msg(i < T_n1, "out of range");
        return reference(reinterpret_cast<T*>(elems[i]));
    }

    const_reference operator[](size_t i) const
    {
        RTTR_Assert_Msg(i < T_n1, "out of range");
        return const_reference(reinterpret_cast<const T*>(elems[i]));
    }
};

template<typename T, size_t T_n1, size_t T_n2>
struct SimpleMultiArray<T, T_n1, T_n2>
{
    static_assert(T_n1 > 0 && T_n2 > 0, "");
    using reference = detail::MultiArrayRef<T, T_n2>;
    using const_reference = detail::MultiArrayRef<const T, T_n2>;

    T elems[T_n1][T_n2];

    size_t size() const { return T_n1; }

    reference operator[](size_t i)
    {
        RTTR_Assert_Msg(i < T_n1, "out of range");
        return reference(reinterpret_cast<T*>(elems[i]));
    }

    const_reference operator[](size_t i) const
    {
        RTTR_Assert_Msg(i < T_n1, "out of range");
        return const_reference(reinterpret_cast<const T*>(elems[i]));
    }
};

} // namespace helpers

#endif // SimpleMultiArray_h__
