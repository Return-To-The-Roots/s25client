// Copyright (c) 2005 - 2015 Settlers Freaks (sf-team at siedler25.org)
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
#include "iwDirectIPConnect.h"
#include "controls/ctrlButton.h"
#include "controls/ctrlEdit.h"
#include "controls/ctrlOptionGroup.h"
#include "controls/ctrlText.h"
#include "Loader.h"
#include "GameClient.h"
#include "WindowManager.h"
#include "drivers/VideoDriverWrapper.h"
#include "desktops/dskHostGame.h"
#include "ogl/glArchivItem_Font.h"
#include "Settings.h"
#include "gameData/const_gui_ids.h"
#include "libutil/src/colors.h"

// Include last!
#include "DebugNew.h" // IWYU pragma: keep

iwDirectIPConnect::iwDirectIPConnect(ServerType server_type)
    : IngameWindow(CGI_DIRECTIPCONNECT, 0xFFFF, 0xFFFF, 300, 285, _("Join Game"), LOADER.GetImageN("resource", 41), true),
      server_type(server_type)
{
    ctrlEdit* host, *port;

    // "IP - Adresse vom Host"
    AddText(0, 20, 30, _("IP Address of Host:"), COLOR_YELLOW, 0, NormalFont);
    host = AddEdit(1, 20, 45, 260, 22, TC_GREEN2, NormalFont, 0, false, (server_type != ServerType::DIRECT),  true);

    // "Server-Port"
    AddText(2, 20, 80, _("Server-Port:"), COLOR_YELLOW, 0, NormalFont);
    port = AddEdit(3, 20, 95, 260, 22, TC_GREEN2, NormalFont, 0, false, (server_type != ServerType::DIRECT),  true);

    // "Passwort (falls vorhanden)"
    AddText(4, 20, 130, _("Password (if needed):"), COLOR_YELLOW, 0, NormalFont);
    AddEdit(5, 20, 145, 260, 22, TC_GREEN2, NormalFont, 0, false, false,  true);

    // ipv6 oder ipv4 benutzen
    AddText(11, 20, 185, _("Use IPv6:"), COLOR_YELLOW, 0, NormalFont);

    ctrlOptionGroup* ipv6 = AddOptionGroup(12, ctrlOptionGroup::CHECK);
    ipv6->AddTextButton(0, 120, 180, 75,    22, TC_GREEN2, _("IPv4"), NormalFont);
    ipv6->AddTextButton(1, 205, 180, 75,    22, TC_GREEN2, _("IPv6"), NormalFont);
    ipv6->SetSelection( (SETTINGS.server.ipv6 ? 1 : 0) );

    // Status
    AddText(6, 150, 215, EMPTY_STRING, COLOR_RED, glArchivItem_Font::DF_CENTER, NormalFont);

    // "Verbinden"
    AddTextButton(7, 20, 240, 125, 22, TC_GREEN2, _("Connect"), NormalFont);

    // "Zurück"
    AddTextButton(8, 155, 240, 125, 22, TC_RED1, _("Back"), NormalFont);

    host->SetFocus();
    host->SetText(SETTINGS.server.last_ip);
    port->SetText(LOADER.GetTextN("client", 3));

    // Client unser Window geben, damit er uns benachrichtigen kann
    GAMECLIENT.SetInterface(this);
}

void iwDirectIPConnect::Msg_EditChange(const unsigned int  /*ctrl_id*/)
{
    // Statustext resetten
    SetText(EMPTY_STRING, COLOR_RED, true);
}

void iwDirectIPConnect::Msg_EditEnter(const unsigned int ctrl_id)
{
    switch(ctrl_id)
    {
        case 1:
        {
            ctrlEdit* host = GetCtrl<ctrlEdit>(1);
            ctrlEdit* port = GetCtrl<ctrlEdit>(3);
            ctrlEdit* pass = GetCtrl<ctrlEdit>(5);
            host->SetFocus(false);
            port->SetFocus(true);
            pass->SetFocus(false);
        } break;
        case 3:
        {
            ctrlEdit* host = GetCtrl<ctrlEdit>(1);
            ctrlEdit* port = GetCtrl<ctrlEdit>(3);
            ctrlEdit* pass = GetCtrl<ctrlEdit>(5);
            host->SetFocus(false);
            port->SetFocus(false);
            pass->SetFocus(true);
        } break;
        case 5:
        {
            Msg_ButtonClick(7);
        } break;
    }
}

void iwDirectIPConnect::Msg_ButtonClick(const unsigned int ctrl_id)
{
    switch(ctrl_id)
    {
        case 7: // "Verbinden"
        {
            ctrlEdit* edtHost = GetCtrl<ctrlEdit>(1);
            ctrlEdit* edtPort = GetCtrl<ctrlEdit>(3);
            ctrlEdit* edtPw = GetCtrl<ctrlEdit>(5);

            int iPort = atoi(edtPort->GetText().c_str());
            if(iPort <= 0 || iPort >= 65535 || iPort == 3664)
            {
                SetText(_("Invalid port. The valid port-range is 1 to 65535!"), COLOR_RED, false);
                edtHost->SetFocus(false);
                edtPort->SetFocus(true);
                edtPw->SetFocus(false);
                break;
            }

            // einstellung speichern
            SETTINGS.server.last_ip = edtHost->GetText();

            // Text auf "Verbinde mit Host..." setzen und Button deaktivieren
            SetText( _("Connecting with Host..."), COLOR_RED, false);

            GAMECLIENT.Stop();
            if(!GAMECLIENT.Connect(edtHost->GetText(), edtPw->GetText(), server_type, static_cast<unsigned short>(iPort), false, SETTINGS.server.ipv6))
            {
                // Text auf "Verbindung fehlgeschlagen" setzen und Button aktivieren
                SetText( _("Connection failed!"), COLOR_RED, true);
            }

        } break;
        case 8:
        {
            Close();
        } break;
    }
}

void iwDirectIPConnect::Msg_OptionGroupChange(const unsigned int ctrl_id, const int selection)
{
    switch(ctrl_id)
    {
        case 12: // IPv6 Ja/Nein
        {
            SETTINGS.server.ipv6 = (selection == 1);
        } break;
    }
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  Setzt den Text und Schriftfarbe vom Textfeld und den Status des
 *  Buttons.
 *
 *  @author FloSoft
 */
void iwDirectIPConnect::SetText(const std::string& text, unsigned int color, bool button)
{
    // Text setzen
    GetCtrl<ctrlText>(6)->SetColor(color);
    GetCtrl<ctrlText>(6)->SetText(text);

    // Button (de)aktivieren
    GetCtrl<ctrlButton>(7)->Enable(button);
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  Setzt den Hostnamen im Editfeld.
 *
 *  @author FloSoft
 */
void iwDirectIPConnect::SetHost(const std::string& hostIp)
{
    ctrlEdit* host = GetCtrl<ctrlEdit>(1);
    host->SetText(hostIp);
}

void iwDirectIPConnect::Connect(const std::string& hostOrIp, const unsigned short port, const bool isIPv6, const bool hasPwd)
{
    SetHost(hostOrIp);
    SetPort(port);
    GetCtrl<ctrlOptionGroup>(12)->SetSelection(isIPv6 ? 1 : 0, true);
    ctrlTextButton* btConnect = GetCtrl<ctrlTextButton>(7);
    VIDEODRIVER.SetMousePos(btConnect->GetX() + btConnect->GetWidth() / 2, btConnect->GetY());
    if(!hasPwd)
        Msg_ButtonClick(7);
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  Setzt den Port im Editfeld.
 *
 *  @author FloSoft
 */
void iwDirectIPConnect::SetPort(unsigned short port)
{
    static char p[256];
    snprintf(p, 256, "%d", port);

    ctrlEdit* pp = GetCtrl<ctrlEdit>(3);
    pp->SetText(p);
}

void iwDirectIPConnect::CI_Error(const ClientError ce)
{
    switch(ce)
    {
        case CE_SERVERFULL:        SetText(_("This Server is full!"), COLOR_RED, true); break;
        case CE_WRONGPW:           SetText(_("Wrong Password!"), COLOR_RED, true); break;
        case CE_WRONGVERSION:      SetText(_("Wrong client version"), COLOR_RED, true); break;
        case CE_CONNECTIONLOST:    SetText(_("Connection to Host closed!"), COLOR_RED, true); break;
        case CE_INCOMPLETEMESSAGE: SetText(_("Too short Message received!"), COLOR_RED, true); break;
        case CE_INVALIDSERVERTYPE: SetText(_("Wrong Server Type!"), COLOR_RED, true); break;
        case CE_WRONGMAP:          SetText("", COLOR_RED, true); break;
        default:                   break;
    }
}

void iwDirectIPConnect::CI_NextConnectState(const ConnectState cs)
{
    switch(cs)
    {
        case CS_WAITFORANSWER:     SetText(_("Waiting for Reply..."), COLOR_YELLOW, true); break;
        case CS_QUERYPW:           SetText(_("Checking Password..."), COLOR_YELLOW, true); break;
        case CS_QUERYMAPNAME:      SetText(_("Checking Map..."), COLOR_YELLOW, true); break;
        case CS_QUERYPLAYERLIST:   SetText(_("Waiting for Playerinfo..."), COLOR_YELLOW, true); break;

        case CS_FINISHED: // Wir wurden verbunden
        {
            WINDOWMANAGER.Switch(new dskHostGame(server_type));
        } break;
        default: break;
    }
}
