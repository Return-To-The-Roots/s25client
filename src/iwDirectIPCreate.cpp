// $Id: iwDirectIPCreate.cpp 9357 2014-04-25 15:35:25Z FloSoft $
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
#include "iwDirectIPCreate.h"

#include "Loader.h"
#include "WindowManager.h"
#include "controls.h"
#include "GameProtocol.h"
#include "LobbyClient.h"
#include "Settings.h"

#include "dskSelectMap.h"

///////////////////////////////////////////////////////////////////////////////
// Makros / Defines
#if defined _WIN32 && defined _DEBUG && defined _MSC_VER
#define new new(_NORMAL_BLOCK, THIS_FILE, __LINE__)
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

///////////////////////////////////////////////////////////////////////////////
/**
 *  Konstruktor von @p iwDirectIPCreate.
 *
 *  @author OLiver
 */
iwDirectIPCreate::iwDirectIPCreate(unsigned int server_type)
    : IngameWindow(CGI_DIRECTIPCREATE, 0xFFFF, 0xFFFF, 300, 285, _("Create Game"), LOADER.GetImageN("resource", 41), true),
      server_type(server_type)
{
    ctrlEdit* name, *port;

    // "Name des Spiels"
    AddText(0, 20, 30, _("Game's Name:"), COLOR_YELLOW, 0, NormalFont);
    name = AddEdit(1, 20, 45, 260, 22, TC_GREEN2, NormalFont, 0, false, false, true);

    // "Server-Port"
    AddText(2, 20, 80, _("Server-Port:"), COLOR_YELLOW, 0, NormalFont);
    port = AddEdit(3, 20, 95, 260, 22, TC_GREEN2, NormalFont, 0, false, false,  true);

    // "Passwort"
    AddText(4, 20, 130, _("Password:"), COLOR_YELLOW, 0, NormalFont);
    AddEdit(5, 20, 145, 260, 22, TC_GREEN2, NormalFont, 0, false, false,  true);

    // ipv6 oder ipv4 benutzen
    AddText(11, 20, 185, _("Use IPv6:"), COLOR_YELLOW, 0, NormalFont);

    ctrlOptionGroup* ipv6 = AddOptionGroup(12, ctrlOptionGroup::CHECK);
    ipv6->AddTextButton(0, 120, 180, 75,    22, TC_GREEN2, _("IPv4"), NormalFont);
    ipv6->AddTextButton(1, 205, 180, 75,    22, TC_GREEN2, _("IPv6"), NormalFont);
    ipv6->SetSelection( (SETTINGS.server.ipv6 ? 1 : 0) );

    // Status
    AddText(6, 150, 215, "", COLOR_RED, glArchivItem_Font::DF_CENTER, NormalFont);

    // "Starten"
    AddTextButton(7, 20, 240, 125, 22, TC_GREEN2, _("Start"), NormalFont);

    // "Zurück"
    AddTextButton(8, 155, 240, 125, 22, TC_RED1, _("Back"), NormalFont);

    name->SetText(SETTINGS.lobby.name + _("'s Game"));
    name->SetFocus();
    port->SetText(LOADER.GetTextN("client", 3));
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  Statustext resetten
 *
 *  @author FloSoft
 */
void iwDirectIPCreate::Msg_EditChange(const unsigned int ctrl_id)
{
    // Statustext resetten
    SetText("", COLOR_RED, true);
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  Bei Enter nächstes Steuerelement auswählen
 *
 *  @author FloSoft
 */
void iwDirectIPCreate::Msg_EditEnter(const unsigned int ctrl_id)
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
        } break;
        case 3:
        {
            name->SetFocus(false);
            port->SetFocus(false);
            pass->SetFocus(false);
        } break;
        case 5:
        {
            // Starten klicken
            Msg_ButtonClick(7);
        } break;
    }
}

///////////////////////////////////////////////////////////////////////////////
/**
 *
 *
 *  @author FloSoft
 */
void iwDirectIPCreate::Msg_OptionGroupChange(const unsigned int ctrl_id, const unsigned short selection)
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
 *  Button Clicki-Di-Bunti-Li
 *
 *  @author FloSoft
 */
void iwDirectIPCreate::Msg_ButtonClick(const unsigned int ctrl_id)
{
    switch(ctrl_id)
    {
        case 7: // "Starten"
        {
            ctrlEdit* name = GetCtrl<ctrlEdit>(1);
            ctrlEdit* port = GetCtrl<ctrlEdit>(3);
            ctrlEdit* pass = GetCtrl<ctrlEdit>(5);

            if(name->GetText().length() < 1)
            {
                SetText(_("Please enter a name for the game"), COLOR_RED, false);
                name->SetFocus(true);
                port->SetFocus(false);
                pass->SetFocus(false);
                break;
            }
            if(atoi(port->GetText().c_str()) <= 0 || atoi(port->GetText().c_str()) >= 65535 || atoi(port->GetText().c_str()) == 3664)
            {
                SetText(_("Invalid port. The valid port-range is 1 to 65535!"), COLOR_RED, false);
                name->SetFocus(false);
                port->SetFocus(true);
                pass->SetFocus(false);
                break;
            }

            CreateServerInfo csi;
            csi.gamename = name->GetText();
            csi.password = pass->GetText();
            csi.port = static_cast<unsigned short>(atoi(port->GetText().c_str()));
            csi.type = server_type;
            csi.ipv6 = SETTINGS.server.ipv6;
            csi.use_upnp = (SETTINGS.global.use_upnp == 1);

            // Map auswählen
            WindowManager::inst().Switch(new dskSelectMap(csi));
        } break;
        case 8:
        {
            Close();
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
void iwDirectIPCreate::SetText(const std::string& text, unsigned int color, bool button)
{
    // Text setzen
    GetCtrl<ctrlText>(6)->SetColor(color);
    GetCtrl<ctrlText>(6)->SetText(text);

    // Button (de)aktivieren
    GetCtrl<ctrlButton>(7)->Enable(button);
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  Status: Benutzerdefinierter Fehler
 *
 *  @author FloSoft
 */
void iwDirectIPCreate::LC_Status_Error(std::string error)
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
