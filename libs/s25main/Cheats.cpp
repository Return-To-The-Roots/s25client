// Copyright (C) 2024 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "Cheats.h"
#include "GameInterface.h"
#include "GamePlayer.h"
#include "factories/BuildingFactory.h"
#include "network/GameClient.h"
#include "world/GameWorldBase.h"

Cheats::Cheats(GameWorldBase& world) : world_(world) {}

bool Cheats::areCheatsAllowed() const
{
    return world_.IsSinglePlayer();
}

void Cheats::toggleCheatMode()
{
    // In S2, if you enabled cheat mode, revealed the map and disabled cheat mode, the map would remain revealed and you
    // would be unable to unreveal the map.
    // In RTTR, disabling cheat mode turns all cheats off and they have to be turned on again manually.
    if(isCheatModeOn_)
        turnAllCheatsOff();

    isCheatModeOn_ = !isCheatModeOn_;
}

void Cheats::toggleAllVisible()
{
    if(isCheatModeOn())
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
    // all buildings after enabling cheats.
    if(isCheatModeOn())
        areAllBuildingsEnabled_ = !areAllBuildingsEnabled_;
}

void Cheats::toggleShowEnemyProductivityOverlay()
{
    // In S2, if you enabled cheats you would automatically see the enemy productivity overlay - most importantly what
    // buildings the enemy intends to build.
    // In RTTR, the user must explicitly enable this feature after enabling cheats.
    if(isCheatModeOn())
        shouldShowEnemyProductivityOverlay_ = !shouldShowEnemyProductivityOverlay_;
}

bool Cheats::canPlaceCheatBuilding(const MapPoint& mp) const
{
    if(!isCheatModeOn())
        return false;

    // It seems that in the original game you can only build headquarters in unoccupied territory at least 2 nodes
    // away from any border markers and that it doesn't need more bq than a hut.
    const MapNode& node = world_.GetNode(mp);
    return !node.owner && !world_.IsAnyNeighborOwned(mp) && node.bq >= BuildingQuality::Hut;
}

void Cheats::placeCheatBuilding(const MapPoint& mp, const GamePlayer& player)
{
    if(!canPlaceCheatBuilding(mp))
        return;

    // The new HQ will have default resources.
    // In the original game, new HQs created in the Roman campaign had no resources.
    constexpr auto checkExists = false;
    world_.DestroyNO(mp, checkExists); // if CanPlaceCheatBuilding is true then this must be safe to destroy
    BuildingFactory::CreateBuilding(world_, BuildingType::Headquarters, mp, player.GetPlayerId(), player.nation,
                                    player.IsHQTent());
}

void Cheats::toggleHumanAIPlayer()
{
    if(isCheatModeOn() && !GAMECLIENT.IsReplayModeOn())
    {
        GAMECLIENT.ToggleHumanAIPlayer(AI::Info{AI::Type::Default, AI::Level::Easy});
        isHumanAIPlayer_ = !isHumanAIPlayer_;
    }
}

void Cheats::armageddon() const
{
    if(isCheatModeOn())
        GAMECLIENT.CheatArmageddon();
}

void Cheats::turnAllCheatsOff()
{
    if(isAllVisible_)
        toggleAllVisible();
    if(areAllBuildingsEnabled_)
        toggleAllBuildingsEnabled();
    if(shouldShowEnemyProductivityOverlay_)
        toggleShowEnemyProductivityOverlay();
    if(isHumanAIPlayer_)
        toggleHumanAIPlayer();
}
