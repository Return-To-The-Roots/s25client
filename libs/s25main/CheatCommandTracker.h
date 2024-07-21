// Copyright (C) 2024 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <string>

class Cheats;
struct KeyEvent;

class CheatCommandTracker
{
public:
    CheatCommandTracker(Cheats& cheats) : cheats_(cheats) {}

    void trackKeyEvent(const KeyEvent& ke);
    void trackChatCommand(const std::string& cmd);

private:
    Cheats& cheats_;
    std::string curCheatTxt_;
};
