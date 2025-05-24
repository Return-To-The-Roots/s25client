// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "MaxEnumValue.h"
#include <boost/config.hpp>
#include <iterator>
#include <type_traits>

// Using BOOST_FORCEINLINE mostly for increased debug performance (doesn't work for MSVC though)
#define RTTR_CONSTEXPR_INLINE constexpr BOOST_FORCEINLINE

namespace helpers {
template<class T>
struct EnumIterTraits
{
    using iterator_category = std::forward_iterator_tag;
    using value_type = T;
    using difference_type = std::make_signed_t<std::common_type_t<std::underlying_type_t<T>, int>>;
    using pointer = T*;
    using reference = T&;
};

/// Can be used in range-based for loops to iterate over each enum value
/// Requires: T must be an enum with MaxEnumValue trait specialized
///           T must be simple: Start at zero, no gaps
template<class T>
struct EnumRange
{
    static_assert(std::is_enum<T>::value, "Must be an enum!");
    class iterator : public EnumIterTraits<T>
    {
        unsigned value;

    public:
        explicit RTTR_CONSTEXPR_INLINE iterator(unsigned value) : value(value) {}
        RTTR_CONSTEXPR_INLINE T operator*() const { return static_cast<T>(value); }
        RTTR_CONSTEXPR_INLINE void operator++() { ++value; }
        RTTR_CONSTEXPR_INLINE bool operator!=(iterator rhs) const { return value != rhs.value; }
    };

    RTTR_CONSTEXPR_INLINE iterator begin() const { return iterator(0); }
    RTTR_CONSTEXPR_INLINE iterator end() const { return iterator(NumEnumValues_v<T>); }
};

/// Can be used in range-based for loops to iterate over each enum value starting at a given value
/// Requires: T must be an enum with MaxEnumValue trait specialized
///           T must be simple: Start at zero, no gaps
template<class T>
struct EnumRangeWithOffset
{
    static_assert(std::is_enum<T>::value, "Must be an enum!");
    class iterator : public EnumIterTraits<T>
    {
        unsigned value;

    public:
        explicit RTTR_CONSTEXPR_INLINE iterator(unsigned value) : value(value) {}
        RTTR_CONSTEXPR_INLINE T operator*() const
        {
            return static_cast<T>(value > MaxEnumValue_v<T> ? value - MaxEnumValue_v<T> - 1u : value);
        }
        RTTR_CONSTEXPR_INLINE void operator++() { ++value; }
        RTTR_CONSTEXPR_INLINE bool operator!=(iterator rhs) const { return value != rhs.value; }
    };

    explicit RTTR_CONSTEXPR_INLINE EnumRangeWithOffset(T startValue)
        : startValue_(static_cast<std::underlying_type_t<T>>(startValue))
    {}
    unsigned startValue_;

    RTTR_CONSTEXPR_INLINE iterator begin() const { return iterator(startValue_); }
    RTTR_CONSTEXPR_INLINE iterator end() const { return iterator(startValue_ + NumEnumValues_v<T>); }
};

template<typename T>
RTTR_CONSTEXPR_INLINE auto enumRange()
{
    return EnumRange<T>{};
}
template<typename T>
RTTR_CONSTEXPR_INLINE auto enumRange(T startValue)
{
    return EnumRangeWithOffset<T>(startValue);
}

#undef RTTR_CONSTEXPR_INLINE

} // namespace helpers
