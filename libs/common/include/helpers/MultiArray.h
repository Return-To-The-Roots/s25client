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

#pragma once

#include "RTTR_Assert.h"
#include <boost/config.hpp>
#include <array>
#include <cstddef>
#include <type_traits>

namespace helpers {

/// Wrapper around a regular C-Array with multiple dimensions which provides range checking with assertions
/// The declaration 'FooBar myVar[N1][N2][N3]' can be replaced with 'MultiArray<FooBar, N1, N2, N3> myVar'
/// The given dimensions must be greater than 0
/// This can be initialized with a standard initializer list: ... myVar = { {{i11, i12, i13}, {i21, i22, i23}} }
template<typename T, size_t... T_n>
struct MultiArray;

namespace detail {
    // TODO: Use C++14 constexpr function when dropping MSVC2015
    template<size_t... sizes>
    struct Product;

    template<size_t size, size_t... sizes>
    struct Product<size, sizes...>
    {
        static constexpr size_t value = size * Product<sizes...>::value;
    };

    template<>
    struct Product<>
    {
        static constexpr size_t value = 1;
    };

    template<typename T, size_t... T_ns>
    struct MultiArrayRef;

    template<typename T, size_t T_n, size_t... T_ns>
    struct MultiArrayRef<T, T_n, T_ns...>
    {
        static constexpr bool is1D = sizeof...(T_ns) == 0;
        using reference = std::conditional_t<is1D, T&, MultiArrayRef<T, T_ns...>>;
        using const_reference = std::add_const_t<reference>;
        static constexpr size_t stride = Product<T_ns...>::value;

        T* elems;
        constexpr BOOST_FORCEINLINE explicit MultiArrayRef(T* elems) : elems(elems) {}
        constexpr BOOST_FORCEINLINE explicit MultiArrayRef(T& elems) : elems(&elems) {}
        constexpr size_t size() const { return T_n; }

        BOOST_FORCEINLINE reference operator[](size_t i)
        {
            RTTR_Assert_Msg(i < T_n, "out of range");
            return reference(elems[i * stride]);
        }

        BOOST_FORCEINLINE const_reference operator[](size_t i) const
        {
            RTTR_Assert_Msg(i < T_n, "out of range");
            return const_reference(elems[i * stride]);
        }
    };
    template<typename T, size_t... T_n>
    struct AddExtents;
    template<typename T, size_t T_n, size_t... T_ns>
    struct AddExtents<T, T_n, T_ns...>
    {
        using type = typename AddExtents<T, T_ns...>::type[T_n];
    };
    template<typename T, size_t T_n>
    struct AddExtents<T, T_n>
    {
        using type = T[T_n];
    };
    template<typename T, size_t... T_ns>
    using AddExtents_t = typename AddExtents<T, T_ns...>::type;

    template<size_t numDims>
    size_t getFlatIndex(const std::array<size_t, numDims>& idxs, const std::array<size_t, numDims>& shape)
    {
        static_assert(numDims > 0, "");
        RTTR_Assert_Msg(idxs < shape, "out of range");
        size_t result = idxs[0];
        for(size_t i = 1; i < numDims; i++)
        {
            result *= shape[i];
            result += idxs[i];
        }
        return result;
    }
} // namespace detail

template<typename T, size_t T_n, size_t... T_ns>
struct MultiArray<T, T_n, T_ns...>
{
    using array_type = detail::AddExtents_t<T, T_n, T_ns...>;
    using reference = detail::MultiArrayRef<T, T_ns...>;
    using const_reference = detail::MultiArrayRef<const T, T_ns...>;

    array_type elems;

    static constexpr size_t numDims = sizeof...(T_ns) + 1u;
    static_assert(numDims > 1, "Use std::array for 1D arrays");
    static constexpr std::array<size_t, numDims> shape() { return {T_n, T_ns...}; }
    static constexpr size_t size() { return shape()[0]; }
    static constexpr size_t numElements() { return detail::Product<T_n, T_ns...>::value; }

    T* data() { return reinterpret_cast<T*>(elems); }
    constexpr const T* data() const { return reinterpret_cast<const T*>(elems); }

    T* begin() { return data(); }
    constexpr const T* begin() const { return data(); }
    T* end() { return data() + size(); }
    constexpr const T* end() const { return data() + size(); }

    template<typename... I>
    std::enable_if_t<sizeof...(I) == numDims, T&> operator()(I... i)
    {
        return data()[detail::getFlatIndex(std::array<size_t, numDims>{size_t(i)...}, shape())];
    }

    template<typename... I>
    std::enable_if_t<sizeof...(I) == numDims, const T&> operator()(I... i) const
    {
        return data()[detail::getFlatIndex(std::array<size_t, numDims>{size_t(i)...}, shape())];
    }

    reference operator[](size_t i)
    {
        RTTR_Assert_Msg(i < T_n, "out of range");
        return reference(reinterpret_cast<T*>(elems[i]));
    }

    const_reference operator[](size_t i) const
    {
        RTTR_Assert_Msg(i < T_n, "out of range");
        return const_reference(reinterpret_cast<const T*>(elems[i]));
    }
};

} // namespace helpers
