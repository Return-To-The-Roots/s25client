// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
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
                   LOADER.GetImageN("resource", 41), true),
      serverType_(serverType)
{
    AddText(ID_txtIp, DrawPoint(20, 30), _("IP Address of Host:"), COLOR_YELLOW, FontStyle{}, NormalFont);
    ctrlEdit* host = AddEdit(ID_edtIp, DrawPoint(20, 45), Extent(260, 22), TextureColor::Green2, NormalFont, 0, false,
                             (serverType != ServerType::Direct), true);

    AddText(ID_txtPort, DrawPoint(20, 80), _("Server-Port:"), COLOR_YELLOW, FontStyle{}, NormalFont);
    ctrlEdit* port = AddEdit(ID_edtPort, DrawPoint(20, 95), Extent(260, 22), TextureColor::Green2, NormalFont, 0, false,
                             (serverType != ServerType::Direct), true);

    AddText(ID_txtPw, DrawPoint(20, 130), _("Password (if needed):"), COLOR_YELLOW, FontStyle{}, NormalFont);
    AddEdit(ID_edtIp, DrawPoint(20, 145), Extent(260, 22), TextureColor::Green2, NormalFont, 0, false, false, true);

    AddText(ID_txtIpv6, DrawPoint(20, 185), _("Use IPv6:"), COLOR_YELLOW, FontStyle{}, NormalFont);

    ctrlOptionGroup* ipv6 = AddOptionGroup(ID_grpIpv6, GroupSelectType::Check);
    ipv6->AddTextButton(0, DrawPoint(120, 180), Extent(75, 22), TextureColor::Green2, _("IPv4"), NormalFont);
    ipv6->AddTextButton(1, DrawPoint(205, 180), Extent(75, 22), TextureColor::Green2, _("IPv6"), NormalFont);
    ipv6->SetSelection((SETTINGS.server.ipv6 ? 1 : 0));

    AddText(ID_txtStatus, DrawPoint(150, 215), "", COLOR_RED, FontStyle::CENTER, NormalFont);
    AddTextButton(ID_btConnect, DrawPoint(20, 240), Extent(125, 22), TextureColor::Green2, _("Connect"), NormalFont);
    AddTextButton(ID_btBack, DrawPoint(155, 240), Extent(125, 22), TextureColor::Red1, _("Back"), NormalFont);

    host->SetFocus();
    host->SetText(SETTINGS.server.last_ip);
    port->SetText(SETTINGS.server.localPort);
}

void iwDirectIPConnect::Msg_EditChange(const unsigned /*ctrl_id*/)
{
    // Reset status
    SetStatus("", COLOR_RED);
}

void iwDirectIPConnect::Msg_EditEnter(const unsigned ctrl_id)
{
    switch(ctrl_id)
    {
        case ID_edtIp: GetCtrl<ctrlEdit>(ID_edtPort)->SetFocus(true); break;
        case ID_edtPort: GetCtrl<ctrlEdit>(ID_edtPw)->SetFocus(true); break;
        case ID_edtPw: GetCtrl<ctrlEdit>(ID_btConnect)->SetFocus(true); break;
    }
}

void iwDirectIPConnect::Msg_ButtonClick(const unsigned ctrl_id)
{
    switch(ctrl_id)
    {
        case ID_btConnect:
        {
            auto* edtHost = GetCtrl<ctrlEdit>(ID_edtIp);
            auto* edtPort = GetCtrl<ctrlEdit>(ID_edtPort);
            auto* edtPw = GetCtrl<ctrlEdit>(ID_edtPw);
            boost::optional<uint16_t> port = validate::checkPort(edtPort->GetText());
            if(!port)
            {
                SetStatus(_("Invalid port. The valid port-range is 1 to 65535!"), COLOR_RED);
                edtHost->SetFocus(false);
                edtPort->SetFocus(true);
                edtPw->SetFocus(false);
                break;
            }

            // save settings
            SETTINGS.server.last_ip = edtHost->GetText();

            if(!GAMECLIENT.Connect(edtHost->GetText(), edtPw->GetText(), serverType_, *port, false,
                                   SETTINGS.server.ipv6))
            {
                SetStatus(_("Connection failed!"), COLOR_RED);
            } else
            {
                SetStatus(_("Connecting with Host..."), COLOR_RED);
                GetCtrl<ctrlButton>(ID_btConnect)->SetEnabled(false);
                std::unique_ptr<ILobbyClient> lobbyClient;
                if(serverType_ == ServerType::Lobby)
                    lobbyClient = std::make_unique<RttrLobbyClient>(LOBBYCLIENT);
                iwConnecting& wnd =
                  WINDOWMANAGER.Show(std::make_unique<iwConnecting>(serverType_, std::move(lobbyClient)));
                onErrorConnection_ = wnd.onError.connect([this](ClientError error) {
                    SetStatus(ClientErrorToStr(error), COLOR_RED);
                    this->GetCtrl<ctrlButton>(ID_btConnect)->SetEnabled();
                });
            }
        }
        break;
        case ID_btBack: Close(); break;
    }
}

void iwDirectIPConnect::Msg_OptionGroupChange(const unsigned ctrl_id, const unsigned selection)
{
    if(ctrl_id == ID_grpIpv6)
        SETTINGS.server.ipv6 = (selection == 1);
}

void iwDirectIPConnect::SetStatus(const std::string& text, unsigned color)
{
    auto* txtStatus = GetCtrl<ctrlText>(ID_txtStatus);
    txtStatus->SetTextColor(color);
    txtStatus->SetText(text);
}

/**
 *  Setzt den Hostnamen im Editfeld.
 */
void iwDirectIPConnect::SetHost(const std::string& hostIp)
{
    GetCtrl<ctrlEdit>(ID_edtIp)->SetText(hostIp);
}

void iwDirectIPConnect::Connect(const std::string& hostOrIp, const unsigned short port, const bool isIPv6,
                                const bool hasPwd)
{
    SetHost(hostOrIp);
    SetPort(port);
    GetCtrl<ctrlOptionGroup>(ID_grpIpv6)->SetSelection(isIPv6 ? 1 : 0, true);
    auto* btConnect = GetCtrl<ctrlButton>(ID_btConnect);
    VIDEODRIVER.SetMousePos(btConnect->GetDrawPos() + DrawPoint(btConnect->GetSize()) / 2);
    if(!hasPwd)
        Msg_ButtonClick(ID_btConnect);
}

/**
 *  Setzt den Port im Editfeld.
 */
void iwDirectIPConnect::SetPort(unsigned short port)
{
    GetCtrl<ctrlEdit>(ID_edtPort)->SetText(s25util::toStringClassic(port));
}
