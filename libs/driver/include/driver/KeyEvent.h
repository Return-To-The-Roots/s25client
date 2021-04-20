// Copyright (c) 2005 - 2021 Settlers Freaks (sfteam at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

///////////////////////////////////////////////////////////////////////////////
/**
 *  Keydown Spezial Tasten
 *
 *  @author OLiver
 */
enum class KeyType
{
    Invalid,
    Space,
    Return,
    Left,
    Up,
    Right,
    Down,
    Backspace,
    Delete,
    Shift,
    Tab,
    F1,
    F2,
    F3,
    F4,
    F5,
    F6,
    F7,
    F8,
    F9,
    F10,
    F11,
    F12,
    End,  // Ende
    Home, // Pos1
    Escape,
    Print,
    Char
};

/// TastatureventStruktur
struct KeyEvent
{
    KeyType kt;
    unsigned c;
    bool ctrl, shift, alt;
};
