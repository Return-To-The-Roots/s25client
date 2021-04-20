// Copyright (c) 2005 - 2017 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

/// Window size or resolution
struct VideoMode
{
    unsigned short width;
    unsigned short height;

    VideoMode() : width(0), height(0) {}
    VideoMode(unsigned short width, unsigned short height) : width(width), height(height) {}
    bool operator==(const VideoMode& o) const { return (width == o.width && height == o.height); }
    bool operator!=(const VideoMode& o) const { return !(*this == o); }
};
