// Copyright (C) 2005 - 2025 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <type_traits>

namespace detail {
template<class T>
struct GetFontStyleMask;
template<class T>
constexpr unsigned GetFontStyleMask_v = GetFontStyleMask<T>::value;
} // namespace detail

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
    template<class T_Enum, typename = std::enable_if_t<std::is_enum_v<T_Enum>>>
    constexpr FontStyle(T_Enum style) : value(style)
    {}

    template<class T_Enum, typename = std::enable_if_t<std::is_enum_v<T_Enum>>>
    constexpr FontStyle operator|(T_Enum style) const
    {
        return (value & ~detail::GetFontStyleMask_v<T_Enum>) | style;
    }

    template<class T_Enum, typename = std::enable_if_t<std::is_enum_v<T_Enum>>>
    constexpr bool is(T_Enum style) const
    {
        return (value & detail::GetFontStyleMask_v<T_Enum>) == style;
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
