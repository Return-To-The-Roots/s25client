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
#ifndef CLIENTGUIINTERFACE_H_
#define CLIENTGUIINTERFACE_H_

#pragma once

#include "ClientError.h"
#include "gameTypes/ChatDestination.h"
#include "gameTypes/PlayerState.h"
#include "gameData/NationConsts.h"
#include <boost/shared_ptr.hpp>

class Game;
class GlobalGameSettings;

/// Verbindungsstatus beim Verbinden zum Server
enum ConnectState
{
    CS_WAITFORANSWER = 0,
    CS_QUERYPW,
    CS_QUERYMAPNAME,
    CS_QUERYPLAYERLIST,
    CS_FINISHED,
    CS_REGISTERED,
    CS_LOGGEDIN,
    CS_SERVERCREATED
};

class ClientInterface
{
public:
    virtual ~ClientInterface() {}

    virtual void CI_NextConnectState(const ConnectState) {}
    virtual void CI_Error(const ClientError) {}

    virtual void CI_NewPlayer(const unsigned /*playerId*/) {}
    virtual void CI_PlayerLeft(const unsigned /*playerId*/) {}
    /// Game entered loading state
    virtual void CI_GameLoading(boost::shared_ptr<Game>) {}
    /// Game is started and running
    virtual void CI_GameStarted(boost::shared_ptr<Game>) {}

    virtual void CI_PlayerDataChanged(unsigned /*playerId*/) {}
    virtual void CI_PingChanged(const unsigned /*playerId*/, const unsigned short /*ping*/) {}
    virtual void CI_ReadyChanged(const unsigned /*playerId*/, const bool /*ready*/) {}
    virtual void CI_PlayersSwapped(const unsigned /*player1*/, const unsigned /*player2*/) {}
    virtual void CI_GGSChanged(const GlobalGameSettings&) {}

    virtual void CI_Chat(const unsigned /*playerId*/, const ChatDestination /*cd*/, const std::string& /*msg*/) {}
    virtual void CI_Countdown(unsigned /*remainingTimeInSec*/) {}
    virtual void CI_CancelCountdown(bool /*error*/) {}

    virtual void CI_Async(const std::string& /*checksums_list*/) {}
    virtual void CI_ReplayAsync(const std::string& /*msg*/) {}
    virtual void CI_ReplayEndReached(const std::string& /*msg*/) {}
    virtual void CI_GamePaused() {}
    virtual void CI_GameResumed() {}
};

#endif
