// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "Desktop.h"
#include "network/ClientInterface.h"
#include "gameData/GameLoader.h"
#include "liblobby/LobbyInterface.h"
#include <memory>

class dskGameInterface;

class dskGameLoader : public Desktop, public ClientInterface, public LobbyInterface
{
public:
    dskGameLoader(std::shared_ptr<Game> game);
    ~dskGameLoader() override;

    void LC_Status_Error(const std::string& error) override;
    void CI_GameStarted() override;
    void CI_Error(ClientError ce) override;

private:
    void Msg_MsgBoxResult(unsigned msgbox_id, MsgboxResult mbr) override;
    void Msg_Timer(unsigned ctrl_id) override;
    void ShowErrorMsg(const std::string& error);

    unsigned position;
    GameLoader loader_;
    std::unique_ptr<dskGameInterface> gameInterface;
};
