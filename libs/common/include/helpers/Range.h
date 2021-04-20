// Copyright (c) 2020 - 2020 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <boost/config.hpp>
#include <type_traits>

namespace helpers {
/// Can be used in range-based for loops to iterate over each value in [0, endValue)
/// Requires: T must be an integral
template<typename T>
struct Range
{
    static_assert(std::is_integral<T>::value, "Must be an integral!");
    class iterator
    {
        T value;

    public:
        explicit BOOST_FORCEINLINE constexpr iterator(T value) : value(value) {}
        BOOST_FORCEINLINE constexpr T operator*() const noexcept { return value; }
        BOOST_FORCEINLINE void operator++() noexcept { ++value; }
        BOOST_FORCEINLINE constexpr bool operator!=(iterator rhs) const noexcept { return value != rhs.value; }
    };

    BOOST_FORCEINLINE constexpr iterator begin() const noexcept { return iterator(0); }
    BOOST_FORCEINLINE constexpr iterator end() const noexcept { return iterator(endValue); }

    const T endValue;
};

template<typename T>
constexpr Range<T> range(T endValue)
{
    return {endValue};
}

} // namespace helpers
