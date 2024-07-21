// Copyright (C) 2024 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "CheatCommandTracker.h"
#include "Cheats.h"
#include "driver/KeyEvent.h"

void CheatCommandTracker::trackKeyEvent(const KeyEvent& ke)
{
    switch(ke.kt)
    {
        case KeyType::F10: return cheats_.toggleHumanAIPlayer();
        default:;
    }

    static std::string winterCheat = "winter";
    switch(ke.c)
    {
        case 'w':
        case 'i':
        case 'n':
        case 't':
        case 'e':
        case 'r':
            curCheatTxt_ += char(ke.c);
            if(winterCheat.find(curCheatTxt_) == 0)
            {
                if(curCheatTxt_ == winterCheat)
                {
                    cheats_.toggleCheatMode();
                    curCheatTxt_.clear();
                }
            } else
                curCheatTxt_.clear();
            break;
    }
}

void CheatCommandTracker::trackChatCommand(const std::string& cmd)
{
    if(cmd == "apocalypsis")
        cheats_.armageddon();
}
