// Copyright (C) 2024 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "Cheats.h"
#include "CheatCommandTracker.h"
#include "GameInterface.h"
#include "GamePlayer.h"
#include "RttrForeachPt.h"
#include "factories/BuildingFactory.h"
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

void Cheats::toggleAllVisible()
{
    // This is actually the behavior of the original game.
    // If you enabled cheats, revealed the map and disabled cheats you would be unable to unreveal the map.
    if(!isCheatModeOn())
        return;

    isAllVisible_ = !isAllVisible_;

    // The minimap in the original game is not updated immediately, but here this would cause complications.
    if(GameInterface* gi = world_.GetGameInterface())
        gi->GI_UpdateMapVisibility();
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

void Cheats::setGameSpeed(uint8_t speedIndex)
{
    if(!isCheatModeOn())
        return;

    constexpr auto gfLengthInMs = 50;
    GAMECLIENT.SetGFLengthReq(FramesInfo::milliseconds32_t{gfLengthInMs >> speedIndex});
    // 50 -> 25 -> 12 -> 6 -> 3 -> 1
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

Cheats::ResourceRevealMode Cheats::getResourceRevealMode() const
{
    return isCheatModeOn() ? resourceRevealMode_ : ResourceRevealMode::Nothing;
}

void Cheats::toggleResourceRevealMode()
{
    switch(resourceRevealMode_)
    {
        case ResourceRevealMode::Nothing: resourceRevealMode_ = ResourceRevealMode::Ores; break;
        case ResourceRevealMode::Ores: resourceRevealMode_ = ResourceRevealMode::Fish; break;
        case ResourceRevealMode::Fish: resourceRevealMode_ = ResourceRevealMode::Water; break;
        default: resourceRevealMode_ = ResourceRevealMode::Nothing; break;
    }
}

void Cheats::destroyBuildings(const PlayerIDSet& playerIds)
{
    if(!isCheatModeOn())
        return;

    RTTR_FOREACH_PT(MapPoint, world_.GetSize())
        if(world_.GetNO(pt)->GetType() == NodalObjectType::Building && playerIds.count(world_.GetNode(pt).owner - 1))
            world_.DestroyNO(pt);
}

void Cheats::destroyAllAIBuildings()
{
    if(!isCheatModeOn())
        return;

    PlayerIDSet ais;
    for(auto i = 0u; i < world_.GetNumPlayers(); ++i)
        if(!world_.GetPlayer(i).isHuman())
            ais.insert(i);
    destroyBuildings(ais);
}

bool Cheats::canCheatModeBeOn() const
{
    return world_.IsSinglePlayer();
}
