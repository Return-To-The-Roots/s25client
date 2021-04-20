// Copyright (c) 2005 - 2017 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "network/IGameLobbyController.h"
#include "gameTypes/AIInfo.h"
#include "gameTypes/PlayerState.h"
#include <memory>

class GameLobby;
class GlobalGameSettings;
class NetworkPlayer;

class GameLobbyController : public IGameLobbyController
{
    NetworkPlayer& mainPlayer_;

public:
    std::shared_ptr<GameLobby> lobby;
    GameLobbyController(std::shared_ptr<GameLobby> lobby, NetworkPlayer& mainPlayer);
    ~GameLobbyController() override;

    unsigned GetMaxNumPlayers() const override;
    JoinPlayerInfo& GetJoinPlayer(unsigned playerIdx) override;
    void CloseSlot(unsigned playerIdx) override;
    void SetPlayerState(unsigned playerIdx, PlayerState state, const AI::Info& aiInfo) override;
    void TogglePlayerState(unsigned playerIdx);
    void SetName(unsigned playerIdx, const std::string& name) override;
    void SetColor(unsigned playerIdx, unsigned newColor) override;
    void SetTeam(unsigned playerIdx, Team newTeam) override;
    void SetNation(unsigned playerIdx, Nation newNation) override;
    const GlobalGameSettings& GetGGS() const override;
    void ChangeGlobalGameSettings(const GlobalGameSettings& ggs) override;
    void SwapPlayers(unsigned player1, unsigned player2);
    void StartCountdown(unsigned numSecs);
    void CancelCountdown();
    void RemoveLuaScript();
};
