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

///////////////////////////////////////////////////////////////////////////////
// Header
#include "defines.h" // IWYU pragma: keep
#include <build_version.h>
#include "dskLobby.h"

#include "WindowManager.h"
#include "Loader.h"
#include "GameClient.h"
#include "LobbyClient.h"
#include "FileChecksum.h"
#include "Settings.h"

#include "dskHostGame.h"
#include "dskMultiPlayer.h"
#include "controls/ctrlChat.h"
#include "controls/ctrlEdit.h"
#include "controls/ctrlTable.h"
#include "ingameWindows/iwLobbyServerInfo.h"
#include "ingameWindows/iwLobbyRanking.h"
#include "ingameWindows/iwDirectIPCreate.h"
#include "ingameWindows/iwDirectIPConnect.h"
#include "ingameWindows/iwMsgbox.h"
#include "ogl/glArchivItem_Font.h"
#include "ogl/glArchivItem_Sound.h"

#include <Log.h>
#include <boost/lexical_cast.hpp>
#include <set>

// Include last!
#include "DebugNew.h" // IWYU pragma: keep

dskLobby::dskLobby() : Desktop(LOADER.GetImageN("setup013", 0)), serverinfo(NULL), servercreate(NULL)
{
    // Version
    AddVarText(0, 0, 600, _("Return To The Roots - v%s-%s"), COLOR_YELLOW, 0 | glArchivItem_Font::DF_BOTTOM, NormalFont, 2, GetWindowVersion(), GetWindowRevisionShort());
    // URL
    AddText(1, 400, 600, _("http://www.siedler25.org"), COLOR_GREEN, glArchivItem_Font::DF_CENTER | glArchivItem_Font::DF_BOTTOM, NormalFont);
    // Copyright
    AddVarText(2, 800, 600, _("© 2005 - %s Settlers Freaks"), COLOR_YELLOW, glArchivItem_Font::DF_RIGHT | glArchivItem_Font::DF_BOTTOM, NormalFont, 1, GetCurrentYear());

    // "Zurück"
    AddTextButton(3, 530, 530, 250, 22, TC_RED1, _("Back"), NormalFont);
    // "Verbinden"
    AddTextButton(4, 530, 470, 250, 22, TC_GREEN2, _("Connect"), NormalFont);
    // "Internet Ranking"
    AddTextButton(5, 530, 500, 250, 22, TC_GREEN2, _("Internet Ranking"), NormalFont);
    // "Server hinzufügen"
    AddTextButton(6, 530, 440, 250, 22, TC_GREEN2, _("Add Server"), NormalFont);

    // Gameserver-Tabelle - "ID", "Server", "Karte", "Spieler", "Version", "Ping"
    AddTable(10, 20, 20, 500, 262, TC_GREY, NormalFont, 6, _("ID"), 0, ctrlTable::SRT_NUMBER, _("Server"), 300, ctrlTable::SRT_STRING, _("Map"), 300, ctrlTable::SRT_STRING, _("Player"), 200, ctrlTable::SRT_STRING, _("Version"), 100, ctrlTable::SRT_STRING, _("Ping"), 100, ctrlTable::SRT_NUMBER);
    // Spieler-Tabelle - "Name", "Punkte", "Version"
    AddTable(11, 530, 20, 250, 410, TC_GREY, NormalFont, 3, _("Name"), 500, ctrlTable::SRT_STRING, _("Points"), 250, ctrlTable::SRT_STRING, _("Version"), 250, ctrlTable::SRT_STRING);

    // Chatfenster
    AddChatCtrl(20, 20, 290, 500, 238, TC_GREY, NormalFont);
    // Chatfenster-Edit
    AddEdit(21, 20, 530, 500, 22, TC_GREY, NormalFont);

    AddTimer(30, 5000);

    UpdateServerList(true);
    UpdatePlayerList(true);

    LOBBYCLIENT.SetInterface(this);
    LOBBYCLIENT.SendServerListRequest();
    LOBBYCLIENT.SendPlayerListRequest();

    GAMECLIENT.SetInterface(this);
}

void dskLobby::Msg_Timer(const unsigned int  /*ctrl_id*/)
{
    LOBBYCLIENT.SendServerListRequest();
}

void dskLobby::Msg_PaintBefore()
{
    UpdateServerList();
    UpdatePlayerList();
    GetCtrl<ctrlEdit>(21)->SetFocus();
}

void dskLobby::Msg_MsgBoxResult(const unsigned msgbox_id, const MsgboxResult  /*mbr*/)
{
    // Verbindung verloren
    if(msgbox_id == 0)
        WINDOWMANAGER.Switch(new dskMultiPlayer);
}

void dskLobby::Msg_ButtonClick(const unsigned int ctrl_id)
{
    switch(ctrl_id)
    {
        case 3: // Zurück
        {
            LOBBYCLIENT.Stop();
            WINDOWMANAGER.Switch(new dskMultiPlayer);
        } break;
        case 4: // Verbinden - Button
            ConnectToSelectedGame();
            break;
        case 5: // Ranking - Button
        {
            LOBBYCLIENT.SendRankingListRequest();
            WINDOWMANAGER.Show(new iwLobbyRanking, true);
        } break;
        case 6: // GameServer hinzufügen
        {
            if(SETTINGS.proxy.typ != 0)
                WINDOWMANAGER.Show(new iwMsgbox(_("Sorry!"), _("You can't create a game while a proxy server is active\nDisable the use of a proxy server first!"), this, MSB_OK, MSB_EXCLAMATIONGREEN, 1));
            else
            {
                servercreate = new iwDirectIPCreate(ServerType::LOBBY);
                servercreate->SetParent(this);
                WINDOWMANAGER.Show(servercreate, true);
            }
        } break;
    }
}

void dskLobby::Msg_EditEnter(const unsigned int ctrl_id)
{
    switch(ctrl_id)
    {
        case 21: // Chattext senden
        {
            ctrlEdit* edit = GetCtrl<ctrlEdit>(21);
            LOBBYCLIENT.SendChat(edit->GetText());
            edit->SetText("");
        } break;
    }
}

void dskLobby::Msg_TableRightButton(const unsigned int ctrl_id, const int selection)
{
    ctrlTable* table = GetCtrl<ctrlTable>(ctrl_id);
    switch(ctrl_id)
    {
        case 10: // Server list
        {
            const std::string item = table->GetItemText(selection, 0);

            if(atoi(item.c_str()) != 0)
            {
                if(serverinfo)
                {
                    if(serverinfo->GetNr() == (unsigned int)atoi(item.c_str()))
                        break; // raus

                    WINDOWMANAGER.Close(serverinfo);
                }

                serverinfo = new iwLobbyServerInfo();
                serverinfo->SetParent(this);
                serverinfo->Set(NULL, atoi(item.c_str()));
                serverinfo->SetTitle(table->GetItemText(selection, 1));

                LOBBYCLIENT.SendServerInfoRequest(atoi(item.c_str()));
                WINDOWMANAGER.Show(serverinfo, true);
            }
        } break;
    }
}

void dskLobby::Msg_TableChooseItem(const unsigned ctrl_id, const unsigned selection)
{
    if(ctrl_id == 10 && selection != 0xFFFF) // Server list
        ConnectToSelectedGame();
}

void dskLobby::UpdatePlayerList(bool first)
{
    playerlist = LOBBYCLIENT.GetPlayerList();
    if(!playerlist)
        return;

    ctrlTable* playertable = GetCtrl<ctrlTable>(11);

    if(!LOBBYCLIENT.refreshplayerlist)
        return;

    LOBBYCLIENT.refreshplayerlist = false;

    if ((playertable->GetRowCount() > 0) && (playertable->GetRowCount() < playerlist->getCount()))
    {
        LOADER.GetSoundN("sound", 114)->Play(255, false);
    }

    unsigned int selection = playertable->GetSelection();
    if(selection == 0xFFFF)
        selection = 0;
    unsigned short column = playertable->GetSortColumn();
    if(column == 0xFFFF)
        column = 0;
    bool direction = playertable->GetSortDirection();
    playertable->DeleteAllItems();

    if(playerlist->getCount() > 0)
    {
        for(LobbyPlayerList::const_iterator it = playerlist->begin(); it != playerlist->end(); ++it)
        {
            if(it->getId() != 0xFFFFFFFF)
            {
                std::string punkte = boost::lexical_cast<std::string>(it->getPunkte());
                playertable->AddRow(0, it->getName().c_str(), punkte.c_str(), it->getVersion().c_str());
            }
        }
        if(first)
            playertable->SortRows(0);
        else
            playertable->SortRows(column, &direction);
        playertable->SetSelection(selection);
    }
}

void dskLobby::UpdateServerList(bool first)
{
    serverlist = LOBBYCLIENT.GetServerList();
    if(!serverlist)
        return;

    ctrlTable* servertable = GetCtrl<ctrlTable>(10);

    if(!LOBBYCLIENT.refreshserverlist)
        return;

    LOBBYCLIENT.refreshserverlist = false;

    unsigned int selection = servertable->GetSelection();
    if(selection == 0xFFFF)
        selection = 0;
    unsigned short column = servertable->GetSortColumn();
    if(column == 0xFFFF)
        column = 0;
    bool direction = servertable->GetSortDirection();
    servertable->DeleteAllItems();

    if(serverlist->getCount() > 0)
    {
        std::set<unsigned> ids;
        for(LobbyServerList::const_iterator it = serverlist->begin(); it != serverlist->end(); ++it)
        {
            if(it->getName().empty())
                continue;

            if(helpers::contains(ids, it->getId()))
            {
                LOG.lprintf("Duplicate ID in serverlist detected: %u\n", it->getId());
                continue;
            }
            ids.insert(it->getId());
            std::string id = boost::lexical_cast<std::string>(it->getId());
            std::string name = (it->hasPassword() ? "(pwd) " : "") + it->getName();
            std::string ping = boost::lexical_cast<std::string>(it->getPing());
            std::string player = boost::lexical_cast<std::string>(it->getCurPlayers()) + "/" + boost::lexical_cast<std::string>(it->getMaxPlayers());
                servertable->AddRow(0, id.c_str(), name.c_str(), it->getMap().c_str(), player.c_str(), it->getVersion().c_str(), ping.c_str());
        }
        if(first)
            servertable->SortRows(0);
        else
            servertable->SortRows(column, &direction);
        servertable->SetSelection(selection);
    }
}

bool dskLobby::ConnectToSelectedGame()
{
    if(!serverlist)
        return false;

    ctrlTable* table = GetCtrl<ctrlTable>(10);
    unsigned int selection = table->GetSelection();
    if(selection >= serverlist->getCount())
        return false;

    selection = atoi(table->GetItemText(selection, 0).c_str());
    for(LobbyServerList::const_iterator it = serverlist->begin(); it != serverlist->end(); ++it)
    {
        if(it->getId() != selection)
            continue;

        if(it->getVersion() == std::string(GetWindowVersion()))
        {
            iwDirectIPConnect* connect = new iwDirectIPConnect(ServerType::LOBBY);
            connect->Connect(it->getHost(), it->getPort(), false, it->hasPassword());
            WINDOWMANAGER.Show(connect);
            return true;
        }
        else
            WINDOWMANAGER.Show(new iwMsgbox(_("Sorry!"), _("You can't join that game with your version!"), this, MSB_OK, MSB_EXCLAMATIONRED, 1));
        break;
    }
    return false;
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  Status: Verbindung verloren.
 *
 *  @author FloSoft
 */
void dskLobby::LC_Status_ConnectionLost()
{
    LC_Status_IncompleteMessage();
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  Status: fehlerhafte Anfrage / kaputte Daten.
 *
 *  @author FloSoft
 */
void dskLobby::LC_Status_IncompleteMessage()
{
    WINDOWMANAGER.Show(new iwMsgbox(_("Error"), _("Lost connection to lobby!"), this, MSB_OK, MSB_EXCLAMATIONRED, 0));
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  Status: Benutzerdefinierter Fehler
 *
 *  @author FloSoft
 */
void dskLobby::LC_Status_Error(const std::string& error)
{
    if(servercreate)
        servercreate->LC_Status_Error(error);
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  Status: Wir wurden erfolgreich mit einem Gameserver verbunden
 *
 *  @author FloSoft
 */
void dskLobby::LC_Connected()
{
    WINDOWMANAGER.Switch(new dskHostGame(ServerType::LOBBY));
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  Chatnachricht erhalten
 *
 *  @author FloSoft
 */
void dskLobby::LC_Chat(const std::string& player, const std::string& text)
{
    unsigned int checksum = CalcChecksumOfBuffer(player.c_str(), unsigned(player.length())) * player.length();
    unsigned int color = checksum | (checksum << 12) | 0xff000000;

    std::string time = TIME.FormatTime("(%H:%i:%s)");

    if (!player.compare("LobbyBot"))
    {
        std::string self = LOBBYCLIENT.GetUser();

        if ((text.length() > (self.length() + 3)) && text[0] == ' ')
        {
            if (text.substr(1, self.length()) == self)
            {
                if (text.substr(self.length() + 1, 2) == ": ")
                {
                    WINDOWMANAGER.Show(new iwMsgbox("LobbyBot", text.substr(self.length() + 3), this, MSB_OK, MSB_EXCLAMATIONGREEN, 2));
                }
                else if (text.substr(self.length() + 1, 2) == ", ")
                {
                    GetCtrl<ctrlChat>(20)->AddMessage(time, player, color, text.substr(self.length() + 3), COLOR_YELLOW);
                }
            }

            return;
        }
    }

    GetCtrl<ctrlChat>(20)->AddMessage(time, player, color, text, COLOR_YELLOW);
}


/// TODO!!

//case MSG_CLOSE: // child notification
//  {
//      switch(ctrl_id)
//      {
//      case CGI_LOBBYSERVERINFO: // pointer expired ;)
//          {
//              serverinfo = NULL;
//          } break;
//      case CGI_DIRECTIPCREATE:
//          {
//              servercreate = NULL;
//          } break;
//      }
//  } break;
