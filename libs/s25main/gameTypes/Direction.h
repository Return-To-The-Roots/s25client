// Copyright (c) 2005 - 2017 Settlers Freaks (sf-team at siedler25.org)
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

#include "helpers/EnumRange.h"
#include "helpers/EnumTraits.h"
#include <cstdint>

/// "Enum" to represent one of the 6 directions from each node
struct Direction
{
    enum Type : uint8_t
    {
        WEST,      /// 0
        NORTHWEST, /// 1
        NORTHEAST, /// 2
        EAST,      /// 3
        SOUTHEAST, /// 4
        SOUTHWEST  /// 5
    };

    Type t_;
    constexpr Direction(Type t = WEST) : t_(t) {}
    /// Convert an UInt safely to a Direction
    explicit Direction(unsigned t) = delete;
    /// Convert an UInt to a Direction without checking its value. Use only when this is actually a Direction
    static Direction fromInt(unsigned t) { return Type(t); }
    /// Use this for use in switches
    constexpr Type native_value() const { return t_; }
    constexpr explicit operator uint8_t() const { return t_; }
};
//-V:Direction:801

namespace helpers {
template<>
struct is_enum<Direction> : std::true_type
{};
} // namespace helpers

constexpr auto maxEnumValue(Direction)
{
    return Direction::SOUTHWEST;
}

/// Convert an UInt safely to a Direction
constexpr Direction convertToDirection(unsigned t)
{
    return static_cast<Direction::Type>(t % helpers::NumEnumValues_v<Direction>);
}

//////////////////////////////////////////////////////////////////////////
// Implementation
//////////////////////////////////////////////////////////////////////////

// Disallow int operators
Direction& operator+=(Direction&, int i) = delete;
Direction& operator-=(Direction&, int i) = delete;

constexpr Direction& operator+=(Direction& dir, unsigned i)
{
    dir = convertToDirection(static_cast<uint8_t>(dir) + i);
    return dir;
}

constexpr Direction& operator-=(Direction& dir, unsigned i)
{
    // Convert to addition
    const unsigned addValue = helpers::NumEnumValues_v<Direction> - (i % helpers::NumEnumValues_v<Direction>);
    const unsigned newDir = static_cast<uint8_t>(dir) + addValue;
    dir = static_cast<Direction::Type>(
      newDir < helpers::NumEnumValues_v<Direction> ? newDir : newDir - helpers::NumEnumValues_v<Direction>);
    return dir;
}

constexpr Direction operator+(Direction dir, unsigned i)
{
    return dir += i;
}

constexpr Direction operator-(Direction dir, unsigned i)
{
    return dir -= i;
}

inline bool operator==(const Direction& lhs, const Direction& rhs)
{
    return lhs.t_ == rhs.t_;
}

inline bool operator!=(const Direction& lhs, const Direction& rhs)
{
    return !(lhs == rhs);
}

// Comparison operators to avoid ambiguity
inline bool operator==(const Direction::Type& lhs, const Direction& rhs)
{
    return lhs == rhs.t_;
}
inline bool operator==(const Direction& lhs, const Direction::Type& rhs)
{
    return lhs.t_ == rhs;
}
inline bool operator!=(const Direction::Type& lhs, const Direction& rhs)
{
    return lhs != rhs.t_;
}
inline bool operator!=(const Direction& lhs, const Direction::Type& rhs)
{
    return lhs.t_ != rhs;
}

namespace helpers {
template<>
struct EnumRange<Direction>
{
    class iterator
    {
        unsigned value;

    public:
        explicit BOOST_FORCEINLINE iterator(unsigned value) : value(value) {}
        BOOST_FORCEINLINE Direction operator*() const { return Direction::fromInt(value); }
        BOOST_FORCEINLINE void operator++() { ++value; }
        BOOST_FORCEINLINE bool operator!=(iterator rhs) const { return value != rhs.value; }
    };

    BOOST_FORCEINLINE iterator begin() const { return iterator(0); }
    BOOST_FORCEINLINE iterator end() const { return iterator(helpers::NumEnumValues_v<Direction>); }
};

template<>
struct EnumRangeWithOffset<Direction>
{
    class iterator
    {
        unsigned value;

    public:
        explicit BOOST_FORCEINLINE iterator(unsigned value) : value(value) {}
        BOOST_FORCEINLINE auto operator*() const { return convertToDirection(value); }
        BOOST_FORCEINLINE void operator++() { ++value; }
        BOOST_FORCEINLINE bool operator!=(iterator rhs) const { return value != rhs.value; }
    };

    explicit BOOST_FORCEINLINE EnumRangeWithOffset(Direction startValue) : startValue_(static_cast<uint8_t>(startValue))
    {}
    unsigned startValue_;

    BOOST_FORCEINLINE iterator begin() const { return iterator(startValue_); }
    BOOST_FORCEINLINE iterator end() const { return iterator(startValue_ + NumEnumValues_v<Direction>); }
};
} // namespace helpers
