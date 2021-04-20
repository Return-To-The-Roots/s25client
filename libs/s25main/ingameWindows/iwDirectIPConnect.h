// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "IngameWindow.h"
#include "network/ClientInterface.h"
#include "gameTypes/ServerType.h"

class iwDirectIPConnect : public IngameWindow, public ClientInterface
{
private:
    ServerType server_type;

public:
    iwDirectIPConnect(ServerType server_type);
    void SetHost(const std::string& hostIp);
    void SetPort(unsigned short port);
    /// Connects to the given server or fills in the info if it has a password
    void Connect(const std::string& hostOrIp, unsigned short port, bool isIPv6, bool hasPwd);

private:
    void SetStatus(const std::string& text, unsigned color);

    void Msg_EditChange(unsigned ctrl_id) override;
    void Msg_EditEnter(unsigned ctrl_id) override;
    void Msg_ButtonClick(unsigned ctrl_id) override;
    void Msg_OptionGroupChange(unsigned ctrl_id, unsigned selection) override;

    void CI_Error(ClientError ce) override;
    void CI_NextConnectState(ConnectState cs) override;
};
