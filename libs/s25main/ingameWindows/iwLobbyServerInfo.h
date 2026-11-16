// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
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

    void setServerId(unsigned serverId);
    unsigned getServerId() const { return serverId_; }
    void updateServerInfo();

protected:
    void msgTimer(unsigned ctrl_id) override;
};
