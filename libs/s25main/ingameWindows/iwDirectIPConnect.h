// Copyright (c) 2005 - 2017 Settlers Freaks (sf-team at siedler25.org)
//
// This file is part of Return To The Roots.
//
// Return To The Roots is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 2 of the License, or
// (at your option) any later version.
//
// Return To The Roots is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with Return To The Roots. If not, see <http://www.gnu.org/licenses/>.

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
