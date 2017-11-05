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

#include "rttrDefines.h" // IWYU pragma: keep
#include "dskLobby.h"
#include "GameClient.h"
#include "Loader.h"
#include "RTTR_Version.h"
#include "Settings.h"
#include "WindowManager.h"
#include "controls/ctrlChat.h"
#include "controls/ctrlEdit.h"
#include "controls/ctrlTable.h"
#include "dskHostGame.h"
#include "dskMultiPlayer.h"
#include "helpers/containerUtils.h"
#include "ingameWindows/iwDirectIPConnect.h"
#include "ingameWindows/iwDirectIPCreate.h"
#include "ingameWindows/iwLobbyRanking.h"
#include "ingameWindows/iwLobbyServerInfo.h"
#include "ingameWindows/iwMsgbox.h"
#include "ogl/SoundEffectItem.h"
#include "liblobby/LobbyClient.h"
#include "libutil/Log.h"
#include "libutil/MyTime.h"
#include "libutil/colors.h"
#include <boost/foreach.hpp>
#include <boost/lexical_cast.hpp>
#include <set>

dskLobby::dskLobby() : dskMenuBase(LOADER.GetImageN("setup013", 0)), serverInfoWnd(NULL), createServerWnd(NULL), lobbyRankingWnd(NULL)
{
    RTTR_Assert(dskMenuBase::ID_FIRST_FREE <= 3);

    // "Zurück"
    AddTextButton(3, DrawPoint(530, 530), Extent(250, 22), TC_RED1, _("Back"), NormalFont);
    // "Verbinden"
    AddTextButton(4, DrawPoint(530, 470), Extent(250, 22), TC_GREEN2, _("Connect"), NormalFont);
    // "Internet Ranking"
    AddTextButton(5, DrawPoint(530, 500), Extent(250, 22), TC_GREEN2, _("Internet Ranking"), NormalFont);
    // "Server hinzufügen"
    AddTextButton(6, DrawPoint(530, 440), Extent(250, 22), TC_GREEN2, _("Add Server"), NormalFont);

    // Gameserver-Tabelle - "ID", "Server", "Karte", "Spieler", "Version", "Ping"
    AddTable(10, DrawPoint(20, 20), Extent(500, 262), TC_GREY, NormalFont, 6, _("ID"), 0, ctrlTable::SRT_NUMBER, _("Server"), 300,
             ctrlTable::SRT_STRING, _("Map"), 300, ctrlTable::SRT_STRING, _("Player"), 200, ctrlTable::SRT_STRING, _("Version"), 100,
             ctrlTable::SRT_STRING, _("Ping"), 100, ctrlTable::SRT_NUMBER);
    // Spieler-Tabelle - "Name", "Punkte", "Version"
    AddTable(11, DrawPoint(530, 20), Extent(250, 410), TC_GREY, NormalFont, 3, _("Name"), 500, ctrlTable::SRT_STRING, _("Points"), 250,
             ctrlTable::SRT_STRING, _("Version"), 250, ctrlTable::SRT_STRING);

    // Chatfenster
    AddChatCtrl(20, DrawPoint(20, 290), Extent(500, 238), TC_GREY, NormalFont);
    // Chatfenster-Edit
    AddEdit(21, DrawPoint(20, 530), Extent(500, 22), TC_GREY, NormalFont);

    AddTimer(30, 5000);

    // If we came from an active game, tell the server we quit
    if(LOBBYCLIENT.IsIngame())
        LOBBYCLIENT.SendLeaveServer();

    UpdateServerList();
    UpdatePlayerList();

    LOBBYCLIENT.SetInterface(this);
    LOBBYCLIENT.SendServerListRequest();
    LOBBYCLIENT.SendPlayerListRequest();

    GAMECLIENT.SetInterface(this);
}

dskLobby::~dskLobby()
{
    LOBBYCLIENT.RemoveInterface(this);
    GAMECLIENT.RemoveInterface(this);
}

void dskLobby::Msg_Timer(const unsigned /*ctrl_id*/)
{
    LOBBYCLIENT.SendServerListRequest();
}

void dskLobby::Msg_PaintBefore()
{
    dskMenuBase::Msg_PaintBefore();
    UpdateServerList();
    UpdatePlayerList();
    GetCtrl<ctrlEdit>(21)->SetFocus();
}

void dskLobby::Msg_MsgBoxResult(const unsigned msgbox_id, const MsgboxResult /*mbr*/)
{
    // Verbindung verloren
    if(msgbox_id == 0)
        WINDOWMANAGER.Switch(new dskMultiPlayer);
}

void dskLobby::Msg_ButtonClick(const unsigned ctrl_id)
{
    switch(ctrl_id)
    {
        case 3: // Zurück
        {
            LOBBYCLIENT.Stop();
            WINDOWMANAGER.Switch(new dskMultiPlayer);
        }
        break;
        case 4: // Verbinden - Button
            ConnectToSelectedGame();
            break;
        case 5: // Ranking - Button
        {
            LOBBYCLIENT.SendRankingListRequest();
            WINDOWMANAGER.Show(lobbyRankingWnd = new iwLobbyRanking, true);
        }
        break;
        case 6: // GameServer hinzufügen
        {
            if(SETTINGS.proxy.typ != 0)
                WINDOWMANAGER.Show(new iwMsgbox(
                  _("Sorry!"), _("You can't create a game while a proxy server is active\nDisable the use of a proxy server first!"), this,
                  MSB_OK, MSB_EXCLAMATIONGREEN, 1));
            else
            {
                createServerWnd = new iwDirectIPCreate(ServerType::LOBBY);
                WINDOWMANAGER.Show(createServerWnd, true);
            }
        }
        break;
    }
}

void dskLobby::Msg_EditEnter(const unsigned ctrl_id)
{
    switch(ctrl_id)
    {
        case 21: // Chattext senden
        {
            ctrlEdit* edit = GetCtrl<ctrlEdit>(21);
            LOBBYCLIENT.SendChat(edit->GetText());
            edit->SetText("");
        }
        break;
    }
}

void dskLobby::Msg_TableRightButton(const unsigned ctrl_id, const int selection)
{
    ctrlTable* table = GetCtrl<ctrlTable>(ctrl_id);
    switch(ctrl_id)
    {
        case 10: // Server list
        {
            const std::string item = table->GetItemText(selection, 0);

            if(boost::lexical_cast<unsigned>(item.c_str()) != 0)
            {
                if(serverInfoWnd)
                {
                    if(serverInfoWnd->GetServerId() == boost::lexical_cast<unsigned>(item.c_str()))
                        return; // raus

                    WINDOWMANAGER.Close(serverInfoWnd);
                }

                serverInfoWnd = new iwLobbyServerInfo(boost::lexical_cast<unsigned>(item.c_str()));
                serverInfoWnd->SetTitle(table->GetItemText(selection, 1));
                WINDOWMANAGER.Show(serverInfoWnd, true);
            }
        }
        break;
    }
}

void dskLobby::Msg_TableChooseItem(const unsigned ctrl_id, const unsigned selection)
{
    if(ctrl_id == 10 && selection != 0xFFFF) // Server list
        ConnectToSelectedGame();
}

void dskLobby::UpdatePlayerList()
{
    if(LOBBYCLIENT.GetPlayerList().empty())
        return;
    LC_PlayerList(LOBBYCLIENT.GetPlayerList());
}

void dskLobby::UpdateServerList()
{
    if(LOBBYCLIENT.GetServerList().empty())
        return;
    LC_ServerList(LOBBYCLIENT.GetServerList());
}

void dskLobby::Msg_WindowClosed(IngameWindow& wnd)
{
    if(&wnd == serverInfoWnd)
        serverInfoWnd = NULL;
    else if(&wnd == createServerWnd)
        createServerWnd = NULL;
    else if(&wnd == lobbyRankingWnd)
        lobbyRankingWnd = NULL;
}

bool dskLobby::ConnectToSelectedGame()
{
    ctrlTable* table = GetCtrl<ctrlTable>(10);
    unsigned selection = boost::lexical_cast<unsigned>(table->GetItemText(table->GetSelection(), 0).c_str());
    BOOST_FOREACH(const LobbyServerInfo& server, LOBBYCLIENT.GetServerList())
    {
        if(server.getId() != selection)
            continue;

        std::string serverRevision = server.getVersion();
        if(!serverRevision.empty() && serverRevision[0] == 'v')
            serverRevision = serverRevision.substr(std::string("v20001011 - ").size());
        if(serverRevision == RTTR_Version::GetShortRevision())
        {
            iwDirectIPConnect* connect = new iwDirectIPConnect(ServerType::LOBBY);
            connect->Connect(server.getHost(), server.getPort(), false, server.hasPassword());
            WINDOWMANAGER.Show(connect);
            return true;
        } else
            WINDOWMANAGER.Show(
              new iwMsgbox(_("Sorry!"), _("You can't join that game with your version!"), this, MSB_OK, MSB_EXCLAMATIONRED, 1));
        break;
    }
    return false;
}

/**
 *  Status: Verbindung verloren.
 */
void dskLobby::LC_Status_ConnectionLost()
{
    LC_Status_IncompleteMessage();
}

/**
 *  Status: fehlerhafte Anfrage / kaputte Daten.
 */
void dskLobby::LC_Status_IncompleteMessage()
{
    WINDOWMANAGER.Show(new iwMsgbox(_("Error"), _("Lost connection to lobby!"), this, MSB_OK, MSB_EXCLAMATIONRED, 0));
}

/**
 *  Status: Benutzerdefinierter Fehler
 */
void dskLobby::LC_Status_Error(const std::string& error)
{
    if(createServerWnd)
        createServerWnd->LC_Status_Error(error);
}

/**
 *  Status: Wir wurden erfolgreich mit einem Gameserver verbunden
 */
void dskLobby::LC_Connected()
{
    WINDOWMANAGER.Switch(new dskHostGame(ServerType::LOBBY, GAMECLIENT.GetGameLobby(), GAMECLIENT.GetPlayerId()));
}

/**
 *  Chatnachricht erhalten
 */
void dskLobby::LC_Chat(const std::string& player, const std::string& text)
{
    unsigned playerColor = ctrlChat::CalcUniqueColor(player);

    std::string time = s25util::Time::FormatTime("(%H:%i:%s)");

    if(player == "LobbyBot")
    {
        std::string self = LOBBYCLIENT.GetUser();

        if((text.length() > (self.length() + 3)) && text[0] == ' ')
        {
            if(text.substr(1, self.length()) == self)
            {
                if(text.substr(self.length() + 1, 2) == ": ")
                {
                    WINDOWMANAGER.Show(new iwMsgbox("LobbyBot", text.substr(self.length() + 3), this, MSB_OK, MSB_EXCLAMATIONGREEN, 2));
                } else if(text.substr(self.length() + 1, 2) == ", ")
                {
                    GetCtrl<ctrlChat>(20)->AddMessage(time, player, playerColor, text.substr(self.length() + 3), COLOR_YELLOW);
                }
            }

            return;
        }
    }

    GetCtrl<ctrlChat>(20)->AddMessage(time, player, playerColor, text, COLOR_YELLOW);
}

void dskLobby::LC_ServerList(const LobbyServerList& servers)
{
    ctrlTable* servertable = GetCtrl<ctrlTable>(10);
    bool first = servertable->GetRowCount() == 0;

    unsigned selection = servertable->GetSelection();
    if(selection == 0xFFFF)
        selection = 0;
    unsigned short column = servertable->GetSortColumn();
    if(column == 0xFFFF)
        column = 0;
    bool direction = servertable->GetSortDirection();
    servertable->DeleteAllItems();

    std::set<unsigned> ids;
    BOOST_FOREACH(const LobbyServerInfo& server, servers)
    {
        if(server.getName().empty())
            continue;

        if(helpers::contains(ids, server.getId()))
        {
            LOG.write("Duplicate ID in serverlist detected: %u\n") % server.getId();
            continue;
        }
        ids.insert(server.getId());
        std::string id = boost::lexical_cast<std::string>(server.getId());
        std::string name = (server.hasPassword() ? "(pwd) " : "") + server.getName();
        std::string ping = boost::lexical_cast<std::string>(server.getPing());
        std::string player =
          boost::lexical_cast<std::string>(server.getCurPlayers()) + "/" + boost::lexical_cast<std::string>(server.getMaxPlayers());
        servertable->AddRow(0, id.c_str(), name.c_str(), server.getMap().c_str(), player.c_str(), server.getVersion().c_str(),
                            ping.c_str());
    }
    if(first)
        servertable->SortRows(0);
    else
        servertable->SortRows(column, &direction);
    servertable->SetSelection(selection);
}

void dskLobby::LC_PlayerList(const LobbyPlayerList& players)
{
    ctrlTable* playertable = GetCtrl<ctrlTable>(11);
    bool first = playertable->GetRowCount() == 0;

    if((playertable->GetRowCount() > 0) && (playertable->GetRowCount() < players.getCount()))
    {
        LOADER.GetSoundN("sound", 114)->Play(255, false);
    }

    unsigned selection = playertable->GetSelection();
    if(selection == 0xFFFF)
        selection = 0;
    unsigned short column = playertable->GetSortColumn();
    if(column == 0xFFFF)
        column = 0;
    bool direction = playertable->GetSortDirection();
    playertable->DeleteAllItems();

    BOOST_FOREACH(const LobbyPlayerInfo& player, players)
    {
        if(player.getId() != 0xFFFFFFFF)
        {
            std::string punkte = boost::lexical_cast<std::string>(player.getPunkte());
            std::string name = player.getName();
            if(player.isIngame)
                name += _(" (playing)");
            playertable->AddRow(0, name.c_str(), punkte.c_str(), player.getVersion().c_str());
        }
    }
    if(first)
        playertable->SortRows(0);
    else
        playertable->SortRows(column, &direction);
    playertable->SetSelection(selection);
}

void dskLobby::LC_ServerInfo(const LobbyServerInfo& info)
{
    if(serverInfoWnd)
        serverInfoWnd->UpdateServerInfo();
}

void dskLobby::LC_RankingList(const LobbyPlayerList& players)
{
    if(lobbyRankingWnd)
        lobbyRankingWnd->UpdateRankings(players);
}
