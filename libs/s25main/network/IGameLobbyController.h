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
    virtual unsigned GetMaxNumPlayers() const = 0;
    virtual JoinPlayerInfo& GetJoinPlayer(unsigned playerIdx) = 0;
    virtual void CloseSlot(unsigned playerIdx) = 0;
    virtual void SetPlayerState(unsigned playerIdx, PlayerState state, const AI::Info& aiInfo) = 0;
    virtual void SetName(unsigned playerIdx, const std::string& name) = 0;
    virtual void SetColor(unsigned playerIdx, unsigned newColor) = 0;
    virtual void SetTeam(unsigned playerIdx, Team newTeam) = 0;
    virtual void SetNation(unsigned playerIdx, Nation newNation) = 0;
    virtual const GlobalGameSettings& GetGGS() const = 0;
    virtual void ChangeGlobalGameSettings(const GlobalGameSettings& ggs) = 0;
};
