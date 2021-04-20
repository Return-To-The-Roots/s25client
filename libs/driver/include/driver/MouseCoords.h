// Copyright (c) 20052008 Settlers Freaks (sfteam at siedler25.org)
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
    MouseCoords() : pos(0, 0), ldown(false), rdown(false), dbl_click(false) {}
    MouseCoords(int x, int y, bool ldown = false, bool rdown = false, bool dbl_click = false)
        : pos(x, y), ldown(ldown), rdown(rdown), dbl_click(dbl_click)
    {}
    MouseCoords(Position pos, bool ldown = false, bool rdown = false, bool dbl_click = false)
        : pos(pos), ldown(ldown), rdown(rdown), dbl_click(dbl_click)
    {}

    Position pos;
    bool ldown;     /// Linke Maustaste gedrückt
    bool rdown;     /// Rechte Maustaste gedrückt
    bool dbl_click; /// Linke Maustaste - Doppelklick

    Position GetPos() const { return pos; }
};

/// Maximale Zeitdifferenz in ms für einen Doppeklick
const unsigned DOUBLE_CLICK_INTERVAL = 500;
