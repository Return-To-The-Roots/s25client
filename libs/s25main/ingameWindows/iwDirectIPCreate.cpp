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

#include "rttrDefines.h" // IWYU pragma: keep
#include "iwDirectIPCreate.h"
#include "Loader.h"
#include "Settings.h"
#include "WindowManager.h"
#include "controls/ctrlButton.h"
#include "controls/ctrlEdit.h"
#include "controls/ctrlOptionGroup.h"
#include "controls/ctrlText.h"
#include "desktops/dskSelectMap.h"
#include "helpers/strUtils.h"
#include "network/CreateServerInfo.h"
#include "ogl/FontStyle.h"
#include "gameData/const_gui_ids.h"

iwDirectIPCreate::iwDirectIPCreate(ServerType server_type)
    : IngameWindow(CGI_DIRECTIPCREATE, IngameWindow::posLastOrCenter, Extent(300, 285), _("Create Game"), LOADER.GetImageN("resource", 41),
                   true, true),
      server_type(server_type)
{
    ctrlEdit *name, *port;

    // "Name des Spiels"
    AddText(0, DrawPoint(20, 30), _("Game's Name:"), COLOR_YELLOW, 0, NormalFont);
    name = AddEdit(1, DrawPoint(20, 45), Extent(260, 22), TC_GREEN2, NormalFont, 0, false, false, true);

    // "Server-Port"
    AddText(2, DrawPoint(20, 80), _("Server-Port:"), COLOR_YELLOW, 0, NormalFont);
    port = AddEdit(3, DrawPoint(20, 95), Extent(260, 22), TC_GREEN2, NormalFont, 0, false, false, true);

    // "Passwort"
    AddText(4, DrawPoint(20, 130), _("Password:"), COLOR_YELLOW, 0, NormalFont);
    AddEdit(5, DrawPoint(20, 145), Extent(260, 22), TC_GREEN2, NormalFont, 0, false, false, true);

    // ipv6 oder ipv4 benutzen
    AddText(11, DrawPoint(20, 185), _("Use IPv6:"), COLOR_YELLOW, 0, NormalFont);

    ctrlOptionGroup* ipv6 = AddOptionGroup(12, ctrlOptionGroup::CHECK);
    ipv6->AddTextButton(0, DrawPoint(120, 180), Extent(75, 22), TC_GREEN2, _("IPv4"), NormalFont);
    ipv6->AddTextButton(1, DrawPoint(205, 180), Extent(75, 22), TC_GREEN2, _("IPv6"), NormalFont);
    ipv6->SetSelection((SETTINGS.server.ipv6 ? 1 : 0));

    // Status
    AddText(6, DrawPoint(150, 215), "", COLOR_RED, FontStyle::CENTER, NormalFont);

    // "Starten"
    AddTextButton(7, DrawPoint(20, 240), Extent(125, 22), TC_GREEN2, _("Start"), NormalFont);

    // "Zurück"
    AddTextButton(8, DrawPoint(155, 240), Extent(125, 22), TC_RED1, _("Back"), NormalFont);

    name->SetText(SETTINGS.lobby.name + _("'s Game"));
    name->SetFocus();
    port->SetText(SETTINGS.server.localPort);
}

/**
 *  Statustext resetten
 */
void iwDirectIPCreate::Msg_EditChange(const unsigned /*ctrl_id*/)
{
    // Statustext resetten
    SetText("", COLOR_RED, true);
}

/**
 *  Bei Enter nächstes Steuerelement auswählen
 */
void iwDirectIPCreate::Msg_EditEnter(const unsigned ctrl_id)
{
    ctrlEdit* name = GetCtrl<ctrlEdit>(1);
    ctrlEdit* port = GetCtrl<ctrlEdit>(3);
    ctrlEdit* pass = GetCtrl<ctrlEdit>(5);

    switch(ctrl_id)
    {
        case 1:
        {
            name->SetFocus(false);
            port->SetFocus(true);
            pass->SetFocus(false);
        }
        break;
        case 3:
        {
            name->SetFocus(false);
            port->SetFocus(false);
            pass->SetFocus(false);
        }
        break;
        case 5:
        {
            // Starten klicken
            Msg_ButtonClick(7);
        }
        break;
    }
}

void iwDirectIPCreate::Msg_OptionGroupChange(const unsigned ctrl_id, const int selection)
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
 *  Button Clicki-Di-Bunti-Li
 */
void iwDirectIPCreate::Msg_ButtonClick(const unsigned ctrl_id)
{
    switch(ctrl_id)
    {
        case 7: // "Starten"
        {
            ctrlEdit* edtName = GetCtrl<ctrlEdit>(1);
            ctrlEdit* edtPort = GetCtrl<ctrlEdit>(3);
            ctrlEdit* edtPw = GetCtrl<ctrlEdit>(5);

            if(edtName->GetText().empty())
            {
                SetText(_("Please enter a name for the game"), COLOR_RED, false);
                edtName->SetFocus(true);
                edtPort->SetFocus(false);
                edtPw->SetFocus(false);
                break;
            }
            boost::optional<uint16_t> port = validate::checkPort(edtPort->GetText());
            if(!port)
            {
                SetText(_("Invalid port. The valid port-range is 1 to 65535!"), COLOR_RED, false);
                edtName->SetFocus(false);
                edtPort->SetFocus(true);
                edtPw->SetFocus(false);
                break;
            }

            CreateServerInfo csi(server_type, *port, edtName->GetText(), edtPw->GetText(), SETTINGS.server.ipv6,
                                 (SETTINGS.global.use_upnp == 1));

            // Map auswählen
            WINDOWMANAGER.Switch(new dskSelectMap(csi));
        }
        break;
        case 8: { Close();
        }
        break;
    }
}

/**
 *  Setzt den Text und Schriftfarbe vom Textfeld und den Status des
 *  Buttons.
 */
void iwDirectIPCreate::SetText(const std::string& text, unsigned color, bool button)
{
    // Text setzen
    GetCtrl<ctrlText>(6)->SetTextColor(color);
    GetCtrl<ctrlText>(6)->SetText(text);

    // Button (de)aktivieren
    GetCtrl<ctrlButton>(7)->SetEnabled(button);
}

/**
 *  Status: Benutzerdefinierter Fehler
 */
void iwDirectIPCreate::LC_Status_Error(const std::string& error)
{
    ctrlEdit* name = GetCtrl<ctrlEdit>(1);
    ctrlEdit* port = GetCtrl<ctrlEdit>(3);
    ctrlEdit* pass = GetCtrl<ctrlEdit>(5);

    name->SetFocus(true);
    port->SetFocus(false);
    pass->SetFocus(false);

    name->SetDisabled(false);
    port->SetDisabled(false);
    pass->SetDisabled(false);

    SetText(error, COLOR_RED, true);
}
