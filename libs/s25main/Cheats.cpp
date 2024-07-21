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
    if(!isCheatModeOn())
        return;

    if(GAMECLIENT.IsReplayModeOn())
        return;

    GAMECLIENT.ToggleHumanAIPlayer(AI::Info{AI::Type::Default, AI::Level::Easy});
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
