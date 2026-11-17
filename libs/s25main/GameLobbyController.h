// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
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

    unsigned getMaxNumPlayers() const override;
    JoinPlayerInfo& getJoinPlayer(unsigned playerIdx) override;
    void closeSlot(unsigned playerIdx) override;
    void setPlayerState(unsigned playerIdx, PlayerState state, const AI::Info& aiInfo) override;
    void togglePlayerState(unsigned playerIdx);
    void setName(unsigned playerIdx, const std::string& name) override;
    void setPortrait(unsigned playerIdx, unsigned portraitIndex) override;
    void setColor(unsigned playerIdx, unsigned newColor) override;
    void setTeam(unsigned playerIdx, Team newTeam) override;
    void setNation(unsigned playerIdx, Nation newNation) override;
    const GlobalGameSettings& getGGS() const override;
    void changeGlobalGameSettings(const GlobalGameSettings& ggs) override;
    void swapPlayers(unsigned player1, unsigned player2);
    void startCountdown(unsigned numSecs);
    void cancelCountdown();
    void removeLuaScript();
};
