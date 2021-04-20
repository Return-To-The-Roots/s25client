// Copyright (c) 2020 - 2020 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "MaxEnumValue.h"
#include <boost/none.hpp>
#include <boost/optional/bad_optional_access.hpp>
#include <limits>
#include <type_traits>

namespace helpers {
/// Similar to std::optional but optimized for enums to allow for more speed and less storage by storing the underlying
/// value directly
template<class T>
class OptionalEnum
{
    static_assert(std::is_enum<T>::value, "Only works for enums");
    using underlying_type = std::underlying_type_t<T>;

public:
    static constexpr underlying_type invalidValue = std::numeric_limits<underlying_type>::max();
    static_assert(static_cast<unsigned>(invalidValue) > MaxEnumValue_v<T>,
                  "Enum has to many values to be used. Use a larger underlying type!");
    using value_type = T;

    constexpr OptionalEnum() noexcept = default;
    OptionalEnum(boost::none_t) noexcept {}
    constexpr OptionalEnum(const T& value) noexcept : value_(static_cast<underlying_type>(value)) {}
    constexpr OptionalEnum& operator=(const T& value) noexcept
    {
        value_ = static_cast<underlying_type>(value);
        return *this;
    }
    // Copy and conversion not required, copy/move are default and conversion between enums is not possible

    constexpr T operator*() const noexcept { return static_cast<T>(value_); }
    constexpr explicit operator bool() const noexcept { return has_value(); }
    constexpr bool has_value() const noexcept { return value_ != invalidValue; }
    constexpr T value() const
    {
        if(!has_value())
            throw boost::bad_optional_access();
        return **this;
    }
    constexpr T value_or(T default_value) const { return bool(*this) ? **this : default_value; }

    constexpr void reset() noexcept { value_ = invalidValue; }

    // Those comparisons can be simplified to comparing value_ due to the invalid value being a distinct value
    friend constexpr bool operator==(OptionalEnum lhs, OptionalEnum rhs) { return lhs.value_ == rhs.value_; }
    friend constexpr bool operator!=(OptionalEnum lhs, OptionalEnum rhs) { return lhs.value_ != rhs.value_; }
    // Technically not required as above operators would be used, but included for better debug performance and
    // mirroring the standard
    friend constexpr bool operator==(OptionalEnum lhs, const T& rhs)
    {
        return lhs.value_ == static_cast<underlying_type>(rhs);
    }
    friend constexpr bool operator!=(OptionalEnum lhs, const T& rhs)
    {
        return lhs.value_ != static_cast<underlying_type>(rhs);
    }
    friend constexpr bool operator==(const T& lhs, OptionalEnum rhs)
    {
        return static_cast<underlying_type>(lhs) == rhs.value_;
    }
    friend constexpr bool operator!=(const T& lhs, OptionalEnum rhs)
    {
        return static_cast<underlying_type>(lhs) != rhs.value_;
    }

private:
    underlying_type value_ = invalidValue;
};
} // namespace helpers
