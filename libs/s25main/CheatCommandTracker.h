// Copyright (C) 2024 Settlers Freaks (sf-team at siedler25.org)
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
    CheatCommandTracker(Cheats* cheats);

    void trackKeyEvent(const KeyEvent& ke);
    void trackChatCommand(const std::string& cmd);

private:
    bool trackSpecialKeyEvent(const KeyEvent& ke);
    void trackCharKeyEvent(const KeyEvent& ke);

    Cheats* cheats_;
    boost::circular_buffer<char> lastChars_;
};
