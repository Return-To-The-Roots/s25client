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
#ifndef dskLOBBY_H_INCLUDED
#define dskLOBBY_H_INCLUDED

#pragma once

#include "desktops/dskMenuBase.h"

#include "network/ClientInterface.h"
#include "liblobby/LobbyInterface.h"

class iwLobbyRanking;
class iwLobbyServerInfo;
class iwDirectIPCreate;
class LobbyServerList;
class LobbyPlayerList;

class dskLobby final : public dskMenuBase, public ClientInterface, public LobbyInterface
{
private:
    iwLobbyServerInfo* serverInfoWnd;
    iwDirectIPCreate* createServerWnd;
    iwLobbyRanking* lobbyRankingWnd;

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
    void LC_RankingList(const LobbyPlayerList& players) override;

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

#endif // dskLOBBY_H_INCLUDED
