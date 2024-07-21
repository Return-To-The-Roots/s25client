// Copyright (C) 2024 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <memory>
#include <string>

class CheatCommandTracker;
class GameWorldBase;
struct KeyEvent;

class Cheats
{
public:
    Cheats(GameWorldBase& world);
    ~Cheats(); // = default - for unique_ptr

    void trackKeyEvent(const KeyEvent& ke);
    void trackChatCommand(const std::string& cmd);

    void toggleCheatMode();
    bool isCheatModeOn() const { return isCheatModeOn_; }

    void toggleHumanAIPlayer();
    void armageddon();

private:
    bool canCheatModeBeOn() const;

    std::unique_ptr<CheatCommandTracker> cheatCmdTracker_;
    bool isCheatModeOn_ = false;
    GameWorldBase& world_;
};
