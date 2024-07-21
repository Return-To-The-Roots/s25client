// Copyright (C) 2024 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <memory>
#include <string>

class CheatCommandTracker;
struct KeyEvent;

class Cheats
{
public:
    Cheats();
    ~Cheats(); // = default - for unique_ptr

    void trackKeyEvent(const KeyEvent& ke);
    void trackChatCommand(const std::string& cmd);

    void toggleCheatMode();
    bool isCheatModeOn() const { return isCheatModeOn_; }

    void toggleHumanAIPlayer();
    void armageddon();

private:
    std::unique_ptr<CheatCommandTracker> cheatCmdTracker_;
    bool isCheatModeOn_ = false;
};
