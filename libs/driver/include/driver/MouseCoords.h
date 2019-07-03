// Copyright (c) 2005 - 2019 Settlers Freaks (sf-team at siedler25.org)
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
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once
#ifndef libs_driver_include_driver_MouseCoords_h
#define libs_driver_include_driver_MouseCoords_h

#include <Point.h>

///////////////////////////////////////////////////////////////////////////////
/**
 *  Mausstatusstruct
 *
 *  @author OLiver
 */
class MouseCoords
{
public:
    MouseCoords()
        : pos(0, 0), ldown(false), rdown(false), dbl_click(false)
    {}

    MouseCoords(int x, int y, bool ldown = false, bool rdown = false, bool dbl_click = false)
        : pos(x, y), ldown(ldown), rdown(rdown), dbl_click(dbl_click)
    {}

    MouseCoords(Position pos, bool ldown = false, bool rdown = false, bool dbl_click = false)
        : pos(pos), ldown(ldown), rdown(rdown), dbl_click(dbl_click)
    {}

    Position GetPos() const { return pos; }

public:
    Position pos;
    bool ldown;     /// Linke Maustaste gedrückt
    bool rdown;     /// Rechte Maustaste gedrückt
    bool dbl_click; /// Linke Maustaste - Doppelklick
};

/// Maximale Zeitdifferenz in ms für einen Doppeklick
const unsigned DOUBLE_CLICK_INTERVAL = 500;

#endif // !libs_driver_include_driver_MouseCoords_h
