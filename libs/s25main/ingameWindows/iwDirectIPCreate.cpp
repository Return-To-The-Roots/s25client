// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "iwDirectIPCreate.h"
#include "Loader.h"
#include "Settings.h"
#include "WindowManager.h"
#include "controls/ctrlButton.h"
#include "controls/ctrlEdit.h"
#include "controls/ctrlOptionGroup.h"
#include "controls/ctrlText.h"
#include "desktops/dskSelectMap.h"
#include "network/CreateServerInfo.h"
#include "ogl/FontStyle.h"
#include "gameData/const_gui_ids.h"

iwDirectIPCreate::iwDirectIPCreate(ServerType server_type)
    : IngameWindow(CGI_DIRECTIPCREATE, IngameWindow::posLastOrCenter, Extent(300, 285), _("Create Game"),
                   LOADER.GetImageN("resource", 41), true, CloseBehavior::Custom),
      server_type(server_type)
{
    ctrlEdit *name, *port;

    // "Name des Spiels"
    AddText(0, DrawPoint(20, 30), _("Game's Name:"), COLOR_YELLOW, FontStyle{}, NormalFont);
    name = AddEdit(1, DrawPoint(20, 45), Extent(260, 22), TextureColor::Green2, NormalFont, 0, false, false, true);

    // "Server-Port"
    AddText(2, DrawPoint(20, 80), _("Server-Port:"), COLOR_YELLOW, FontStyle{}, NormalFont);
    port = AddEdit(3, DrawPoint(20, 95), Extent(260, 22), TextureColor::Green2, NormalFont, 0, false, false, true);

    // "Passwort"
    AddText(4, DrawPoint(20, 130), _("Password:"), COLOR_YELLOW, FontStyle{}, NormalFont);
    AddEdit(5, DrawPoint(20, 145), Extent(260, 22), TextureColor::Green2, NormalFont, 0, false, false, true);

    // ipv6 oder ipv4 benutzen
    AddText(11, DrawPoint(20, 185), _("Use IPv6:"), COLOR_YELLOW, FontStyle{}, NormalFont);

    ctrlOptionGroup* ipv6 = AddOptionGroup(12, GroupSelectType::Check);
    ipv6->AddTextButton(0, DrawPoint(120, 180), Extent(75, 22), TextureColor::Green2, _("IPv4"), NormalFont);
    ipv6->AddTextButton(1, DrawPoint(205, 180), Extent(75, 22), TextureColor::Green2, _("IPv6"), NormalFont);
    ipv6->SetSelection((SETTINGS.server.ipv6 ? 1 : 0));

    // Status
    AddText(6, DrawPoint(150, 215), "", COLOR_RED, FontStyle::CENTER, NormalFont);

    // "Starten"
    AddTextButton(7, DrawPoint(20, 240), Extent(125, 22), TextureColor::Green2, _("Start"), NormalFont);

    // "Zurück"
    AddTextButton(8, DrawPoint(155, 240), Extent(125, 22), TextureColor::Red1, _("Back"), NormalFont);

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
    auto* name = GetCtrl<ctrlEdit>(1);
    auto* port = GetCtrl<ctrlEdit>(3);
    auto* pass = GetCtrl<ctrlEdit>(5);

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

void iwDirectIPCreate::Msg_OptionGroupChange(const unsigned ctrl_id, const unsigned selection)
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
            auto* edtName = GetCtrl<ctrlEdit>(1);
            auto* edtPort = GetCtrl<ctrlEdit>(3);
            auto* edtPw = GetCtrl<ctrlEdit>(5);

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
            WINDOWMANAGER.Switch(std::make_unique<dskSelectMap>(csi));
        }
        break;
        case 8:
        {
            Close();
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
    auto* name = GetCtrl<ctrlEdit>(1);
    auto* port = GetCtrl<ctrlEdit>(3);
    auto* pass = GetCtrl<ctrlEdit>(5);

    name->SetFocus(true);
    port->SetFocus(false);
    pass->SetFocus(false);

    name->SetDisabled(false);
    port->SetDisabled(false);
    pass->SetDisabled(false);

    SetText(error, COLOR_RED, true);
}
