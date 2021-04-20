// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "desktops/dskMenuBase.h"
#include "gameTypes/LanGameInfo.h"
#include "s25util/LANDiscoveryClient.h"
#include <vector>

class dskLAN : public dskMenuBase
{
public:
    struct GameInfo
    {
        std::string ip;
        LanGameInfo info;
    };
    dskLAN();

protected:
    void Msg_Timer(unsigned ctrl_id) override;
    void Msg_PaintBefore() override;
    void Msg_ButtonClick(unsigned ctrl_id) override;
    void Msg_TableChooseItem(unsigned ctrl_id, unsigned selection) override;

    /**
     * Connects to the currently selected game and returns true on success
     */
    bool ConnectToSelectedGame();

private:
    LANDiscoveryClient discovery;
    std::vector<GameInfo> openGames;

    void UpdateServerList();
    void ReadOpenGames();
};
