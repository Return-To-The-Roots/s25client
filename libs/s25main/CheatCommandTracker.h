// Copyright (C) 2025 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <boost/circular_buffer.hpp>
#include <string>

class Cheats;
struct KeyEvent;

class CheatCommandTracker
{
public:
    CheatCommandTracker(Cheats& cheats);

    /**
     * Tracks keyboard events related to cheats and triggers the actual cheats.
     * Calls related private methods of this class in order but returns at the first success (return true).
     */
    void onKeyEvent(const KeyEvent& ke);
    /**
     * Tracks chat commands related to cheats and triggers the actual cheats.
     */
    void onChatCommand(const std::string& cmd);

private:
    /// Handle possible cheat events triggered by Keys different than KeyType::Char (e.g. F-keys).
    void onSpecialKeyEvent(const KeyEvent& ke);
    /// Check if keyboard event is related to game speed cheats (ALT+1..ALT+6) and triggers the actual cheats.
    /// true if keyboard event was related to game speed cheats, false otherwise
    bool checkSpeedKeyEvent(const KeyEvent& ke);
    /// Tracks keyboard events related to cheats and triggers the actual cheats for character keys,
    /// and e.g.enabling cheat mode by typing "winter"
    void onCharKeyEvent(const KeyEvent& ke);

    Cheats& cheats_;
    boost::circular_buffer<char> lastChars_;
};
