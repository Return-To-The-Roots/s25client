// Copyright (c) 2005 - 2017 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "IngameWindow.h"

class iwLobbyServerInfo : public IngameWindow
{
private:
    unsigned serverId_;

public:
    iwLobbyServerInfo(unsigned serverId);

    void SetServerId(unsigned serverId);
    unsigned GetServerId() const { return serverId_; }
    void UpdateServerInfo();

protected:
    void Msg_Timer(unsigned ctrl_id) override;
};
