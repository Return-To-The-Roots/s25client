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
    bool ldown;            /// Linke Maustaste gedrückt
    bool rdown;            /// Rechte Maustaste gedrückt
    bool dbl_click;        /// Linke Maustaste - Doppelklick
    unsigned num_tfingers; /// Anzahl Finger auf dem Display

    Position GetPos() const { return pos; }
};

/// Maximale Zeitdifferenz in ms für einen Doppeklick
const unsigned DOUBLE_CLICK_INTERVAL = 500;
