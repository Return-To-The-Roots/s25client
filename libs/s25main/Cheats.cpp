// Copyright (C) 2024 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "Cheats.h"
#include "GameInterface.h"
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

void Cheats::toggleAllVisible()
{
    // In the original game if you enabled cheats, revealed the map and disabled cheats, you would be unable to unreveal
    // the map. In RTTR, with cheats disabled, you can unreveal the map but cannot reveal it.
    if(isCheatModeOn() || isAllVisible())
    {
        isAllVisible_ = !isAllVisible_;

        // The minimap in the original game is not updated immediately, but in RTTR this would mess up the minimap.
        if(GameInterface* gi = world_.GetGameInterface())
            gi->GI_UpdateMapVisibility();
    }
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
