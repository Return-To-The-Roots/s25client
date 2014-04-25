// $Id: MouseAndKeys.h 9357 2014-04-25 15:35:25Z FloSoft $
//
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

    public:
        int x;      /// xKoordinate
        int y;      /// yKoordinate
        bool ldown; /// Linke Maustaste gedrückt
        bool rdown; /// Rechte Maustaste gedrückt
        bool dbl_click; /// Linke Maustaste - Doppelklick
};

/// Maximale Zeitdifferenz in ms für einen Doppeklick
const unsigned DOUBLE_CLICK_INTERVAL = 500;

///////////////////////////////////////////////////////////////////////////////
/**
 *  Keydown Spezial Tasten
 *
 *  @author OLiver
 */
enum KeyType
{
    KT_INVALID = 0,
    KT_SPACE,
    KT_RETURN,
    KT_LEFT,
    KT_UP,
    KT_RIGHT,
    KT_DOWN,
    KT_BACKSPACE,
    KT_DELETE,
    KT_SHIFT,
    KT_TAB,
    KT_F1, KT_F2, KT_F3, KT_F4, KT_F5, KT_F6, KT_F7, KT_F8, KT_F9, KT_F10, KT_F11, KT_F12,
    KT_END,  // Ende
    KT_HOME, // Pos1
    KT_ESCAPE,
    KT_CHAR = 0xFFFFFFFF
};

/// TastatureventStruktur
struct KeyEvent
{
    KeyType kt;
    unsigned int c;
    bool ctrl, shift, alt;
};

/// ScreenResize-Event
struct ScreenResizeEvent
{
    unsigned short oldWidth;
    unsigned short oldHeight;
    unsigned short newWidth;
    unsigned short newHeight;
};

#endif //!MOUSEANDKEYS_H_INCLUDED
