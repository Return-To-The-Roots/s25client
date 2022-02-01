// Copyright (C) 2005 - 2022 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "ILobbyClient.hpp"
#include "IngameWindow.h"
#include "network/ClientInterface.h"
#include "gameTypes/ServerType.h"
#include <boost/signals2.hpp>

/// Window to show status progress during a connecting attempt
/// At the end either the onError signal is triggered or the current desktop changed to dskGameLobby
/// In either case this window will be closed
class iwConnecting : public IngameWindow, public ClientInterface
{
public:
    iwConnecting(ServerType serverType, std::unique_ptr<ILobbyClient> lobbyClient);

    /// Signal triggered when the connection is aborted due to an error
    /// If unset an error message box is shown
    boost::signals2::signal<void(ClientError)> onError;

    void Close() override;
    void Msg_ButtonClick(unsigned ctrlId) override;

private:
    ServerType serverType_;
    std::unique_ptr<ILobbyClient> lobbyClient_;
    unsigned short downloadProgress_ = 0;

    void CI_Error(ClientError ce) override;
    void CI_NextConnectState(ConnectState cs) override;
    void CI_MapPartReceived(uint32_t bytesReceived, uint32_t bytesTotal) override;
    void setStatus(const std::string& status);
};
