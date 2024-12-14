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
    // In S2, if you enabled cheats, revealed the map and disabled cheats, you would be unable to unreveal the map.
    // In RTTR, with cheats disabled, you can unreveal the map but cannot reveal it.
    if(isCheatModeOn() || isAllVisible())
    {
        isAllVisible_ = !isAllVisible_;

        // In S2, the minimap is not updated immediately.
        // In RTTR, the minimap would become messed up if it wasn't updated here.
        if(GameInterface* gi = world_.GetGameInterface())
            gi->GI_UpdateMapVisibility();
    }
}

void Cheats::toggleAllBuildingsEnabled()
{
    // In S2, if you enabled cheats you would automatically have all buildings enabled.
    // In RTTR, because this may have unintended consequences when playing campaigns, the user must explicitly enable
    // all buildings after enabling cheats. This function follows the same logic as toggleAllVisible in that it will
    // allow disabling this feature even when cheats are off.
    if(isCheatModeOn() || areAllBuildingsEnabled())
        areAllBuildingsEnabled_ = !areAllBuildingsEnabled_;
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
