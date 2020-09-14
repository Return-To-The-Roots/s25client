// Copyright (c) 2016 - 2017 Settlers Freaks (sf-team at siedler25.org)
//
// This file is part of Return To The Roots.
//
// Return To The Roots is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 2 of the License, or
// (at your option) any later version.
//
// Return To The Roots is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with Return To The Roots. If not, see <http://www.gnu.org/licenses/>.

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
