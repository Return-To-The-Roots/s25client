// $Id: dskLobby.cpp 9357 2014-04-25 15:35:25Z FloSoft $
//
// Copyright (c) 2005 - 2011 Settlers Freaks (sf-team at siedler25.org)
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
#include "main.h"
#include "dskLobby.h"

#include "WindowManager.h"
#include "Loader.h"
#include "VideoDriverWrapper.h"
#include "GameClient.h"
#include "LobbyClient.h"
#include "LobbyProtocol.h"
#include "controls.h"
#include "FileChecksum.h"
#include "Settings.h"

#include "dskHostGame.h"
#include "dskMultiPlayer.h"
#include "iwLobbyServerInfo.h"
#include "iwLobbyRanking.h"
#include "iwDirectIPCreate.h"
#include "iwDirectIPConnect.h"
#include "iwMsgbox.h"

///////////////////////////////////////////////////////////////////////////////
// Makros / Defines
#if defined _WIN32 && defined _DEBUG && defined _MSC_VER
#define new new(_NORMAL_BLOCK, THIS_FILE, __LINE__)
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

///////////////////////////////////////////////////////////////////////////////
/**
 *  Konstruktor von @p dskLobby.
 *
 *  @author FloSoft
 *  @author Devil
 */
dskLobby::dskLobby() : Desktop(LOADER.GetImageN("setup013", 0)), serverinfo(NULL), servercreate(NULL)
{
    // Version
    AddVarText(0, 0, 600, _("Return To The Roots - v%s-%s"), COLOR_YELLOW, 0 | glArchivItem_Font::DF_BOTTOM, NormalFont, 2, GetWindowVersion(), GetWindowRevision());
    // URL
    AddText(1, 400, 600, _("http://www.siedler25.org"), COLOR_GREEN, glArchivItem_Font::DF_CENTER | glArchivItem_Font::DF_BOTTOM, NormalFont);
    // Copyright
    AddText(2, 800, 600, _("\xA9 2005 - 2011 Settlers Freaks"), COLOR_YELLOW, glArchivItem_Font::DF_RIGHT | glArchivItem_Font::DF_BOTTOM, NormalFont);

    // "Zurück"
    AddTextButton(3, 530, 530, 250, 22, TC_RED1, _("Back"), NormalFont);
    // "Verbinden"
    AddTextButton(4, 530, 470, 250, 22, TC_GREEN2, _("Connect"), NormalFont);
    // "Internet Ranking"
    AddTextButton(5, 530, 500, 250, 22, TC_GREEN2, _("Internet Ranking"), NormalFont);
    // "Server hinzufügen"
    AddTextButton(6, 530, 440, 250, 22, TC_GREEN2, _("Add Server"), NormalFont);

    // Gameserver-Tabelle - "ID", "Server", "Karte", "Spieler", "Version", "Ping"
    AddTable(10, 20, 20, 500, 262, TC_GREY, NormalFont, 6, _("ID"), 0, ctrlTable::SRT_NUMBER, _("Server"), 300, ctrlTable::SRT_STRING, _("Map"), 300, ctrlTable::SRT_STRING, _("Player"), 200, ctrlTable::SRT_NUMBER, _("Version"), 100, ctrlTable::SRT_STRING, _("Ping"), 100, ctrlTable::SRT_NUMBER);
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

void dskLobby::Msg_Timer(const unsigned int ctrl_id)
{
    LOBBYCLIENT.SendServerListRequest();
}

void dskLobby::Msg_PaintBefore()
{
    UpdateServerList();
    UpdatePlayerList();
    GetCtrl<ctrlEdit>(21)->SetFocus();
}

void dskLobby::Msg_MsgBoxResult(const unsigned msgbox_id, const MsgboxResult mbr)
{
    // Verbindung verloren
    if(msgbox_id == 0)
        WindowManager::inst().Switch(new dskMultiPlayer);
}

void dskLobby::Msg_ButtonClick(const unsigned int ctrl_id)
{
    switch(ctrl_id)
    {
        case 3: // Zurück
        {
            LOBBYCLIENT.Stop();
            WindowManager::inst().Switch(new dskMultiPlayer);
        } break;
        case 4: // Verbinden - Button
        {
            if(serverlist)
            {
                ctrlTable* table = GetCtrl<ctrlTable>(10);

                unsigned int selection = table->GetSelection();
                if(selection < serverlist->getCount())
                {
                    selection = atoi(table->GetItemText(selection, 0).c_str());
                    for(unsigned int i = 0; i < serverlist->getCount(); ++i)
                    {
                        if(serverlist->getElement(i)->getId() == selection)
                        {
                            iwDirectIPConnect* connect = new iwDirectIPConnect(NP_LOBBY);
                            connect->SetHost(serverlist->getElement(i)->getHost().c_str());
                            connect->SetPort(serverlist->getElement(i)->getPort());
                            WindowManager::inst().Show(connect);
                            break;
                        }
                    }
                }
            }
        } break;
        case 5: // Ranking - Button
        {
            LOBBYCLIENT.SendRankingListRequest();
            WindowManager::inst().Show(new iwLobbyRanking, true);
        } break;
        case 6: // GameServer hinzufügen
        {
            if(SETTINGS.proxy.typ != 0)
                WindowManager::inst().Show(new iwMsgbox(_("Sorry!"), _("You can't create a game while a proxy server is active\nDisable the use of a proxy server first!"), this, MSB_OK, MSB_EXCLAMATIONGREEN, 1));
            else
            {
                servercreate = new iwDirectIPCreate(NP_LOBBY);
                servercreate->SetParent(this);
                WindowManager::inst().Show(servercreate, true);
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
            LOBBYCLIENT.SendChat(edit->GetText().c_str());
            edit->SetText("");
        } break;
    }
}

void dskLobby::Msg_TableRightButton(const unsigned int ctrl_id, const unsigned short selection)
{
    ctrlTable* table = GetCtrl<ctrlTable>(ctrl_id);
    switch(ctrl_id)
    {
        case 10:
        {
            const std::string item = table->GetItemText(table->GetSelection(false), 0);

            if(atoi(item.c_str()) != 0)
            {
                if(serverinfo)
                {
                    if(serverinfo->GetNr() == (unsigned int)atoi(item.c_str()))
                        break; // raus

                    WindowManager::inst().Close(serverinfo);
                }

                serverinfo = new iwLobbyServerInfo();
                serverinfo->SetParent(this);
                serverinfo->Set(NULL, atoi(item.c_str()));
                serverinfo->SetTitle(table->GetItemText(table->GetSelection(false), 1));

                LOBBYCLIENT.SendServerInfoRequest(atoi(item.c_str()));
                WindowManager::inst().Show(serverinfo, true);
            }
        } break;
    }
}

void dskLobby::UpdatePlayerList(bool first)
{
    playerlist = LOBBYCLIENT.GetPlayerList();
    if(!playerlist)
        return;

    ctrlTable* playertable = GetCtrl<ctrlTable>(11);

    if(LOBBYCLIENT.refreshplayerlist == true)
    {
        LOBBYCLIENT.refreshplayerlist = false;

        if ((playertable->GetRowCount() > 0) && (playertable->GetRowCount() < playerlist->getCount()))
        {
            LOADER.GetSoundN("sound", 114)->Play(255, false);
        }

        playertable->DeleteAllItems();

        if(playerlist->getCount() > 0)
        {
            unsigned int selection = playertable->GetSelection();
            if(selection == 0xFFFF)
                selection = 0;
            unsigned short column = playertable->GetSortColumn();
            if(column == 0xFFFF)
                column = 0;
            bool direction = !playertable->GetSortDirection();

            for(unsigned int i = 0; i < playerlist->getCount(); ++i)
            {
                if(playerlist->getElement(i)->getId() != 0xFFFFFFFF)
                {
                    char punkte[128];
                    snprintf(punkte, 128, "%d", playerlist->getElement(i)->getPunkte());
                    playertable->AddRow(0, playerlist->getElement(i)->getName().c_str(), punkte, playerlist->getElement(i)->getVersion().c_str());
                }
            }
            if(first)
                playertable->SortRows(0);
            else
                playertable->SortRows(column, &direction);
            playertable->SetSelection(selection);
        }
    }
}

void dskLobby::UpdateServerList(bool first)
{
    serverlist = LOBBYCLIENT.GetServerList();
    if(!serverlist)
        return;

    ctrlTable* servertable = GetCtrl<ctrlTable>(10);

    if(LOBBYCLIENT.refreshserverlist == true)
    {
        LOBBYCLIENT.refreshserverlist = false;

        servertable->DeleteAllItems();

        if(serverlist->getCount() > 0)
        {
            unsigned int selection = servertable->GetSelection();
            if(selection == 0xFFFF)
                selection = 0;
            unsigned short column = servertable->GetSortColumn();
            if(column == 0xFFFF)
                column = 0;
            bool direction = !servertable->GetSortDirection();

            for(unsigned int i = 0; i < serverlist->getCount(); ++i)
            {
                if(!serverlist->getElement(i)->getName().empty() && (serverlist->getElement(i)->getVersion() == std::string(GetWindowVersion())))
                {
                    char id[128];
                    char player[128];
                    char ping[128];
                    char name[128];
                    snprintf(id, 128, "%d", serverlist->getElement(i)->getId());
                    snprintf(name, 128, "%s%s", (serverlist->getElement(i)->hasPassword() ? "(pwd) " : ""), serverlist->getElement(i)->getName().c_str());
                    snprintf(player, 128, "%d/%d", serverlist->getElement(i)->getCurPlayers(), serverlist->getElement(i)->getMaxPlayers());
                    snprintf(ping, 128, "%d", serverlist->getElement(i)->getPing());
                    servertable->AddRow(0, id, name, serverlist->getElement(i)->getMap().c_str(), player, serverlist->getElement(i)->getVersion().c_str(), ping);
                }
            }
            if(first)
                servertable->SortRows(0);
            else
                servertable->SortRows(column, &direction);
            servertable->SetSelection(selection);
        }
    }
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
    WindowManager::inst().Show(new iwMsgbox(_("Error"), _("Lost connection to lobby!"), this, MSB_OK, MSB_EXCLAMATIONRED, 0));
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
void dskLobby::LC_Connected(void)
{
    WindowManager::inst().Switch(new dskHostGame);
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

    // Zeit holen
    char time_string[64];
    TIME.FormatTime(time_string, "(%H:%i:%s)", NULL);

    if (!player.compare("LobbyBot"))
    {
        std::string self = LOBBYCLIENT.GetUser();

        if ((text.length() > (self.length() + 3)) && !text.compare(0, 1, " "))
        {
            if (!text.compare(1, self.length(), self))
            {
                if (!text.compare(self.length() + 1, 2, ": "))
                {
                    WindowManager::inst().Show(new iwMsgbox("LobbyBot", text.substr(self.length() + 3), this, MSB_OK, MSB_EXCLAMATIONGREEN, 2));
                }
                else if (!text.compare(self.length() + 1, 2, ", "))
                {
                    GetCtrl<ctrlChat>(20)->AddMessage(time_string, player, color, text.substr(self.length() + 3), COLOR_YELLOW);
                }
            }

            return;
        }
    }

    GetCtrl<ctrlChat>(20)->AddMessage(time_string, player, color, text, COLOR_YELLOW);
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
