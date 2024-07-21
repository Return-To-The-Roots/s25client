// Copyright (C) 2024 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "Cheats.h"
#include "driver/KeyEvent.h"
#include "network/GameClient.h"

void Cheats::trackKeyEvent(const KeyEvent& ke)
{
    switch(ke.kt)
    {
        case KeyType::F10:
        {
#ifdef NDEBUG
            const bool allowHumanAI = isCheatModeOn_;
#else
            const bool allowHumanAI = true;
#endif // !NDEBUG
            if(GAMECLIENT.GetState() == ClientState::Game && allowHumanAI && !GAMECLIENT.IsReplayModeOn())
                GAMECLIENT.ToggleHumanAIPlayer(AI::Info(AI::Type::Default, AI::Level::Easy));
        }
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
                    isCheatModeOn_ = !isCheatModeOn_;
                    curCheatTxt_.clear();
                }
            } else
                curCheatTxt_.clear();
            break;
    }
}

void Cheats::trackChatCommand(const std::string& cmd)
{
    if(cmd == "apocalypsis")
        GAMECLIENT.CheatArmageddon();
}
