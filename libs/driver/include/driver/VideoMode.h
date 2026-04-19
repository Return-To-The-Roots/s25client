// Copyright (C) 2005 - 2026 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

/// Window size or resolution
struct VideoMode
{
    unsigned short width;
    unsigned short height;

    constexpr VideoMode() : width(0), height(0) {}
    constexpr VideoMode(unsigned short width, unsigned short height) : width(width), height(height) {}
    constexpr bool operator==(const VideoMode& o) const { return (width == o.width && height == o.height); }
    constexpr bool operator!=(const VideoMode& o) const { return !(*this == o); }
    constexpr bool operator<(const VideoMode& o) const
    {
        const auto area = static_cast<unsigned>(width) * height;
        const auto otherArea = static_cast<unsigned>(o.width) * o.height;
        if(area != otherArea)
            return area < otherArea;
        if(width != o.width)
            return width < o.width;
        return height < o.height;
    }
};

// Enum like type with extra flag
struct DisplayMode
{
    enum Type
    {
        Windowed,
        Fullscreen,
        BorderlessWindow,
    } type = Windowed;
    bool resizeable = true;

    constexpr DisplayMode() = default;
    constexpr DisplayMode(Type t) : type(t) {}
    constexpr explicit DisplayMode(unsigned t) : type(Type(t)) {}
    constexpr bool operator==(const Type& t) const { return type == t; }
    constexpr bool operator!=(const Type& t) const { return type != t; }
    constexpr bool operator==(const DisplayMode& o) const { return o.type == type && o.resizeable == resizeable; }
    constexpr bool operator!=(const DisplayMode& o) const { return !(o == *this); }
};

constexpr auto maxEnumValue(DisplayMode::Type)
{
    return DisplayMode::BorderlessWindow;
}
