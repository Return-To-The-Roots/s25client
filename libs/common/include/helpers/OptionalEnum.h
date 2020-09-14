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

#pragma once

#include "EnumTraits.h"
#include "MaxEnumValue.h"
#include <boost/none.hpp>
#include <boost/optional/bad_optional_access.hpp>
#include <limits>
#include <type_traits>

namespace helpers {
/// Similar to std::optional but optimized for enums to allow for more speed and less storage by storing the underlying value directly
template<class T>
class OptionalEnum
{
    static_assert(is_enum<T>::value, "Only works for enums");
    using RealEnum = wrapped_enum_t<T>;
    using underlying_type = underlying_type_t<RealEnum>;
    template<typename U>
    using RequiresIsTOrWrapped = std::enable_if_t<std::is_same<U, T>::value || std::is_same<U, RealEnum>::value>;

public:
    static constexpr underlying_type invalidValue = std::numeric_limits<underlying_type>::max();
    static_assert(static_cast<unsigned>(invalidValue) > MaxEnumValue_v<T>,
                  "Enum has to many values to be used. Use a larger underlying type!");
    using value_type = T;

    constexpr OptionalEnum() noexcept = default;
    OptionalEnum(boost::none_t) noexcept {}
    template<typename U, class = RequiresIsTOrWrapped<U>>
    constexpr OptionalEnum(const U& value) noexcept : value_(static_cast<underlying_type>(value))
    {}
    template<typename U, class = RequiresIsTOrWrapped<U>>
    OptionalEnum& operator=(const U& value) noexcept
    {
        value_ = static_cast<underlying_type>(value);
        return *this;
    }
    // Copy and conversion not required, copy/move are default and conversion between enums is not possible

    constexpr T operator*() const noexcept { return static_cast<RealEnum>(value_); }
    constexpr explicit operator bool() const noexcept { return has_value(); }
    constexpr bool has_value() const noexcept { return value_ != invalidValue; }
    T value() const
    {
        if(!has_value())
            throw boost::bad_optional_access();
        return **this;
    }
    constexpr T value_or(T default_value) const { return bool(*this) ? **this : default_value; }

    void reset() noexcept { value_ = invalidValue; }

    // Those comparisons can be simplified to comparing value_ due to the invalid value being a distinct value
    friend constexpr bool operator==(OptionalEnum lhs, OptionalEnum rhs) { return lhs.value_ == rhs.value_; }
    friend constexpr bool operator!=(OptionalEnum lhs, OptionalEnum rhs) { return lhs.value_ != rhs.value_; }
    // Technically not required as above operators would be used, but included for better debug performance and mirroring the standard
    template<typename U, class = RequiresIsTOrWrapped<U>>
    friend constexpr bool operator==(OptionalEnum lhs, const U& rhs)
    {
        return lhs.value_ == static_cast<underlying_type>(rhs);
    }
    template<typename U, class = RequiresIsTOrWrapped<U>>
    friend constexpr bool operator!=(OptionalEnum lhs, const U& rhs)
    {
        return lhs.value_ != static_cast<underlying_type>(rhs);
    }
    template<typename U, class = RequiresIsTOrWrapped<U>>
    friend constexpr bool operator==(const U& lhs, OptionalEnum rhs)
    {
        return static_cast<underlying_type>(lhs) == rhs.value_;
    }
    template<typename U, class = RequiresIsTOrWrapped<U>>
    friend constexpr bool operator!=(const U& lhs, OptionalEnum rhs)
    {
        return static_cast<underlying_type>(lhs) != rhs.value_;
    }

private:
    underlying_type value_ = invalidValue;
};
} // namespace helpers
