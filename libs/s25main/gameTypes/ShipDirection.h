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

#include "helpers/EnumTraits.h"
#include "helpers/MaxEnumValue.h"

/// "Enum" to represent one of the 6 directions a ship can go
struct ShipDirection
{
    enum Type
    {
        NORTH,     // 0
        NORTHEAST, // 1
        SOUTHEAST, // 2
        SOUTH,     // 3
        SOUTHWEST, // 4
        NORTHWEST  // 5
    };
    static const int COUNT = NORTHWEST + 1;
    using Underlying = std::underlying_type_t<Type>;

    Type t_;
    constexpr ShipDirection(Type t = NORTH) : t_(t) {}
    /// Converts an UInt safely to a Direction
    explicit ShipDirection(unsigned t) : t_(Type(t % COUNT)) {}
    constexpr operator Type() const { return t_; }
    constexpr operator Underlying() const { return t_; }
    ShipDirection operator+(unsigned i) const { return ShipDirection(t_ + i); }
    ShipDirection& operator+=(unsigned i)
    {
        t_ = Type((t_ + i) % COUNT);
        return *this;
    }

private:
    // prevent automatic conversion for any other built-in types such as bool, int, etc
    template<typename T>
    operator T() const;
};

DEFINE_MAX_ENUM_VALUE(ShipDirection, ShipDirection::NORTHWEST)
DEFINE_MAX_ENUM_VALUE(ShipDirection::Type, ShipDirection::NORTHWEST)

namespace helpers {
template<>
struct is_enum<ShipDirection> : std::true_type
{};

template<>
struct EnumRange<ShipDirection> : EnumRange<ShipDirection::Type>
{};
} // namespace helpers
