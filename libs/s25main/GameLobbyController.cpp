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

unsigned GameLobbyController::GetMaxNumPlayers() const
{
    return lobby->getNumPlayers();
}

JoinPlayerInfo& GameLobbyController::GetJoinPlayer(unsigned playerIdx)
{
    return lobby->getPlayer(playerIdx);
}

void GameLobbyController::CloseSlot(unsigned playerIdx)
{
    SetPlayerState(playerIdx, PlayerState::Locked, AI::Info());
}

void GameLobbyController::SetPlayerState(unsigned playerIdx, PlayerState state, const AI::Info& aiInfo)
{
    mainPlayer_.sendMsgAsync(new GameMessage_Player_State(playerIdx, state, aiInfo));
}

void GameLobbyController::TogglePlayerState(unsigned playerIdx)
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
    SetPlayerState(playerIdx, newPs, aiInfo);
}

void GameLobbyController::SetColor(unsigned playerIdx, unsigned newColor)
{
    mainPlayer_.sendMsgAsync(new GameMessage_Player_Color(playerIdx, newColor));
}

void GameLobbyController::SetTeam(unsigned playerIdx, Team newTeam)
{
    mainPlayer_.sendMsgAsync(new GameMessage_Player_Team(playerIdx, newTeam));
}

void GameLobbyController::SetNation(unsigned playerIdx, Nation newNation)
{
    mainPlayer_.sendMsgAsync(new GameMessage_Player_Nation(playerIdx, newNation));
}

const GlobalGameSettings& GameLobbyController::GetGGS() const
{
    return lobby->getSettings();
}

void GameLobbyController::ChangeGlobalGameSettings(const GlobalGameSettings& ggs)
{
    // Already change this here or future changes will be ignored before the server acknowledges the change
    lobby->getSettings() = ggs;
    mainPlayer_.sendMsgAsync(new GameMessage_GGSChange(ggs));
}

void GameLobbyController::SwapPlayers(unsigned player1, unsigned player2)
{
    mainPlayer_.sendMsgAsync(new GameMessage_Player_Swap(player1, player2));
}

void GameLobbyController::StartCountdown(unsigned numSecs)
{
    mainPlayer_.sendMsgAsync(new GameMessage_Countdown(numSecs));
}

void GameLobbyController::CancelCountdown()
{
    mainPlayer_.sendMsgAsync(new GameMessage_CancelCountdown());
}

void GameLobbyController::RemoveLuaScript()
{
    mainPlayer_.sendMsgAsync(new GameMessage_RemoveLua());
}

void GameLobbyController::SetName(unsigned playerIdx, const std::string& name)
{
    mainPlayer_.sendMsgAsync(new GameMessage_Player_Name(playerIdx, name));
}
