// Copyright (C) 2024 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "Cheats.h"
#include "CheatCommandTracker.h"
#include "network/GameClient.h"
#include "world/GameWorldBase.h"

Cheats::Cheats(GameWorldBase& world) : cheatCmdTracker_(std::make_unique<CheatCommandTracker>(*this)), world_(world) {}

Cheats::~Cheats() = default;

void Cheats::trackKeyEvent(const KeyEvent& ke)
{
    if(!canCheatModeBeOn())
        return;

    cheatCmdTracker_->trackKeyEvent(ke);
}

void Cheats::trackChatCommand(const std::string& cmd)
{
    if(!canCheatModeBeOn())
        return;

    cheatCmdTracker_->trackChatCommand(cmd);
}

void Cheats::toggleCheatMode()
{
    if(!canCheatModeBeOn())
        return;

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
    if(!isCheatModeOn())
        return;

    GAMECLIENT.CheatArmageddon();
}

bool Cheats::canCheatModeBeOn() const
{
    return world_.IsSinglePlayer();
}
