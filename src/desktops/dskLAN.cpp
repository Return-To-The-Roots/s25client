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
#include "dskLAN.h"
#include "RTTR_Version.h"

#include "Loader.h"
#include "Settings.h"
#include "WindowManager.h"
#include "controls/ctrlTable.h"
#include "desktops/dskMultiPlayer.h"
#include "ingameWindows/iwDirectIPConnect.h"
#include "ingameWindows/iwDirectIPCreate.h"
#include "ingameWindows/iwMsgbox.h"
#include "gameData/LanDiscoveryCfg.h"
#include "libutil/Serializer.h"
#include <boost/lexical_cast.hpp>

namespace {
enum
{
    ID_btBack = dskMenuBase::ID_FIRST_FREE,
    ID_btConnect,
    ID_btAddServer,
    ID_tblServer,
    ID_tmrRefreshServers,
    ID_tmrRefreshList
};
}

dskLAN::dskLAN() : dskMenuBase(LOADER.GetImageN("setup013", 0)), discovery(LAN_DISCOVERY_CFG)
{
    // "Server hinzufügen"
    AddTextButton(ID_btAddServer, DrawPoint(530, 250), Extent(250, 22), TC_GREEN2, _("Add Server"), NormalFont);
    // "Verbinden"
    AddTextButton(ID_btConnect, DrawPoint(530, 280), Extent(250, 22), TC_GREEN2, _("Connect"), NormalFont);
    // "Zurück"
    AddTextButton(ID_btBack, DrawPoint(530, 530), Extent(250, 22), TC_RED1, _("Back"), NormalFont);

    // Gameserver-Tabelle - "ID", "Server", "Karte", "Spieler", "Version"
    AddTable(ID_tblServer, DrawPoint(20, 20), Extent(500, 530), TC_GREY, NormalFont, 5, _("ID"), 0, ctrlTable::SRT_NUMBER, _("Server"), 300,
             ctrlTable::SRT_STRING, _("Map"), 300, ctrlTable::SRT_STRING, _("Player"), 200, ctrlTable::SRT_STRING, _("Version"), 100,
             ctrlTable::SRT_STRING);

    discovery.Start();

    AddTimer(ID_tmrRefreshServers, 60000); // Servers broadcast changes, so force a full update only once a minute
    AddTimer(ID_tmrRefreshList, 2000);
}

void dskLAN::Msg_Timer(const unsigned ctrl_id)
{
    if(ctrl_id == ID_tmrRefreshServers)
        discovery.Refresh();
    else if(ctrl_id == ID_tmrRefreshList)
        UpdateServerList();
    else
        RTTR_Assert(false);
}

void dskLAN::Msg_PaintBefore()
{
    dskMenuBase::Msg_PaintBefore();
    discovery.Run();
}

void dskLAN::Msg_ButtonClick(const unsigned ctrl_id)
{
    switch(ctrl_id)
    {
        case ID_btBack: WINDOWMANAGER.Switch(new dskMultiPlayer); break;
        case ID_btConnect: ConnectToSelectedGame(); break;
        case ID_btAddServer:
            if(SETTINGS.proxy.type != PROXY_NONE)
                WINDOWMANAGER.Show(new iwMsgbox(
                  _("Sorry!"), _("You can't create a game while a proxy server is active\nDisable the use of a proxy server first!"), this,
                  MSB_OK, MSB_EXCLAMATIONGREEN, 1));
            else
            {
                iwDirectIPCreate* servercreate = new iwDirectIPCreate(ServerType::LAN);
                WINDOWMANAGER.Show(servercreate, true);
            }
    }
}

void dskLAN::Msg_TableChooseItem(const unsigned ctrl_id, const unsigned selection)
{
    if(ctrl_id == ID_tblServer && selection != 0xFFFF) // Server list
        ConnectToSelectedGame();
}

void dskLAN::ReadOpenGames()
{
    openGames.clear();
    const LANDiscoveryClient::ServiceMap& services = discovery.GetServices();
    for(LANDiscoveryClient::ServiceMap::const_iterator it = services.begin(); it != services.end(); ++it)
    {
        Serializer ser(&it->second.info.GetPayload().front(), it->second.info.GetPayload().size()); //-V807
        GameInfo info;
        info.ip = it->second.ip;
        info.info.Deserialize(ser);
        openGames.push_back(info);
    }
}

void dskLAN::UpdateServerList()
{
    ReadOpenGames();

    ctrlTable* servertable = GetCtrl<ctrlTable>(ID_tblServer);

    unsigned selection = servertable->GetSelection();
    if(selection == 0xFFFF)
        selection = 0;
    unsigned short column = servertable->GetSortColumn();
    if(column == 0xFFFF)
        column = 0;
    bool direction = servertable->GetSortDirection();
    servertable->DeleteAllItems();

    unsigned curId = 0;
    for(std::vector<GameInfo>::const_iterator it = openGames.begin(); it != openGames.end(); ++it)
    {
        std::string id = boost::lexical_cast<std::string>(curId++);
        std::string name = (it->info.hasPwd ? "(pwd) " : "") + it->info.name; //-V807
        std::string player = boost::lexical_cast<std::string>(static_cast<unsigned>(it->info.curNumPlayers)) + "/"
                             + boost::lexical_cast<std::string>(static_cast<unsigned>(it->info.maxNumPlayers));
        servertable->AddRow(0, id.c_str(), name.c_str(), it->info.map.c_str(), player.c_str(), it->info.version.c_str());
    }

    servertable->SortRows(column, &direction);
    servertable->SetSelection(selection);
}

bool dskLAN::ConnectToSelectedGame()
{
    if(openGames.empty())
        return false;

    ctrlTable* table = GetCtrl<ctrlTable>(ID_tblServer);
    unsigned selection = boost::lexical_cast<unsigned>(table->GetItemText(table->GetSelection(), 0).c_str());
    if(selection >= openGames.size())
        return false;

    GameInfo game = openGames[selection];
    if(game.info.revision == RTTR_Version::GetRevision())
    {
        iwDirectIPConnect* connect = new iwDirectIPConnect(ServerType::LAN);
        connect->Connect(game.ip, game.info.port, game.info.isIPv6, game.info.hasPwd);
        WINDOWMANAGER.Show(connect);
        return true;
    } else
    {
        WINDOWMANAGER.Show(
          new iwMsgbox(_("Sorry!"), _("You can't join that game with your version!"), this, MSB_OK, MSB_EXCLAMATIONRED, 1));
        return false;
    }
}
