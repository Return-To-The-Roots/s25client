// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

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
    Initiated,
    VerifyServer,
    QueryPw,
    QueryMapInfo,
    ReceiveMap,
    VerifyMap,
    QueryServerName,
    QueryPlayerList,
    QuerySettings,
    Finished
};

class ClientInterface
{
public:
    virtual ~ClientInterface() = default;

    virtual void CI_NextConnectState(ConnectState) {}
    virtual void CI_MapPartReceived(uint32_t /*bytesReceived*/, uint32_t /*bytesTotal*/) {}
    virtual void CI_Error(ClientError) {}

    virtual void CI_NewPlayer(unsigned /*playerId*/) {}
    virtual void CI_PlayerLeft(unsigned /*playerId*/) {}
    /// Game entered loading state
    virtual void CI_GameLoading(std::shared_ptr<Game>) {} // NOLINT(performance-unnecessary-value-param)
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
