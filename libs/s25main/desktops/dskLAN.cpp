// Copyright (C) 2005 - 2021 Settlers Freaks ( helpers::toString(gameInfo.info.curNumPlayers)
//
// SPDX-License-Identifier: GPL-2.0-or-later

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

dskLAN::dskLAN() : dskMenuBase(LOADER.getImageN("setup013", 0)), discovery(LAN_DISCOVERY_CFG)
{
    // "Server hinzufügen"
    addTextButton(ID_btAddServer, DrawPoint(530, 250), Extent(250, 22), TextureColor::Green2, _("Add Server"),
                  NormalFont);
    // "Verbinden"
    addTextButton(ID_btConnect, DrawPoint(530, 280), Extent(250, 22), TextureColor::Green2, _("Connect"), NormalFont);
    // "Zurück"
    addTextButton(ID_btBack, DrawPoint(530, 530), Extent(250, 22), TextureColor::Red1, _("Back"), NormalFont);

    // Gameserver-Tabelle - "ID", "Server", "Karte", "Spieler", "Version"
    using SRT = ctrlTable::SortType;
    addTable(ID_tblServer, DrawPoint(20, 20), Extent(500, 530), TextureColor::Grey, NormalFont,
             ctrlTable::Columns{{_("ID"), 0, SRT::Number},
                                {_("Server"), 300, SRT::String},
                                {_("Map"), 300, SRT::String},
                                {_("Player"), 200, SRT::String},
                                {_("Version"), 100, SRT::String}});

    discovery.Start();

    using namespace std::chrono_literals;
    addTimer(ID_tmrRefreshServers, 1min); // Servers broadcast changes, so force a full update only once a minute
    addTimer(ID_tmrRefreshList, 2s);
}

void dskLAN::msgTimer(const unsigned ctrl_id)
{
    if(ctrl_id == ID_tmrRefreshServers)
        discovery.Refresh();
    else if(ctrl_id == ID_tmrRefreshList)
        updateServerList();
    else
        RTTR_Assert(false);
}

void dskLAN::msgPaintBefore()
{
    dskMenuBase::msgPaintBefore();
    discovery.Run();
}

void dskLAN::msgButtonClick(const unsigned ctrl_id)
{
    switch(ctrl_id)
    {
        case ID_btBack: WINDOWMANAGER.switchTo(std::make_unique<dskMultiPlayer>()); break;
        case ID_btConnect: connectToSelectedGame(); break;
        case ID_btAddServer:
            if(SETTINGS.proxy.type != ProxyType::None)
                WINDOWMANAGER.show(std::make_unique<iwMsgbox>(
                  _("Sorry!"),
                  _("You can't create a game while a proxy server is active\nDisable the use of a proxy server first!"),
                  this, MsgboxButton::Ok, MsgboxIcon::ExclamationGreen, 1));
            else
            {
                WINDOWMANAGER.replaceWindow(std::make_unique<iwDirectIPCreate>(ServerType::LAN));
            }
    }
}

void dskLAN::msgTableChooseItem(const unsigned ctrl_id, const unsigned /*selection*/)
{
    if(ctrl_id == ID_tblServer)
        connectToSelectedGame();
}

void dskLAN::readOpenGames()
{
    openGames.clear();
    const LANDiscoveryClient::ServiceMap& services = discovery.GetServices();
    for(const auto& service : services)
    {
        Serializer ser(&service.second.info.GetPayload().front(), service.second.info.GetPayload().size()); //-V807
        GameInfo info;
        info.ip = service.second.ip;
        info.info.deserialize(ser);
        openGames.push_back(info);
    }
}

void dskLAN::updateServerList()
{
    readOpenGames();

    auto* servertable = getCtrl<ctrlTable>(ID_tblServer);

    const unsigned selection = servertable->getSelection().value_or(0u);
    int sortColumn = servertable->getSortColumn();
    if(sortColumn == -1)
        sortColumn = 0;
    const auto direction = servertable->getSortDirection();
    servertable->deleteAllItems();

    unsigned curId = 0;
    for(const auto& gameInfo : openGames)
    {
        std::string id = helpers::toString(curId++);
        std::string name = (gameInfo.info.hasPwd ? "(pwd) " : "") + gameInfo.info.name; //-V807
        std::string player =
          helpers::toString(gameInfo.info.curNumPlayers) + "/" + helpers::toString(gameInfo.info.maxNumPlayers);
        servertable->addRow({id, name, gameInfo.info.map, player, gameInfo.info.version});
    }

    if(sortColumn >= 0)
        servertable->sortRows(sortColumn, direction);
    servertable->setSelection(selection);
}

bool dskLAN::connectToSelectedGame()
{
    if(openGames.empty())
        return false;

    const auto* table = getCtrl<ctrlTable>(ID_tblServer);
    const auto& selectedRow = table->getSelection();
    if(!selectedRow)
        return false;
    const auto selectedId = boost::lexical_cast<unsigned>(table->getItemText(*selectedRow, 0));

    if(selectedId >= openGames.size())
        return false;

    const GameInfo& game = openGames[selectedId];
    if(game.info.revision == rttr::version::getRevision())
    {
        auto connect = std::make_unique<iwDirectIPConnect>(ServerType::LAN);
        connect->connect(game.ip, game.info.port, game.info.isIPv6, game.info.hasPwd);
        WINDOWMANAGER.replaceWindow(std::move(connect));
        return true;
    } else
    {
        WINDOWMANAGER.show(std::make_unique<iwMsgbox>(_("Sorry!"), _("You can't join that game with your version!"),
                                                      this, MsgboxButton::Ok, MsgboxIcon::ExclamationRed, 1));
        return false;
    }
}
