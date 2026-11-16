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
    void msgTimer(unsigned ctrl_id) override;
    void msgPaintBefore() override;
    void msgButtonClick(unsigned ctrl_id) override;
    void msgTableChooseItem(unsigned ctrl_id, unsigned selection) override;

    /**
     * Connects to the currently selected game and returns true on success
     */
    bool connectToSelectedGame();

private:
    LANDiscoveryClient discovery;
    std::vector<GameInfo> openGames;

    void updateServerList();
    void readOpenGames();
};
