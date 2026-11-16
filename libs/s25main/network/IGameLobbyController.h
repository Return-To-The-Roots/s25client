// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "gameTypes/AIInfo.h"
#include "gameTypes/Nation.h"
#include "gameTypes/PlayerState.h"
#include "gameTypes/TeamTypes.h"
#include <string>

class GameMessage;
class GlobalGameSettings;
struct JoinPlayerInfo;

/// Abstract interface for the GameServer so it can be emulated for testing
class IGameLobbyController
{
public:
    virtual ~IGameLobbyController() = default;
    virtual unsigned getMaxNumPlayers() const = 0;
    virtual JoinPlayerInfo& getJoinPlayer(unsigned playerIdx) = 0;
    virtual void closeSlot(unsigned playerIdx) = 0;
    virtual void setPlayerState(unsigned playerIdx, PlayerState state, const AI::Info& aiInfo) = 0;
    virtual void setName(unsigned playerIdx, const std::string& name) = 0;
    virtual void setPortrait(unsigned playerIdx, unsigned portraitIndex) = 0;
    virtual void setColor(unsigned playerIdx, unsigned newColor) = 0;
    virtual void setTeam(unsigned playerIdx, Team newTeam) = 0;
    virtual void setNation(unsigned playerIdx, Nation newNation) = 0;
    virtual const GlobalGameSettings& getGGS() const = 0;
    virtual void changeGlobalGameSettings(const GlobalGameSettings& ggs) = 0;
};
