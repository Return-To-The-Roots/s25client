// Copyright (C) 2005 - 2025 Settlers Freaks (sfteam at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "Point.h"

/// State of mouse buttons and position
struct MouseCoords
{
    MouseCoords() = default;
    MouseCoords(Position pos) : pos(pos) {}
    MouseCoords(int x, int y) : pos(x, y) {}

    Position pos = Position(0, 0);
    bool ldown = false;        /// left button down
    bool rdown = false;        /// right button down
    bool dbl_click = false;    /// double-click (left button)
    unsigned num_tfingers = 0; /// Count of fingers currently on touchscreen
};

/// Maximum interval between two clicks to be considered a double-click (in milliseconds)
constexpr unsigned DOUBLE_CLICK_INTERVAL = 500;
