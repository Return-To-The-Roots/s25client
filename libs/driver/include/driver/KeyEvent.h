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
#ifndef libs_driver_include_driver_KeyEvent_h
#define libs_driver_include_driver_KeyEvent_h

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
    KT_F1,
    KT_F2,
    KT_F3,
    KT_F4,
    KT_F5,
    KT_F6,
    KT_F7,
    KT_F8,
    KT_F9,
    KT_F10,
    KT_F11,
    KT_F12,
    KT_END,  // Ende
    KT_HOME, // Pos1
    KT_ESCAPE,
    KT_PRINT,
    KT_CHAR = 0xFFFFFFFF
};

/// TastatureventStruktur
struct KeyEvent
{
    KeyType kt;
    unsigned c;
    bool ctrl, shift, alt;
};

#endif // !libs_driver_include_driver_KeyEvent_h
