// Copyright (C) 2005 - 2026 Settlers Freaks (sf-team at siedler25.org)
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
                   LOADER.getImageN("resource", 41), true, CloseBehavior::Custom),
      server_type(server_type)
{
    ctrlEdit *name, *port;

    // "Name des Spiels"
    addText(0, DrawPoint(20, 30), _("Game's Name:"), COLOR_YELLOW, FontStyle{}, NormalFont);
    name = addEdit(1, DrawPoint(20, 45), Extent(260, 22), TextureColor::Green2, NormalFont, 0, false, false, true);

    // "Server-Port"
    addText(2, DrawPoint(20, 80), _("Server-Port:"), COLOR_YELLOW, FontStyle{}, NormalFont);
    port = addEdit(3, DrawPoint(20, 95), Extent(260, 22), TextureColor::Green2, NormalFont, 0, false, false, true);

    // "Passwort"
    addText(4, DrawPoint(20, 130), _("Password:"), COLOR_YELLOW, FontStyle{}, NormalFont);
    addEdit(5, DrawPoint(20, 145), Extent(260, 22), TextureColor::Green2, NormalFont, 0, false, false, true);

    // ipv6 oder ipv4 benutzen
    addText(11, DrawPoint(20, 185), _("Use IPv6:"), COLOR_YELLOW, FontStyle{}, NormalFont);

    ctrlOptionGroup* ipv6 = addOptionGroup(12, GroupSelectType::Check);
    ipv6->addTextButton(0, DrawPoint(120, 180), Extent(75, 22), TextureColor::Green2, _("IPv4"), NormalFont);
    ipv6->addTextButton(1, DrawPoint(205, 180), Extent(75, 22), TextureColor::Green2, _("IPv6"), NormalFont);
    ipv6->setSelection((SETTINGS.server.ipv6 ? 1 : 0));

    // Status
    addText(6, DrawPoint(150, 215), "", COLOR_RED, FontStyle::CENTER, NormalFont);

    // "Starten"
    addTextButton(7, DrawPoint(20, 240), Extent(125, 22), TextureColor::Green2, _("Start"), NormalFont);

    // "Zurück"
    addTextButton(8, DrawPoint(155, 240), Extent(125, 22), TextureColor::Red1, _("Back"), NormalFont);

    name->setText(SETTINGS.lobby.name + _("'s Game"));
    name->setFocus();
    port->setText(SETTINGS.server.localPort);
}

/**
 *  Statustext resetten
 */
void iwDirectIPCreate::msgEditChange(const unsigned /*ctrl_id*/)
{
    // Statustext resetten
    setText("", COLOR_RED, true);
}

/**
 *  Bei Enter nächstes Steuerelement auswählen
 */
void iwDirectIPCreate::msgEditEnter(const unsigned ctrl_id)
{
    auto* name = getCtrl<ctrlEdit>(1);
    auto* port = getCtrl<ctrlEdit>(3);
    auto* pass = getCtrl<ctrlEdit>(5);

    switch(ctrl_id)
    {
        case 1:
        {
            name->setFocus(false);
            port->setFocus(true);
            pass->setFocus(false);
        }
        break;
        case 3:
        {
            name->setFocus(false);
            port->setFocus(false);
            pass->setFocus(false);
        }
        break;
        case 5:
        {
            // Starten klicken
            msgButtonClick(7);
        }
        break;
    }
}

void iwDirectIPCreate::msgOptionGroupChange(const unsigned ctrl_id, const unsigned selection)
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
void iwDirectIPCreate::msgButtonClick(const unsigned ctrl_id)
{
    switch(ctrl_id)
    {
        case 7: // "Starten"
        {
            auto* edtName = getCtrl<ctrlEdit>(1);
            auto* edtPort = getCtrl<ctrlEdit>(3);
            auto* edtPw = getCtrl<ctrlEdit>(5);

            if(edtName->getText().empty())
            {
                setText(_("Please enter a name for the game"), COLOR_RED, false);
                edtName->setFocus(true);
                edtPort->setFocus(false);
                edtPw->setFocus(false);
                break;
            }
            std::optional<uint16_t> port = validate::checkPort(edtPort->getText());
            if(!port)
            {
                setText(_("Invalid port. The valid port-range is 1 to 65535!"), COLOR_RED, false);
                edtName->setFocus(false);
                edtPort->setFocus(true);
                edtPw->setFocus(false);
                break;
            }

            CreateServerInfo csi(server_type, *port, edtName->getText(), edtPw->getText(), SETTINGS.server.ipv6,
                                 SETTINGS.global.useUPNP);

            // Map auswählen
            WINDOWMANAGER.switchTo(std::make_unique<dskSelectMap>(csi));
        }
        break;
        case 8:
        {
            close();
        }
        break;
    }
}

/**
 *  Setzt den Text und Schriftfarbe vom Textfeld und den Status des
 *  Buttons.
 */
void iwDirectIPCreate::setText(const std::string& text, unsigned color, bool button)
{
    // Text setzen
    getCtrl<ctrlText>(6)->setTextColor(color);
    getCtrl<ctrlText>(6)->setText(text);

    // Button (de)aktivieren
    getCtrl<ctrlButton>(7)->setEnabled(button);
}

/**
 *  Status: Benutzerdefinierter Fehler
 */
void iwDirectIPCreate::LC_Status_Error(const std::string& error)
{
    auto* name = getCtrl<ctrlEdit>(1);
    auto* port = getCtrl<ctrlEdit>(3);
    auto* pass = getCtrl<ctrlEdit>(5);

    name->setFocus(true);
    port->setFocus(false);
    pass->setFocus(false);

    name->setDisabled(false);
    port->setDisabled(false);
    pass->setDisabled(false);

    setText(error, COLOR_RED, true);
}
