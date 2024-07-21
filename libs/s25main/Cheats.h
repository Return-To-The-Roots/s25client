// Copyright (C) 2024 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <string>

struct KeyEvent;

class Cheats
{
public:
    bool isCheatModeOn() const { return isCheatModeOn_; }

    void trackKeyEvent(const KeyEvent& ke);
    void trackChatCommand(const std::string& cmd);

private:
    bool isCheatModeOn_ = false;
    std::string curCheatTxt_;
};
