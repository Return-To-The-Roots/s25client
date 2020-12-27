// Copyright (c) 2005 - 2020 Settlers Freaks ( helpers::toString(gameInfo.info.curNumPlayers)
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

#include "dskLAN.h"
#include "Loader.h"
#include "RTTR_Version.h"
#include "Settings.h"
#include "WindowManager.h"
#include "controls/ctrlTable.h"
#include "desktops/dskMultiPlayer.h"
#include "helpers/toString.h"
#include "ingameWindows/iwDirectIPConnect.h"
#include "ingameWindows/iwDirectIPCreate.h"
#include "ingameWindows/iwMsgbox.h"
#include "gameData/LanDiscoveryCfg.h"
#include "s25util/Serializer.h"
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
    using SRT = ctrlTable::SortType;
    AddTable(ID_tblServer, DrawPoint(20, 20), Extent(500, 530), TC_GREY, NormalFont,
             ctrlTable::Columns{{_("ID"), 0, SRT::Number},
                                {_("Server"), 300, SRT::String},
                                {_("Map"), 300, SRT::String},
                                {_("Player"), 200, SRT::String},
                                {_("Version"), 100, SRT::String}});

    discovery.Start();

    using namespace std::chrono_literals;
    AddTimer(ID_tmrRefreshServers, 1min); // Servers broadcast changes, so force a full update only once a minute
    AddTimer(ID_tmrRefreshList, 2s);
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
        case ID_btBack: WINDOWMANAGER.Switch(std::make_unique<dskMultiPlayer>()); break;
        case ID_btConnect: ConnectToSelectedGame(); break;
        case ID_btAddServer:
            if(SETTINGS.proxy.type != ProxyType::None)
                WINDOWMANAGER.Show(std::make_unique<iwMsgbox>(
                  _("Sorry!"),
                  _("You can't create a game while a proxy server is active\nDisable the use of a proxy server first!"),
                  this, MSB_OK, MSB_EXCLAMATIONGREEN, 1));
            else
            {
                WINDOWMANAGER.ReplaceWindow(std::make_unique<iwDirectIPCreate>(ServerType::LAN));
            }
    }
}

void dskLAN::Msg_TableChooseItem(const unsigned ctrl_id, const unsigned /*selection*/)
{
    if(ctrl_id == ID_tblServer)
        ConnectToSelectedGame();
}

void dskLAN::ReadOpenGames()
{
    openGames.clear();
    const LANDiscoveryClient::ServiceMap& services = discovery.GetServices();
    for(const auto& service : services)
    {
        Serializer ser(&service.second.info.GetPayload().front(), service.second.info.GetPayload().size()); //-V807
        GameInfo info;
        info.ip = service.second.ip;
        info.info.Deserialize(ser);
        openGames.push_back(info);
    }
}

void dskLAN::UpdateServerList()
{
    ReadOpenGames();

    auto* servertable = GetCtrl<ctrlTable>(ID_tblServer);

    const unsigned selection = servertable->GetSelection().value_or(0u);
    int sortColumn = servertable->GetSortColumn();
    if(sortColumn == -1)
        sortColumn = 0;
    const auto direction = servertable->GetSortDirection();
    servertable->DeleteAllItems();

    unsigned curId = 0;
    for(const auto& gameInfo : openGames)
    {
        std::string id = helpers::toString(curId++);
        std::string name = (gameInfo.info.hasPwd ? "(pwd) " : "") + gameInfo.info.name; //-V807
        std::string player =
          helpers::toString(gameInfo.info.curNumPlayers) + "/" + helpers::toString(gameInfo.info.maxNumPlayers);
        servertable->AddRow({id, name, gameInfo.info.map, player, gameInfo.info.version});
    }

    if(sortColumn >= 0)
        servertable->SortRows(sortColumn, direction);
    servertable->SetSelection(selection);
}

bool dskLAN::ConnectToSelectedGame()
{
    if(openGames.empty())
        return false;

    const auto* table = GetCtrl<ctrlTable>(ID_tblServer);
    const auto& selectedRow = table->GetSelection();
    if(!selectedRow)
        return false;
    const auto selectedId = boost::lexical_cast<unsigned>(table->GetItemText(*selectedRow, 0));

    if(selectedId >= openGames.size())
        return false;

    const GameInfo& game = openGames[selectedId];
    if(game.info.revision == RTTR_Version::GetRevision())
    {
        auto connect = std::make_unique<iwDirectIPConnect>(ServerType::LAN);
        connect->Connect(game.ip, game.info.port, game.info.isIPv6, game.info.hasPwd);
        WINDOWMANAGER.ReplaceWindow(std::move(connect));
        return true;
    } else
    {
        WINDOWMANAGER.Show(std::make_unique<iwMsgbox>(_("Sorry!"), _("You can't join that game with your version!"),
                                                      this, MSB_OK, MSB_EXCLAMATIONRED, 1));
        return false;
    }
}
