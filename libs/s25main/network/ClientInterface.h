// Copyright (c) 2005 - 2017 Settlers Freaks (sf-team at siedler25.org)
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

#include "ClientError.h"
#include "gameTypes/ChatDestination.h"
#include "gameTypes/PlayerState.h"
#include "gameData/NationConsts.h"
#include <memory>

class Game;
class GlobalGameSettings;

/// Verbindungsstatus beim Verbinden zum Server
enum class ConnectState
{
    WaitForAnswer,
    QueryPw,
    QueryMapName,
    QueryPlayerList,
    Finished
};

class ClientInterface
{
public:
    virtual ~ClientInterface() = default;

    virtual void CI_NextConnectState(ConnectState) {}
    virtual void CI_Error(ClientError) {}

    virtual void CI_NewPlayer(unsigned /*playerId*/) {}
    virtual void CI_PlayerLeft(unsigned /*playerId*/) {}
    /// Game entered loading state
    virtual void CI_GameLoading(std::shared_ptr<Game>) {}
    /// Game is started and running
    virtual void CI_GameStarted() {}

    virtual void CI_PlayerDataChanged(unsigned /*playerId*/) {}
    virtual void CI_PingChanged(unsigned /*playerId*/, unsigned short /*ping*/) {}
    virtual void CI_ReadyChanged(unsigned /*playerId*/, bool /*ready*/) {}
    virtual void CI_PlayersSwapped(unsigned /*player1*/, unsigned /*player2*/) {}
    virtual void CI_GGSChanged(const GlobalGameSettings&) {}

    virtual void CI_Chat(unsigned /*playerId*/, ChatDestination /*cd*/, const std::string& /*msg*/) {}
    virtual void CI_Countdown(unsigned /*remainingTimeInSec*/) {}
    virtual void CI_CancelCountdown(bool /*error*/) {}

    virtual void CI_Async(const std::string& /*checksums_list*/) {}
    virtual void CI_ReplayAsync(const std::string& /*msg*/) {}
    virtual void CI_ReplayEndReached(const std::string& /*msg*/) {}
    virtual void CI_GamePaused() {}
    virtual void CI_GameResumed() {}
};
