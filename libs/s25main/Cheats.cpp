// Copyright (C) 2024 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "Cheats.h"
#include "network/GameClient.h"
#include "world/GameWorldBase.h"

Cheats::Cheats(GameWorldBase& world) : world_(world) {}

bool Cheats::areCheatsAllowed() const
{
    return world_.IsSinglePlayer();
}

void Cheats::toggleCheatMode()
{
    isCheatModeOn_ = !isCheatModeOn_;
}

void Cheats::toggleHumanAIPlayer() const
{
    if(isCheatModeOn() && !GAMECLIENT.IsReplayModeOn())
        GAMECLIENT.ToggleHumanAIPlayer(AI::Info{AI::Type::Default, AI::Level::Easy});
}

void Cheats::armageddon() const
{
    if(isCheatModeOn())
        GAMECLIENT.CheatArmageddon();
}
