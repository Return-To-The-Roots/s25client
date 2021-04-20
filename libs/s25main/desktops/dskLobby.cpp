// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "dskLobby.h"
#include "Loader.h"
#include "RTTR_Version.h"
#include "Settings.h"
#include "WindowManager.h"
#include "controls/ctrlChat.h"
#include "controls/ctrlEdit.h"
#include "controls/ctrlTable.h"
#include "dskMultiPlayer.h"
#include "helpers/containerUtils.h"
#include "helpers/toString.h"
#include "ingameWindows/iwDirectIPConnect.h"
#include "ingameWindows/iwDirectIPCreate.h"
#include "ingameWindows/iwLobbyServerInfo.h"
#include "ingameWindows/iwMsgbox.h"
#include "network/GameClient.h"
#include "ogl/SoundEffectItem.h"
#include "liblobby/LobbyClient.h"
#include "s25util/Log.h"
#include "s25util/MyTime.h"
#include "s25util/colors.h"
#include <boost/lexical_cast.hpp>
#include <set>

namespace {
enum
{
    ID_btBack = dskMenuBase::ID_FIRST_FREE,
    ID_btConnect,
    ID_btAddServer,
    ID_tblGames,
    ID_tblPlayers,
    ID_Chat,
    ID_edtChat,
    ID_tmrUpdateGames
};
}

dskLobby::dskLobby() : dskMenuBase(LOADER.GetImageN("setup013", 0)), serverInfoWnd(nullptr), createServerWnd(nullptr)
{
    AddTextButton(ID_btBack, DrawPoint(530, 530), Extent(250, 22), TextureColor::Red1, _("Back"), NormalFont);
    AddTextButton(ID_btConnect, DrawPoint(530, 470), Extent(250, 22), TextureColor::Green2, _("Connect"), NormalFont);
    AddTextButton(ID_btAddServer, DrawPoint(530, 440), Extent(250, 22), TextureColor::Green2, _("Add Server"),
                  NormalFont);

    using SRT = ctrlTable::SortType;
    AddTable(ID_tblGames, DrawPoint(20, 20), Extent(500, 262), TextureColor::Grey, NormalFont,
             ctrlTable::Columns{{_("ID"), 0, SRT::Number},
                                {_("Server"), 300, SRT::String},
                                {_("Map"), 300, SRT::String},
                                {_("Player"), 200, SRT::String},
                                {_("Version"), 100, SRT::String},
                                {_("Ping"), 100, SRT::Number}});
    AddTable(ID_tblPlayers, DrawPoint(530, 20), Extent(250, 410), TextureColor::Grey, NormalFont,
             ctrlTable::Columns{{_("Name"), 12, SRT::String}, {_("Version"), 10, SRT::String}});

    // Chatfenster
    AddChatCtrl(ID_Chat, DrawPoint(20, 290), Extent(500, 238), TextureColor::Grey, NormalFont);
    // Chatfenster-Edit
    AddEdit(ID_edtChat, DrawPoint(20, 530), Extent(500, 22), TextureColor::Grey, NormalFont);

    using namespace std::chrono_literals;
    AddTimer(ID_tmrUpdateGames, 5s);

    // If we came from an active game, tell the server we quit
    if(LOBBYCLIENT.IsIngame())
        LOBBYCLIENT.SendLeaveServer();

    UpdateServerList();
    UpdatePlayerList();

    LOBBYCLIENT.AddListener(this);
    LOBBYCLIENT.SendServerListRequest();
    LOBBYCLIENT.SendPlayerListRequest();
}

dskLobby::~dskLobby()
{
    LOBBYCLIENT.RemoveListener(this);
}

void dskLobby::Msg_Timer(const unsigned /*ctrl_id*/)
{
    if(LOBBYCLIENT.IsLoggedIn())
        LOBBYCLIENT.SendServerListRequest();
}

void dskLobby::Msg_PaintBefore()
{
    dskMenuBase::Msg_PaintBefore();
    UpdateServerList();
    UpdatePlayerList();
    GetCtrl<ctrlEdit>(ID_edtChat)->SetFocus();
}

void dskLobby::Msg_MsgBoxResult(const unsigned msgbox_id, const MsgboxResult /*mbr*/)
{
    // Verbindung verloren
    if(msgbox_id == 0)
        WINDOWMANAGER.Switch(std::make_unique<dskMultiPlayer>());
}

void dskLobby::Msg_ButtonClick(const unsigned ctrl_id)
{
    switch(ctrl_id)
    {
        case ID_btBack:
        {
            LOBBYCLIENT.Stop();
            WINDOWMANAGER.Switch(std::make_unique<dskMultiPlayer>());
        }
        break;
        case ID_btConnect: ConnectToSelectedGame(); break;
        case ID_btAddServer:
        {
            if(SETTINGS.proxy.type != ProxyType::None)
                WINDOWMANAGER.Show(std::make_unique<iwMsgbox>(
                  _("Sorry!"),
                  _("You can't create a game while a proxy server is active\nDisable the use of a proxy server first!"),
                  this, MsgboxButton::Ok, MsgboxIcon::ExclamationGreen, 1));
            else
            {
                createServerWnd = &WINDOWMANAGER.ReplaceWindow(std::make_unique<iwDirectIPCreate>(ServerType::Lobby));
            }
        }
        break;
    }
}

void dskLobby::Msg_EditEnter(const unsigned ctrl_id)
{
    if(ctrl_id == ID_edtChat)
    {
        auto* edit = GetCtrl<ctrlEdit>(ID_edtChat);
        LOBBYCLIENT.SendChat(edit->GetText());
        edit->SetText("");
    }
}

void dskLobby::Msg_TableRightButton(const unsigned ctrl_id, const boost::optional<unsigned>& selection)
{
    if(!selection)
        return;
    auto* table = GetCtrl<ctrlTable>(ctrl_id);
    switch(ctrl_id)
    {
        case ID_tblGames: // Server list
        {
            const std::string& item = table->GetItemText(*selection, 0);

            if(boost::lexical_cast<unsigned>(item.c_str()) != 0)
            {
                if(serverInfoWnd)
                {
                    if(serverInfoWnd->GetServerId() == boost::lexical_cast<unsigned>(item))
                        return; // raus

                    WINDOWMANAGER.Close(serverInfoWnd);
                }

                serverInfoWnd = &WINDOWMANAGER.ReplaceWindow(
                  std::make_unique<iwLobbyServerInfo>(boost::lexical_cast<unsigned>(item)));
                serverInfoWnd->SetTitle(table->GetItemText(*selection, 1));
            }
        }
        break;
    }
}

void dskLobby::Msg_TableChooseItem(const unsigned ctrl_id, const unsigned /*selection*/)
{
    if(ctrl_id == ID_tblGames) // Server list
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
        serverInfoWnd = nullptr;
    else if(&wnd == createServerWnd)
        createServerWnd = nullptr;
}

bool dskLobby::ConnectToSelectedGame()
{
    const auto* table = GetCtrl<ctrlTable>(ID_tblGames);
    const auto& selectedRow = table->GetSelection();
    if(!selectedRow)
        return false;
    const auto selectedId = boost::lexical_cast<unsigned>(table->GetItemText(*selectedRow, 0));
    const auto serverList = LOBBYCLIENT.GetServerList();
    const auto itServer = helpers::find_if(
      serverList, [selectedId](const LobbyServerInfo& server) { return server.getId() == selectedId; });
    if(itServer == serverList.end())
    {
        WINDOWMANAGER.Show(std::make_unique<iwMsgbox>(_("Sorry!"), _("The selected game was not found anymore."), this,
                                                      MsgboxButton::Ok, MsgboxIcon::ExclamationRed, 1));
    } else
    {
        std::string serverRevision = itServer->getVersion();
        if(!serverRevision.empty() && serverRevision[0] == 'v')
            serverRevision = serverRevision.substr(std::string("v20001011 - ").size());
        if(serverRevision == RTTR_Version::GetShortRevision())
        {
            auto connect = std::make_unique<iwDirectIPConnect>(ServerType::Lobby);
            connect->Connect(itServer->getHost(), itServer->getPort(), false, itServer->hasPassword());
            WINDOWMANAGER.ReplaceWindow(std::move(connect));
            return true;
        } else
        {
            WINDOWMANAGER.Show(std::make_unique<iwMsgbox>(_("Sorry!"), _("You can't join that game with your version!"),
                                                          this, MsgboxButton::Ok, MsgboxIcon::ExclamationRed, 1));
        }
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
    WINDOWMANAGER.Show(std::make_unique<iwMsgbox>(_("Error"), _("Lost connection to lobby!"), this, MsgboxButton::Ok,
                                                  MsgboxIcon::ExclamationRed, 0));
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
                    WINDOWMANAGER.Show(std::make_unique<iwMsgbox>("LobbyBot", text.substr(self.length() + 3), this,
                                                                  MsgboxButton::Ok, MsgboxIcon::ExclamationGreen, 2));
                } else if(text.substr(self.length() + 1, 2) == ", ")
                {
                    GetCtrl<ctrlChat>(ID_Chat)->AddMessage(time, player, playerColor, text.substr(self.length() + 3),
                                                           COLOR_YELLOW);
                }
            }

            return;
        }
    }

    GetCtrl<ctrlChat>(ID_Chat)->AddMessage(time, player, playerColor, text, COLOR_YELLOW);
}

void dskLobby::LC_ServerList(const LobbyServerList& servers)
{
    auto* servertable = GetCtrl<ctrlTable>(ID_tblGames);
    bool first = servertable->GetNumRows() == 0;

    const unsigned selection = servertable->GetSelection().value_or(0u);
    int sortColumn = servertable->GetSortColumn();
    if(sortColumn == -1)
        sortColumn = 0;
    const auto direction = servertable->GetSortDirection();
    servertable->DeleteAllItems();

    std::set<unsigned> ids;
    for(const LobbyServerInfo& server : servers)
    {
        if(server.getName().empty())
            continue;

        if(helpers::contains(ids, server.getId()))
        {
            LOG.write("Duplicate ID in serverlist detected: %u\n") % server.getId();
            continue;
        }
        ids.insert(server.getId());
        std::string id = helpers::toString(server.getId());
        std::string name = (server.hasPassword() ? "(pwd) " : "") + server.getName();
        std::string ping = helpers::toString(server.getPing());
        std::string player =
          helpers::toString(server.getCurPlayers()) + "/" + helpers::toString(server.getMaxPlayers());
        servertable->AddRow({id, name, server.getMap(), player, server.getVersion(), ping});
    }
    if(first)
        servertable->SortRows(0, TableSortDir::Ascending);
    else
        servertable->SortRows(sortColumn, direction);
    servertable->SetSelection(selection);
}

void dskLobby::LC_PlayerList(const LobbyPlayerList& players)
{
    auto* playertable = GetCtrl<ctrlTable>(ID_tblPlayers);
    bool first = playertable->GetNumRows() == 0;

    if((playertable->GetNumRows() > 0) && (playertable->GetNumRows() < players.size()))
    {
        LOADER.GetSoundN("sound", 114)->Play(255, false);
    }

    const unsigned selection = playertable->GetSelection().value_or(0u);
    int sortColumn = playertable->GetSortColumn();
    if(sortColumn == -1)
        sortColumn = 0;
    const auto direction = playertable->GetSortDirection();
    playertable->DeleteAllItems();

    for(const LobbyPlayerInfo& player : players)
    {
        if(player.getId() != 0xFFFFFFFF)
        {
            std::string name = player.getName();
            if(player.isIngame)
                name += _(" (playing)");
            playertable->AddRow({name, player.getVersion()});
        }
    }
    if(first)
        playertable->SortRows(0, TableSortDir::Ascending);
    else
        playertable->SortRows(sortColumn, direction);
    playertable->SetSelection(selection);
}

void dskLobby::LC_ServerInfo(const LobbyServerInfo&)
{
    if(serverInfoWnd)
        serverInfoWnd->UpdateServerInfo();
}
