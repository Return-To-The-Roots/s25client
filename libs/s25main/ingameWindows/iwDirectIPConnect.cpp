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

#include "iwDirectIPConnect.h"
#include "Loader.h"
#include "RttrLobbyClient.hpp"
#include "Settings.h"
#include "WindowManager.h"
#include "controls/ctrlButton.h"
#include "controls/ctrlEdit.h"
#include "controls/ctrlOptionGroup.h"
#include "controls/ctrlText.h"
#include "desktops/dskHostGame.h"
#include "drivers/VideoDriverWrapper.h"
#include "network/GameClient.h"
#include "ogl/FontStyle.h"
#include "gameData/const_gui_ids.h"
#include "liblobby/LobbyClient.h"
#include "s25util/StringConversion.h"
#include "s25util/colors.h"

iwDirectIPConnect::iwDirectIPConnect(ServerType server_type)
    : IngameWindow(CGI_DIRECTIPCONNECT, IngameWindow::posLastOrCenter, Extent(300, 285), _("Join Game"), LOADER.GetImageN("resource", 41),
                   true),
      server_type(server_type)
{
    ctrlEdit *host, *port;

    // "IP - Adresse vom Host"
    AddText(0, DrawPoint(20, 30), _("IP Address of Host:"), COLOR_YELLOW, FontStyle{}, NormalFont);
    host = AddEdit(1, DrawPoint(20, 45), Extent(260, 22), TC_GREEN2, NormalFont, 0, false, (server_type != ServerType::DIRECT), true);

    // "Server-Port"
    AddText(2, DrawPoint(20, 80), _("Server-Port:"), COLOR_YELLOW, FontStyle{}, NormalFont);
    port = AddEdit(3, DrawPoint(20, 95), Extent(260, 22), TC_GREEN2, NormalFont, 0, false, (server_type != ServerType::DIRECT), true);

    // "Passwort (falls vorhanden)"
    AddText(4, DrawPoint(20, 130), _("Password (if needed):"), COLOR_YELLOW, FontStyle{}, NormalFont);
    AddEdit(5, DrawPoint(20, 145), Extent(260, 22), TC_GREEN2, NormalFont, 0, false, false, true);

    // ipv6 oder ipv4 benutzen
    AddText(11, DrawPoint(20, 185), _("Use IPv6:"), COLOR_YELLOW, FontStyle{}, NormalFont);

    ctrlOptionGroup* ipv6 = AddOptionGroup(12, ctrlOptionGroup::CHECK);
    ipv6->AddTextButton(0, DrawPoint(120, 180), Extent(75, 22), TC_GREEN2, _("IPv4"), NormalFont);
    ipv6->AddTextButton(1, DrawPoint(205, 180), Extent(75, 22), TC_GREEN2, _("IPv6"), NormalFont);
    ipv6->SetSelection((SETTINGS.server.ipv6 ? 1 : 0));

    // Status
    AddText(6, DrawPoint(150, 215), "", COLOR_RED, FontStyle::CENTER, NormalFont);

    // "Verbinden"
    AddTextButton(7, DrawPoint(20, 240), Extent(125, 22), TC_GREEN2, _("Connect"), NormalFont);

    // "Zurück"
    AddTextButton(8, DrawPoint(155, 240), Extent(125, 22), TC_RED1, _("Back"), NormalFont);

    host->SetFocus();
    host->SetText(SETTINGS.server.last_ip);
    port->SetText(SETTINGS.server.localPort);

    // Client unser Window geben, damit er uns benachrichtigen kann
    GAMECLIENT.SetInterface(this);
}

void iwDirectIPConnect::Msg_EditChange(const unsigned /*ctrl_id*/)
{
    // Statustext resetten
    SetStatus("", COLOR_RED);
}

void iwDirectIPConnect::Msg_EditEnter(const unsigned ctrl_id)
{
    switch(ctrl_id)
    {
        case 1:
        {
            auto* host = GetCtrl<ctrlEdit>(1);
            auto* port = GetCtrl<ctrlEdit>(3);
            auto* pass = GetCtrl<ctrlEdit>(5);
            host->SetFocus(false);
            port->SetFocus(true);
            pass->SetFocus(false);
        }
        break;
        case 3:
        {
            auto* host = GetCtrl<ctrlEdit>(1);
            auto* port = GetCtrl<ctrlEdit>(3);
            auto* pass = GetCtrl<ctrlEdit>(5);
            host->SetFocus(false);
            port->SetFocus(false);
            pass->SetFocus(true);
        }
        break;
        case 5: { Msg_ButtonClick(7);
        }
        break;
    }
}

void iwDirectIPConnect::Msg_ButtonClick(const unsigned ctrl_id)
{
    switch(ctrl_id)
    {
        case 7: // "Verbinden"
        {
            auto* edtHost = GetCtrl<ctrlEdit>(1);
            auto* edtPort = GetCtrl<ctrlEdit>(3);
            auto* edtPw = GetCtrl<ctrlEdit>(5);
            boost::optional<uint16_t> port = validate::checkPort(edtPort->GetText());
            if(!port)
            {
                SetStatus(_("Invalid port. The valid port-range is 1 to 65535!"), COLOR_RED);
                edtHost->SetFocus(false);
                edtPort->SetFocus(true);
                edtPw->SetFocus(false);
                break;
            }

            // einstellung speichern
            SETTINGS.server.last_ip = edtHost->GetText();

            // Text auf "Verbinde mit Host..." setzen und Button deaktivieren
            SetStatus(_("Connecting with Host..."), COLOR_RED);

            GAMECLIENT.Stop();
            if(!GAMECLIENT.Connect(edtHost->GetText(), edtPw->GetText(), server_type, *port, false, SETTINGS.server.ipv6))
            {
                // Text auf "Verbindung fehlgeschlagen" setzen und Button aktivieren
                SetStatus(_("Connection failed!"), COLOR_RED);
            } else
                GetCtrl<ctrlButton>(ctrl_id)->SetEnabled(false);
        }
        break;
        case 8: { Close();
        }
        break;
    }
}

void iwDirectIPConnect::Msg_OptionGroupChange(const unsigned ctrl_id, const unsigned selection)
{
    switch(ctrl_id)
    {
        case 12: // IPv6 Ja/Nein
        {
            SETTINGS.server.ipv6 = (selection == 1);
        }
        break;
    }
}

/**
 *  Setzt den Text und Schriftfarbe vom Textfeld und den Status des
 *  Buttons.
 */
void iwDirectIPConnect::SetStatus(const std::string& text, unsigned color)
{
    // Text setzen
    GetCtrl<ctrlText>(6)->SetTextColor(color);
    GetCtrl<ctrlText>(6)->SetText(text);
}

/**
 *  Setzt den Hostnamen im Editfeld.
 */
void iwDirectIPConnect::SetHost(const std::string& hostIp)
{
    auto* host = GetCtrl<ctrlEdit>(1);
    host->SetText(hostIp);
}

void iwDirectIPConnect::Connect(const std::string& hostOrIp, const unsigned short port, const bool isIPv6, const bool hasPwd)
{
    SetHost(hostOrIp);
    SetPort(port);
    GetCtrl<ctrlOptionGroup>(12)->SetSelection(isIPv6 ? 1 : 0, true);
    auto* btConnect = GetCtrl<ctrlButton>(7);
    VIDEODRIVER.SetMousePos(btConnect->GetDrawPos() + DrawPoint(btConnect->GetSize()) / 2);
    if(!hasPwd)
        Msg_ButtonClick(7);
}

/**
 *  Setzt den Port im Editfeld.
 */
void iwDirectIPConnect::SetPort(unsigned short port)
{
    GetCtrl<ctrlEdit>(3)->SetText(s25util::toStringClassic(port));
}

void iwDirectIPConnect::CI_Error(const ClientError ce)
{
    SetStatus(ClientErrorToStr(ce), COLOR_RED);
    GetCtrl<ctrlButton>(7)->SetEnabled();
}

void iwDirectIPConnect::CI_NextConnectState(const ConnectState cs)
{
    switch(cs)
    {
        case CS_WAITFORANSWER: SetStatus(_("Waiting for Reply..."), COLOR_YELLOW); break;
        case CS_QUERYPW: SetStatus(_("Checking Password..."), COLOR_YELLOW); break;
        case CS_QUERYMAPNAME: SetStatus(_("Checking Map..."), COLOR_YELLOW); break;
        case CS_QUERYPLAYERLIST: SetStatus(_("Waiting for Playerinfo..."), COLOR_YELLOW); break;

        case CS_FINISHED: // Wir wurden verbunden
        {
            std::unique_ptr<ILobbyClient> lobbyClient;
            if(server_type == ServerType::LOBBY)
                lobbyClient = std::make_unique<RttrLobbyClient>(LOBBYCLIENT);
            WINDOWMANAGER.Switch(
              std::make_unique<dskHostGame>(server_type, GAMECLIENT.GetGameLobby(), GAMECLIENT.GetPlayerId(), std::move(lobbyClient)));
        }
        break;
        default: break;
    }
}
