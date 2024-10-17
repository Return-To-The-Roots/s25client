// Copyright (C) 2005 - 2024 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "RTTR_Assert.h"
#include <boost/config.hpp>
#include <iterator>
#include <type_traits>

namespace helpers {
/// Similar to Pythons `range()` function can can be used in range-based for loops:
/// Yield each value in [startValue, endValue) or [0, endValue) if startValue is not given
/// Requires: T must be an integral
template<typename T>
class range
{
    static_assert(std::is_integral<T>::value, "Must be an integral!");
    const T startValue_;
    const T endValue_;

public:
    explicit range(T endValue) : range(0, endValue) {}
    explicit range(T startValue, T endValue) : startValue_(startValue), endValue_(endValue)
    {
        RTTR_Assert(startValue <= endValue);
    }

    class iterator
    {
        T value;

    public:
        using iterator_category = std::forward_iterator_tag;
        using value_type = T;
        using difference_type = std::make_signed_t<std::common_type_t<T, int>>;
        using pointer = T*;
        using reference = T&;

        explicit BOOST_FORCEINLINE constexpr iterator(T value) : value(value) {}
        BOOST_FORCEINLINE constexpr T operator*() const noexcept { return value; }
        BOOST_FORCEINLINE void operator++() noexcept { ++value; }
        BOOST_FORCEINLINE constexpr bool operator!=(iterator rhs) const noexcept { return value != rhs.value; }
    };

    BOOST_FORCEINLINE constexpr iterator begin() const noexcept { return iterator(startValue_); }
    BOOST_FORCEINLINE constexpr iterator end() const noexcept { return iterator(endValue_); }
};

} // namespace helpers
