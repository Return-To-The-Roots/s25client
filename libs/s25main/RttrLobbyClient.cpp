// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "RttrLobbyClient.hpp"
#include "liblobby/LobbyClient.h"

RttrLobbyClient::RttrLobbyClient(LobbyClient& client) : client_(client) {}

bool RttrLobbyClient::IsLoggedIn() const
{
    return client_.IsLoggedIn();
}

void RttrLobbyClient::addListener(LobbyInterface* listener)
{
    client_.AddListener(listener);
}

void RttrLobbyClient::removeListener(LobbyInterface* listener)
{
    client_.RemoveListener(listener);
}

void RttrLobbyClient::sendServerJoinRequest()
{
    client_.SendServerJoinRequest();
}

void RttrLobbyClient::sendChat(const std::string& text)
{
    client_.SendChat(text);
}
