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
#ifndef MOUSEANDKEYS_H_INCLUDED
#define MOUSEANDKEYS_H_INCLUDED

#pragma once

#include "../../src/Point.h"

///////////////////////////////////////////////////////////////////////////////
/**
 *  Mausstatusstruct
 *
 *  @author OLiver
 */
class MouseCoords
{
    public:
        MouseCoords() : x(0), y(0), ldown(false), rdown(false), dbl_click(false) {}
        MouseCoords(int x, int y, bool ldown, bool rdown, const bool dbl_click)
            : x(x), y(y), ldown(ldown), rdown(rdown), dbl_click(dbl_click) {}

        int x;      /// xKoordinate
        int y;      /// yKoordinate
        bool ldown; /// Linke Maustaste gedrückt
        bool rdown; /// Rechte Maustaste gedrückt
        bool dbl_click; /// Linke Maustaste - Doppelklick

        Point<int> GetPos() const { return Point<int>(x, y); }
};

/// Maximale Zeitdifferenz in ms für einen Doppeklick
const unsigned DOUBLE_CLICK_INTERVAL = 500;

#endif //!MOUSEANDKEYS_H_INCLUDED
