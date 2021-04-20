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

class dskLobby final : public dskMenuBase, public LobbyInterface
{
private:
    iwLobbyServerInfo* serverInfoWnd;
    iwDirectIPCreate* createServerWnd;

public:
    dskLobby();
    ~dskLobby();

    void UpdatePlayerList();
    void UpdateServerList();

    void Msg_WindowClosed(IngameWindow& wnd) override;

    void LC_Status_ConnectionLost() override;
    void LC_Status_IncompleteMessage() override;
    void LC_Status_Error(const std::string& error) override;

    void LC_Chat(const std::string& player, const std::string& text) override;
    void LC_ServerList(const LobbyServerList& servers) override;
    void LC_PlayerList(const LobbyPlayerList& players) override;
    void LC_ServerInfo(const LobbyServerInfo& info) override;

protected:
    void Msg_Timer(unsigned ctrl_id) override;
    void Msg_PaintBefore() override;
    void Msg_MsgBoxResult(unsigned msgbox_id, MsgboxResult mbr) override;
    void Msg_ButtonClick(unsigned ctrl_id) override;
    void Msg_EditEnter(unsigned ctrl_id) override;
    void Msg_TableRightButton(unsigned ctrl_id, const boost::optional<unsigned>& selection) override;
    void Msg_TableChooseItem(unsigned ctrl_id, unsigned selection) override;

    /**
     * Connects to the currently selected game and returns true on success
     */
    bool ConnectToSelectedGame();
};
