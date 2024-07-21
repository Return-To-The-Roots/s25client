// Copyright (C) 2024 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "Cheats.h"
#include "CheatCommandTracker.h"
#include "network/GameClient.h"

Cheats::Cheats() : cheatCmdTracker_(std::make_unique<CheatCommandTracker>(*this)) {}

Cheats::~Cheats() = default;

void Cheats::trackKeyEvent(const KeyEvent& ke)
{
    cheatCmdTracker_->trackKeyEvent(ke);
}

void Cheats::trackChatCommand(const std::string& cmd)
{
    cheatCmdTracker_->trackChatCommand(cmd);
}

void Cheats::toggleCheatMode()
{
    isCheatModeOn_ = !isCheatModeOn_;
}

void Cheats::toggleHumanAIPlayer()
{
#ifdef NDEBUG
    const bool allowHumanAI = isCheatModeOn_;
#else
    const bool allowHumanAI = true;
#endif // !NDEBUG
    if(GAMECLIENT.GetState() == ClientState::Game && allowHumanAI && !GAMECLIENT.IsReplayModeOn())
        GAMECLIENT.ToggleHumanAIPlayer(AI::Info(AI::Type::Default, AI::Level::Easy));
}

void Cheats::armageddon()
{
    GAMECLIENT.CheatArmageddon();
}
