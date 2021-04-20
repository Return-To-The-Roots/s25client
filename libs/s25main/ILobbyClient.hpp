// Copyright (C) 2016 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <string>

class LobbyInterface;

/// Interface for lobby clients
class ILobbyClient
{
public:
    virtual ~ILobbyClient() = default;
    virtual bool IsLoggedIn() const = 0;
    virtual void AddListener(LobbyInterface* listener) = 0;
    virtual void RemoveListener(LobbyInterface* listener) = 0;
    virtual void SendServerJoinRequest() = 0;
    virtual void SendChat(const std::string& text) = 0;
};
