// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "iwLobbyServerInfo.h"
#include "Loader.h"
#include "controls/ctrlEdit.h"
#include "helpers/toString.h"
#include "gameData/const_gui_ids.h"
#include "liblobby/LobbyClient.h"
#include "s25util/colors.h"

/**
 *  aktualisiert das Serverinfo.
 */
void iwLobbyServerInfo::UpdateServerInfo()
{
    const LobbyServerInfo& serverinfo = LOBBYCLIENT.GetServerInfo();

    setTitle(serverinfo.getName());

    getCtrl<ctrlEdit>(1)->setText(serverinfo.getMap());
    getCtrl<ctrlEdit>(4)->setText(serverinfo.getName());
    getCtrl<ctrlEdit>(6)->setText(serverinfo.getHost() + ":" + helpers::toString(serverinfo.getPort()));
    getCtrl<ctrlEdit>(8)->setText(serverinfo.getVersion());
}

iwLobbyServerInfo::iwLobbyServerInfo(unsigned serverId)
    : IngameWindow(CGI_LOBBYSERVERINFO, IngameWindow::posLastOrCenter, Extent(260, 260), _("Server Information"),
                   LOADER.getImageN("resource", 41)),
      serverId_(serverId)
{
    AddText(0, DrawPoint(30, 40), _("Map Name:"), COLOR_YELLOW, FontStyle{}, NormalFont);
    AddEdit(1, DrawPoint(30, 60), Extent(200, 22), TextureColor::Green2, NormalFont, 0, false, true);
    AddText(3, DrawPoint(30, 90), _("Server Name:"), COLOR_YELLOW, FontStyle{}, NormalFont);
    AddEdit(4, DrawPoint(30, 110), Extent(200, 22), TextureColor::Green2, NormalFont, 0, false, true);
    AddText(5, DrawPoint(30, 140), _("Host:"), COLOR_YELLOW, FontStyle{}, NormalFont);
    AddEdit(6, DrawPoint(30, 160), Extent(200, 22), TextureColor::Green2, NormalFont, 0, false, true);
    AddText(7, DrawPoint(30, 190), _("Version:"), COLOR_YELLOW, FontStyle{}, NormalFont);
    AddEdit(8, DrawPoint(30, 210), Extent(200, 22), TextureColor::Green2, NormalFont, 0, false, true);

    LOBBYCLIENT.SendServerInfoRequest(serverId_);
    using namespace std::chrono_literals;
    AddTimer(9, 5s);
}

void iwLobbyServerInfo::msgTimer(const unsigned ctrl_id)
{
    switch(ctrl_id)
    {
        case 9: // alle 5 Sek
            LOBBYCLIENT.SendServerInfoRequest(serverId_);
            break;
    }
}

/**
 *  setzt Serverinfo und ServerNummer.
 */
void iwLobbyServerInfo::setServerId(unsigned serverId)
{
    serverId_ = serverId;
}
