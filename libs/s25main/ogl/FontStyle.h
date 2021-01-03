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

#include <type_traits>

namespace detail {
template<class T>
struct GetFontStyleMask;
}

class FontStyle
{
public:
    /// Horizontal align
    enum AlignH : unsigned
    {
        LEFT = 0,
        RIGHT = 1 << 0,
        CENTER = 1 << 1
    };

    /// Vertical align
    enum AlignV : unsigned
    {
        TOP = 0,
        BOTTOM = 1 << 2,
        VCENTER = 1 << 3
    };

    /// Additional style
    enum Additional : unsigned
    {
        OUTLINE = 0,
        NO_OUTLINE = 1 << 4
    };

    constexpr FontStyle() = default;
    template<class T_Enum, typename = std::enable_if_t<std::is_enum<T_Enum>::value>>
    constexpr FontStyle(T_Enum style) : value(style)
    {}

    template<class T_Enum, typename = std::enable_if_t<std::is_enum<T_Enum>::value>>
    constexpr FontStyle operator|(T_Enum style) const
    {
        return (value & ~detail::GetFontStyleMask<T_Enum>::value) | style;
    }

    template<class T_Enum, typename = std::enable_if_t<std::is_enum<T_Enum>::value>>
    constexpr bool is(T_Enum style) const
    {
        return (value & detail::GetFontStyleMask<T_Enum>::value) == style;
    }

private:
    constexpr FontStyle(unsigned style) : value(style) {}
    unsigned value = 0;
};

namespace detail {
template<>
struct GetFontStyleMask<FontStyle::AlignH>
{
    static constexpr unsigned value = 3;
};
template<>
struct GetFontStyleMask<FontStyle::AlignV>
{
    static constexpr unsigned value = 12;
};
template<>
struct GetFontStyleMask<FontStyle::Additional>
{
    static constexpr unsigned value = 16;
};
} // namespace detail

constexpr FontStyle operator|(FontStyle::AlignH lhs, FontStyle::AlignV rhs)
{
    return FontStyle(lhs) | rhs;
}

constexpr FontStyle operator|(FontStyle::AlignH lhs, FontStyle::Additional rhs)
{
    return FontStyle(lhs) | rhs;
}

constexpr FontStyle operator|(FontStyle::AlignV lhs, FontStyle::Additional rhs)
{
    return FontStyle(lhs) | rhs;
}

// Enforce order
unsigned operator|(FontStyle::AlignV, FontStyle::AlignH) = delete;
unsigned operator|(FontStyle::Additional, FontStyle::AlignH) = delete;
unsigned operator|(FontStyle::Additional, FontStyle::AlignV) = delete;
