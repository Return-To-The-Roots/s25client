// Copyright (C) 2005 - 2021 Settlers Freaks (sfteam at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "Point.h"

///////////////////////////////////////////////////////////////////////////////
/**
 *  Mausstatusstruct
 *
 *  @author OLiver
 */
class MouseCoords
{
public:
    MouseCoords() : pos(0, 0), ldown(false), rdown(false), dbl_click(false), num_tfingers(0) {}
    MouseCoords(int x, int y, bool ldown = false, bool rdown = false, bool dbl_click = false, unsigned num_tfingers = 0)
        : pos(x, y), ldown(ldown), rdown(rdown), dbl_click(dbl_click), num_tfingers(num_tfingers)
    {}
    MouseCoords(Position pos, bool ldown = false, bool rdown = false, bool dbl_click = false, unsigned num_tfingers = 0)
        : pos(pos), ldown(ldown), rdown(rdown), dbl_click(dbl_click), num_tfingers(num_tfingers)
    {}

    Position pos;
    bool ldown;            // Left mouse button pressed
    bool rdown;            // Right mouse button pressed
    bool dbl_click;        // Left mouse button - doubleclick
    unsigned num_tfingers; // Count of fingers currently on touchscreen

    Position GetPos() const { return pos; }
};

// Max time difference in ms to trigger doubleclick
constexpr unsigned DOUBLE_CLICK_INTERVAL = 500;

// Max time difference in ms to trigger contextclick
constexpr unsigned TOUCH_MAX_CLICK_INTERVAL = 250;
constexpr unsigned TOUCH_DOUBLE_CLICK_INTERVAL = 175;
// Max distance between the two clicks to trigger doubleclick
constexpr unsigned TOUCH_MAX_DOUBLE_CLICK_DISTANCE = 30;