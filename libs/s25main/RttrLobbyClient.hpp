// Copyright (C) 2016 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "ILobbyClient.hpp"

class LobbyClient;

/// Default lobby client for RTTR
class RttrLobbyClient : public ILobbyClient
{
    LobbyClient& client_;

public:
    explicit RttrLobbyClient(LobbyClient& client);
    bool IsLoggedIn() const override;
    void AddListener(LobbyInterface* listener) override;
    void RemoveListener(LobbyInterface* listener) override;
    void SendServerJoinRequest() override;
    void SendChat(const std::string& text) override;
};
