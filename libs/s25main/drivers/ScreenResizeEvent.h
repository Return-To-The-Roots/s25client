// Copyright (c) 2005 - 2017 Settlers Freaks (sfteam at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "Point.h"

/// ScreenResize-Event
struct ScreenResizeEvent
{
    ScreenResizeEvent(const Extent& oldSize, const Extent& newSize) : oldSize(oldSize), newSize(newSize) {}
    Extent oldSize, newSize;
};
