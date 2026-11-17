// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "RttrLobbyClient.hpp"
#include "liblobby/LobbyClient.h"

RttrLobbyClient::RttrLobbyClient(LobbyClient& client) : client_(client) {}

bool RttrLobbyClient::isLoggedIn() const
{
    return client_.isLoggedIn();
}

void RttrLobbyClient::addListener(LobbyInterface* listener)
{
    client_.addListener(listener);
}

void RttrLobbyClient::removeListener(LobbyInterface* listener)
{
    client_.removeListener(listener);
}

void RttrLobbyClient::sendServerJoinRequest()
{
    client_.sendServerJoinRequest();
}

void RttrLobbyClient::sendChat(const std::string& text)
{
    client_.sendChat(text);
}
