// Copyright (c) 20052008 Settlers Freaks (sfteam at siedler25.org)
//
// This program is free software; you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by the
// Free Software Foundation; either version 2, or (at your option) any
// later version.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
// THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.

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
