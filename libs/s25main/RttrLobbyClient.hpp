// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
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
    bool isLoggedIn() const override;
    void addListener(LobbyInterface* listener) override;
    void removeListener(LobbyInterface* listener) override;
    void sendServerJoinRequest() override;
    void sendChat(const std::string& text) override;
};
