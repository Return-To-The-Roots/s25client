// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "GameLobbyController.h"
#include "GameLobby.h"
#include "JoinPlayerInfo.h"
#include "network/GameMessages.h"
#include "network/NetworkPlayer.h"
#include <utility>

GameLobbyController::GameLobbyController(std::shared_ptr<GameLobby> lobby, NetworkPlayer& mainPlayer)
    : mainPlayer_(mainPlayer), lobby(std::move(lobby))
{}

GameLobbyController::~GameLobbyController() = default;

unsigned GameLobbyController::getMaxNumPlayers() const
{
    return lobby->getNumPlayers();
}

JoinPlayerInfo& GameLobbyController::getJoinPlayer(unsigned playerIdx)
{
    return lobby->getPlayer(playerIdx);
}

void GameLobbyController::closeSlot(unsigned playerIdx)
{
    setPlayerState(playerIdx, PlayerState::Locked, AI::Info());
}

void GameLobbyController::setPlayerState(unsigned playerIdx, PlayerState state, const AI::Info& aiInfo)
{
    mainPlayer_.sendMsgAsync(new GameMessage_Player_State(playerIdx, state, aiInfo));
}

void GameLobbyController::togglePlayerState(unsigned playerIdx)
{
    const JoinPlayerInfo& player = lobby->getPlayer(playerIdx);

    PlayerState newPs = player.ps;
    AI::Info aiInfo = player.aiInfo;

    // playerstatus weiterwechseln
    switch(newPs)
    {
        default: break;
        case PlayerState::Occupied: newPs = PlayerState::Free; break;
        case PlayerState::Free:
            newPs = PlayerState::AI;
            aiInfo = AI::Info(AI::Type::Default, AI::Level::Easy);
            break;
        case PlayerState::AI:
            // Verschiedene KIs durchgehen
            switch(aiInfo.type)
            {
                case AI::Type::Default:
                default:
                    switch(aiInfo.level)
                    {
                        case AI::Level::Easy: aiInfo.level = AI::Level::Medium; break;
                        case AI::Level::Medium: aiInfo.level = AI::Level::Hard; break;
                        case AI::Level::Hard: aiInfo = AI::Info(AI::Type::Dummy); break;
                    }
                    break;
                case AI::Type::Dummy: newPs = PlayerState::Locked;
            }
            break;
        case PlayerState::Locked: newPs = PlayerState::Free; break;
    }
    setPlayerState(playerIdx, newPs, aiInfo);
}

void GameLobbyController::setColor(unsigned playerIdx, unsigned newColor)
{
    mainPlayer_.sendMsgAsync(new GameMessage_Player_Color(playerIdx, newColor));
}

void GameLobbyController::setTeam(unsigned playerIdx, Team newTeam)
{
    mainPlayer_.sendMsgAsync(new GameMessage_Player_Team(playerIdx, newTeam));
}

void GameLobbyController::setNation(unsigned playerIdx, Nation newNation)
{
    mainPlayer_.sendMsgAsync(new GameMessage_Player_Nation(playerIdx, newNation));
}

const GlobalGameSettings& GameLobbyController::getGGS() const
{
    return lobby->getSettings();
}

void GameLobbyController::changeGlobalGameSettings(const GlobalGameSettings& ggs)
{
    // Already change this here or future changes will be ignored before the server acknowledges the change
    lobby->getSettings() = ggs;
    mainPlayer_.sendMsgAsync(new GameMessage_GGSChange(ggs));
}

void GameLobbyController::swapPlayers(unsigned player1, unsigned player2)
{
    mainPlayer_.sendMsgAsync(new GameMessage_Player_Swap(player1, player2));
}

void GameLobbyController::startCountdown(unsigned numSecs)
{
    mainPlayer_.sendMsgAsync(new GameMessage_Countdown(numSecs));
}

void GameLobbyController::cancelCountdown()
{
    mainPlayer_.sendMsgAsync(new GameMessage_CancelCountdown());
}

void GameLobbyController::removeLuaScript()
{
    mainPlayer_.sendMsgAsync(new GameMessage_RemoveLua());
}

void GameLobbyController::setName(unsigned playerIdx, const std::string& name)
{
    mainPlayer_.sendMsgAsync(new GameMessage_Player_Name(playerIdx, name));
}

void GameLobbyController::setPortrait(unsigned playerIdx, unsigned portraitIndex)
{
    mainPlayer_.sendMsgAsync(new GameMessage_Player_Portrait(playerIdx, portraitIndex));
}
