// Copyright (C) 2005 - 2026 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "iwDirectIPConnect.h"
#include "Loader.h"
#include "RttrLobbyClient.hpp"
#include "Settings.h"
#include "WindowManager.h"
#include "controls/ctrlButton.h"
#include "controls/ctrlEdit.h"
#include "controls/ctrlOptionGroup.h"
#include "controls/ctrlText.h"
#include "drivers/VideoDriverWrapper.h"
#include "iwConnecting.h"
#include "network/GameClient.h"
#include "ogl/FontStyle.h"
#include "gameData/const_gui_ids.h"
#include "liblobby/LobbyClient.h"
#include "s25util/StringConversion.h"
#include "s25util/colors.h"

namespace {
enum : unsigned
{
    ID_txtIp,
    ID_edtIp,
    ID_txtPort,
    ID_edtPort,
    ID_txtPw,
    ID_edtPw,
    ID_txtIpv6,
    ID_grpIpv6,
    ID_txtStatus,
    ID_btConnect,
    ID_btBack,
};
}

iwDirectIPConnect::iwDirectIPConnect(ServerType serverType)
    : IngameWindow(CGI_DIRECTIPCONNECT, IngameWindow::posLastOrCenter, Extent(300, 285), _("Join Game"),
                   LOADER.getImageN("resource", 41), true),
      serverType_(serverType)
{
    AddText(ID_txtIp, DrawPoint(20, 30), _("IP Address of Host:"), COLOR_YELLOW, FontStyle{}, NormalFont);
    ctrlEdit* host = AddEdit(ID_edtIp, DrawPoint(20, 45), Extent(260, 22), TextureColor::Green2, NormalFont, 0, false,
                             (serverType != ServerType::Direct), true);

    AddText(ID_txtPort, DrawPoint(20, 80), _("Server-Port:"), COLOR_YELLOW, FontStyle{}, NormalFont);
    ctrlEdit* port = AddEdit(ID_edtPort, DrawPoint(20, 95), Extent(260, 22), TextureColor::Green2, NormalFont, 0, false,
                             (serverType != ServerType::Direct), true);

    AddText(ID_txtPw, DrawPoint(20, 130), _("Password (if needed):"), COLOR_YELLOW, FontStyle{}, NormalFont);
    AddEdit(ID_edtPw, DrawPoint(20, 145), Extent(260, 22), TextureColor::Green2, NormalFont, 0, false, false, true);

    AddText(ID_txtIpv6, DrawPoint(20, 185), _("Use IPv6:"), COLOR_YELLOW, FontStyle{}, NormalFont);

    ctrlOptionGroup* ipv6 = AddOptionGroup(ID_grpIpv6, GroupSelectType::Check);
    ipv6->AddTextButton(0, DrawPoint(120, 180), Extent(75, 22), TextureColor::Green2, _("IPv4"), NormalFont);
    ipv6->AddTextButton(1, DrawPoint(205, 180), Extent(75, 22), TextureColor::Green2, _("IPv6"), NormalFont);
    ipv6->setSelection((SETTINGS.server.ipv6 ? 1 : 0));

    AddText(ID_txtStatus, DrawPoint(150, 215), "", COLOR_RED, FontStyle::CENTER, NormalFont);
    AddTextButton(ID_btConnect, DrawPoint(20, 240), Extent(125, 22), TextureColor::Green2, _("Connect"), NormalFont);
    AddTextButton(ID_btBack, DrawPoint(155, 240), Extent(125, 22), TextureColor::Red1, _("Back"), NormalFont);

    host->setFocus();
    host->setText(SETTINGS.server.lastIP);
    port->setText(SETTINGS.server.localPort);
}

void iwDirectIPConnect::msgEditChange(const unsigned /*ctrl_id*/)
{
    // Reset status
    setStatus("", COLOR_RED);
}

void iwDirectIPConnect::msgEditEnter(const unsigned ctrl_id)
{
    switch(ctrl_id)
    {
        case ID_edtIp: getCtrl<ctrlEdit>(ID_edtPort)->setFocus(true); break;
        case ID_edtPort: getCtrl<ctrlEdit>(ID_edtPw)->setFocus(true); break;
        case ID_edtPw: getCtrl<ctrlEdit>(ID_btConnect)->setFocus(true); break;
    }
}

void iwDirectIPConnect::msgButtonClick(const unsigned ctrl_id)
{
    switch(ctrl_id)
    {
        case ID_btConnect:
        {
            auto* edtHost = getCtrl<ctrlEdit>(ID_edtIp);
            auto* edtPort = getCtrl<ctrlEdit>(ID_edtPort);
            auto* edtPw = getCtrl<ctrlEdit>(ID_edtPw);
            std::optional<uint16_t> port = validate::checkPort(edtPort->getText());
            if(!port)
            {
                setStatus(_("Invalid port. The valid port-range is 1 to 65535!"), COLOR_RED);
                edtHost->setFocus(false);
                edtPort->setFocus(true);
                edtPw->setFocus(false);
                break;
            }

            // save settings
            SETTINGS.server.lastIP = edtHost->getText();

            if(!GAMECLIENT.Connect(edtHost->getText(), edtPw->getText(), serverType_, *port, false,
                                   SETTINGS.server.ipv6))
            {
                setStatus(_("Connection failed!"), COLOR_RED);
            } else
            {
                setStatus(_("Connecting with Host..."), COLOR_YELLOW);
                getCtrl<ctrlButton>(ID_btConnect)->setEnabled(false);
                std::unique_ptr<ILobbyClient> lobbyClient;
                if(serverType_ == ServerType::Lobby)
                    lobbyClient = std::make_unique<RttrLobbyClient>(LOBBYCLIENT);
                iwConnecting& wnd =
                  WINDOWMANAGER.Show(std::make_unique<iwConnecting>(serverType_, std::move(lobbyClient)));
                onErrorConnection_ = wnd.onError.connect([this](ClientError error) {
                    setStatus(ClientErrorToStr(error), COLOR_RED);
                    this->getCtrl<ctrlButton>(ID_btConnect)->setEnabled();
                });
            }
        }
        break;
        case ID_btBack: Close(); break;
    }
}

void iwDirectIPConnect::msgOptionGroupChange(const unsigned ctrl_id, const unsigned selection)
{
    if(ctrl_id == ID_grpIpv6)
        SETTINGS.server.ipv6 = (selection == 1);
}

void iwDirectIPConnect::setStatus(const std::string& text, unsigned color)
{
    auto* txtStatus = getCtrl<ctrlText>(ID_txtStatus);
    txtStatus->setTextColor(color);
    txtStatus->setText(text);
}

void iwDirectIPConnect::Connect(const std::string& hostOrIp, const unsigned short port, const bool isIPv6,
                                const bool hasPwd)
{
    getCtrl<ctrlEdit>(ID_edtIp)->setText(hostOrIp);
    getCtrl<ctrlEdit>(ID_edtPort)->setText(s25util::toStringClassic(port));
    getCtrl<ctrlOptionGroup>(ID_grpIpv6)->setSelection(isIPv6 ? 1 : 0, true);
    auto* btConnect = getCtrl<ctrlButton>(ID_btConnect);
    VIDEODRIVER.setMousePos(btConnect->getDrawPos() + DrawPoint(btConnect->getSize()) / 2);
    if(hasPwd)
        getCtrl<ctrlEdit>(ID_edtPw)->setFocus(true);
    else
        msgButtonClick(ID_btConnect);
}
