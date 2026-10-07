// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "GameLobby.h"
#include "JoinPlayerInfo.h"

GameLobby::GameLobby(bool isSavegame, bool isHost, unsigned numPlayers)
    : isSavegame_(isSavegame), isHost_(isHost), players_(numPlayers)
{}
GameLobby::~GameLobby() = default;

JoinPlayerInfo& GameLobby::getPlayer(unsigned playerId)
{
    return players_.at(playerId);
}

const JoinPlayerInfo& GameLobby::getPlayer(unsigned playerId) const
{
    return players_.at(playerId);
}

unsigned GameLobby::getNumPlayers() const
{
    return players_.size();
}

void GameLobby::setNumPlayers(unsigned num)
{
    players_.resize(num);
}
