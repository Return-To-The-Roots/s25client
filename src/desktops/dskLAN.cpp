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
#include "defines.h" // IWYU pragma: keep
#include <build_version.h>
#include "dskLAN.h"

#include "WindowManager.h"
#include "Loader.h"
#include "Settings.h"
#include "ingameWindows/iwDirectIPCreate.h"
#include "ingameWindows/iwDirectIPConnect.h"
#include "ingameWindows/iwMsgbox.h"
#include "desktops/dskMultiPlayer.h"
#include "ogl/glArchivItem_Font.h"
#include "gameData/LanDiscoveryCfg.h"
#include "controls/ctrlTable.h"
#include <Serializer.h>
#include <boost/lexical_cast.hpp>

// Include last!
#include "DebugNew.h" // IWYU pragma: keep

namespace {
    const unsigned btBackId = 3;
    const unsigned btConnectId = 4;
    const unsigned btAddServerId = 5;
    const unsigned tblServerId = 6;
    const unsigned tmrRefreshServersId = 7;
    const unsigned tmrRefreshListId = 8;
}

dskLAN::dskLAN() : Desktop(LOADER.GetImageN("setup013", 0)), discovery(LAN_DISCOVERY_CFG)
{
    // Version
    AddVarText(0, 0, 600, _("Return To The Roots - v%s-%s"), COLOR_YELLOW, 0 | glArchivItem_Font::DF_BOTTOM, NormalFont, 2, GetWindowVersion(), GetWindowRevisionShort());
    // URL
    AddText(1, 400, 600, _("http://www.siedler25.org"), COLOR_GREEN, glArchivItem_Font::DF_CENTER | glArchivItem_Font::DF_BOTTOM, NormalFont);
    // Copyright
    AddVarText(2, 800, 600, _("© 2005 - %s Settlers Freaks"), COLOR_YELLOW, glArchivItem_Font::DF_RIGHT | glArchivItem_Font::DF_BOTTOM, NormalFont, 1, GetCurrentYear());

    // "Server hinzufügen"
    AddTextButton(btAddServerId, 530, 250, 250, 22, TC_GREEN2, _("Add Server"), NormalFont);
    // "Verbinden"
    AddTextButton(btConnectId, 530, 280, 250, 22, TC_GREEN2, _("Connect"), NormalFont);
    // "Zurück"
    AddTextButton(btBackId, 530, 530, 250, 22, TC_RED1, _("Back"), NormalFont);

    // Gameserver-Tabelle - "ID", "Server", "Karte", "Spieler", "Version"
    AddTable(tblServerId, 20, 20, 500, 530, TC_GREY, NormalFont, 5, _("ID"), 0, ctrlTable::SRT_NUMBER, _("Server"), 300, ctrlTable::SRT_STRING, _("Map"), 300, ctrlTable::SRT_STRING, _("Player"), 200, ctrlTable::SRT_STRING, _("Version"), 100, ctrlTable::SRT_STRING);

    discovery.Start();

    AddTimer(tmrRefreshServersId, 60000); // Servers broadcast changes, so force a full update only once a minute
    AddTimer(tmrRefreshListId, 2000);
}

void dskLAN::Msg_Timer(const unsigned int ctrl_id)
{
    if (ctrl_id == tmrRefreshServersId)
        discovery.Refresh();
    else if (ctrl_id == tmrRefreshListId)
        UpdateServerList();
    else
        RTTR_Assert(false);
}

void dskLAN::Msg_PaintBefore()
{
    discovery.Run();
}

void dskLAN::Msg_ButtonClick(const unsigned int ctrl_id)
{
    switch(ctrl_id)
    {
    case btBackId:
        WINDOWMANAGER.Switch(new dskMultiPlayer);
        break;
    case btConnectId:
        ConnectToSelectedGame();
        break;
    case btAddServerId:
        if(SETTINGS.proxy.typ != 0)
            WINDOWMANAGER.Show(new iwMsgbox(_("Sorry!"), _("You can't create a game while a proxy server is active\nDisable the use of a proxy server first!"), this, MSB_OK, MSB_EXCLAMATIONGREEN, 1));
        else
        {
            iwDirectIPCreate* servercreate = new iwDirectIPCreate(ServerType::LAN);
            servercreate->SetParent(this);
            WINDOWMANAGER.Show(servercreate, true);
        }
    }
}

void dskLAN::Msg_TableChooseItem(const unsigned ctrl_id, const unsigned selection)
{
    if(ctrl_id == tblServerId && selection != 0xFFFF) // Server list
        ConnectToSelectedGame();
}

void dskLAN::ReadOpenGames()
{
    openGames.clear();
    const LANDiscoveryClient::ServiceMap& services = discovery.GetServices();
    for (LANDiscoveryClient::ServiceMap::const_iterator it = services.begin(); it != services.end(); ++it)
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

    ctrlTable* servertable = GetCtrl<ctrlTable>(tblServerId);

    unsigned int selection = servertable->GetSelection();
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
        std::string player = boost::lexical_cast<std::string>(static_cast<unsigned>(it->info.curPlayer)) + "/"+
                             boost::lexical_cast<std::string>(static_cast<unsigned>(it->info.maxPlayer));
        servertable->AddRow(0, id.c_str(), name.c_str(), it->info.map.c_str(), player.c_str(), it->info.version.c_str());
    }
    
    servertable->SortRows(column, &direction);
    servertable->SetSelection(selection);
}

bool dskLAN::ConnectToSelectedGame()
{
    if(openGames.empty())
        return false;

    ctrlTable* table = GetCtrl<ctrlTable>(tblServerId);
    unsigned int selection = atoi(table->GetItemText(table->GetSelection(), 0).c_str());
    if (selection >= openGames.size())
        return false;

    GameInfo game = openGames[selection];
    if(game.info.version == std::string(GetWindowVersion()))
    {
        iwDirectIPConnect* connect = new iwDirectIPConnect(ServerType::LAN);
        connect->Connect(game.ip, game.info.port, game.info.isIPv6, game.info.hasPwd);
        WINDOWMANAGER.Show(connect);
        return true;
    }
    else
    {
        WINDOWMANAGER.Show(new iwMsgbox(_("Sorry!"), _("You can't join that game with your version!"), this, MSB_OK, MSB_EXCLAMATIONRED, 1));
        return false;
    }

}
