// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "desktops/dskMenuBase.h"
#include "liblobby/LobbyInterface.h"

class iwLobbyServerInfo;
class iwDirectIPCreate;
class LobbyServerList;
class LobbyPlayerList;

/// Check if we can connect to this server, i.e. the version is valid
bool isServerVersionValid(const LobbyServerInfo& serverInfo);

class dskLobby final : public dskMenuBase, public LobbyInterface
{
private:
    iwLobbyServerInfo* serverInfoWnd;
    iwDirectIPCreate* createServerWnd;

public:
    dskLobby();
    ~dskLobby();

    void updatePlayerList();
    void updateServerList();

    void msgWindowClosed(IngameWindow& wnd) override;

    void LC_Status_ConnectionLost() override;
    void LC_Status_IncompleteMessage() override;
    void LC_Status_Error(const std::string& error) override;

    void LC_Chat(const std::string& player, const std::string& text) override;
    void LC_ServerList(const LobbyServerList& servers) override;
    void LC_PlayerList(const LobbyPlayerList& players) override;
    void LC_ServerInfo(const LobbyServerInfo& info) override;

protected:
    void msgTimer(unsigned ctrl_id) override;
    void msgPaintBefore() override;
    void msgMsgBoxResult(unsigned msgbox_id, MsgboxResult mbr) override;
    void msgButtonClick(unsigned ctrl_id) override;
    void msgEditEnter(unsigned ctrl_id) override;
    void msgTableRightButton(unsigned ctrl_id, const std::optional<unsigned>& selection) override;
    void msgTableChooseItem(unsigned ctrl_id, unsigned selection) override;

    /**
     * Connects to the currently selected game and returns true on success
     */
    bool connectToSelectedGame();
};
