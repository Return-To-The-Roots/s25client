// Copyright (c) 2005 - 2020 Settlers Freaks (sf-team at siedler25.org)
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

    SetTitle(serverinfo.getName());

    GetCtrl<ctrlEdit>(1)->SetText(serverinfo.getMap());
    GetCtrl<ctrlEdit>(4)->SetText(serverinfo.getName());
    GetCtrl<ctrlEdit>(6)->SetText(serverinfo.getHost() + ":" + helpers::toString(serverinfo.getPort()));
    GetCtrl<ctrlEdit>(8)->SetText(serverinfo.getVersion());
}

iwLobbyServerInfo::iwLobbyServerInfo(unsigned serverId)
    : IngameWindow(CGI_LOBBYSERVERINFO, IngameWindow::posLastOrCenter, Extent(260, 260), _("Server Information"),
                   LOADER.GetImageN("resource", 41)),
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

void iwLobbyServerInfo::Msg_Timer(const unsigned ctrl_id)
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
void iwLobbyServerInfo::SetServerId(unsigned serverId)
{
    serverId_ = serverId;
}
