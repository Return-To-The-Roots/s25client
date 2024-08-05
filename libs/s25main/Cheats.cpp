// Copyright (C) 2024 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "Cheats.h"
#include "network/GameClient.h"
#include "world/GameWorldBase.h"

Cheats::Cheats(GameWorldBase& world) : cheatCmdTracker_(*this), world_(world) {}

Cheats::~Cheats() = default;

void Cheats::trackKeyEvent(const KeyEvent& ke)
{
    if(areCheatsAllowed())
        cheatCmdTracker_.trackKeyEvent(ke);
}

void Cheats::trackChatCommand(const std::string& cmd)
{
    if(areCheatsAllowed())
        cheatCmdTracker_.trackChatCommand(cmd);
}

void Cheats::toggleCheatMode()
{
    if(areCheatsAllowed())
        isCheatModeOn_ = !isCheatModeOn_;
}

void Cheats::toggleHumanAIPlayer()
{
    if(isCheatModeOn() && !GAMECLIENT.IsReplayModeOn())
        GAMECLIENT.ToggleHumanAIPlayer(AI::Info{AI::Type::Default, AI::Level::Easy});
}

void Cheats::armageddon()
{
    if(isCheatModeOn())
        GAMECLIENT.CheatArmageddon();
}

bool Cheats::areCheatsAllowed() const
{
    return world_.IsSinglePlayer();
}
