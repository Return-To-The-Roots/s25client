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

#include "commonDefines.h"
#include "dskSelectMap.h"
#include "ListDir.h"
#include "Loader.h"
#include "RttrConfig.h"
#include "RttrLobbyClient.hpp"
#include "WindowManager.h"
#include "controls/ctrlButton.h"
#include "controls/ctrlOptionGroup.h"
#include "controls/ctrlPreviewMinimap.h"
#include "controls/ctrlTable.h"
#include "controls/ctrlText.h"
#include "desktops/dskDirectIP.h"
#include "desktops/dskHostGame.h"
#include "desktops/dskLAN.h"
#include "desktops/dskLobby.h"
#include "desktops/dskSinglePlayer.h"
#include "files.h"
#include "helpers/containerUtils.h"
#include "helpers/format.hpp"
#include "helpers/toString.h"
#include "ingameWindows/iwMapGenerator.h"
#include "ingameWindows/iwMsgbox.h"
#include "ingameWindows/iwPleaseWait.h"
#include "ingameWindows/iwSave.h"
#include "lua/GameDataLoader.h"
#include "mapGenerator/MapGenerator.h"
#include "network/GameClient.h"
#include "ogl/FontStyle.h"
#include "ogl/glArchivItem_Map.h"
#include "gameData/MapConsts.h"
#include "gameData/WorldDescription.h"
#include "liblobby/LobbyClient.h"
#include "libsiedler2/ArchivItem_Map_Header.h"
#include "libsiedler2/ErrorCodes.h"
#include "libsiedler2/prototypen.h"
#include "s25util/Log.h"
#include "s25util/dynamicUniqueCast.h"
#include "s25util/utf8.h"
#include <boost/filesystem/operations.hpp>
#include <stdexcept>
#include <utility>

namespace bfs = boost::filesystem;

/**
 *  Konstruktor von @p dskSelectMap.
 *
 *  @param[in] type Typ des Servers
 *  @param[in] name Server-Name
 *  @param[in] pass Server-Passwort
 */
dskSelectMap::dskSelectMap(CreateServerInfo csi)
    : Desktop(LOADER.GetImageN("setup015", 0)), csi(std::move(csi)), mapGenThread(nullptr), waitWnd(nullptr)
{
    WorldDescription desc;
    GameDataLoader gdLoader(desc);
    if(!gdLoader.Load())
    {
        LC_Status_Error(_("Failed to load game data!"));
        return;
    }

    for(DescIdx<LandscapeDesc> i(0); i.value < desc.landscapes.size(); i.value++)
        landscapeNames[desc.get(i).s2Id] = _(desc.get(i).name);

    // Die Tabelle für die Maps
    using SRT = ctrlTable::SortType;
    AddTable(1, DrawPoint(110, 35), Extent(680, 400), TC_GREY, NormalFont,
             ctrlTable::Columns{{_("Name"), 250, SRT::String},
                                {_("Author"), 216, SRT::String},
                                {_("Player"), 170, SRT::Number},
                                {_("Type"), 180, SRT::String},
                                {_("Size"), 134, SRT::MapSize},
                                {"", 0, SRT::Default}});

    // "Karten Auswahl"
    AddText(2, DrawPoint(400, 5), _("Selection of maps"), COLOR_YELLOW, FontStyle::CENTER, LargeFont);

    // "Zurück"
    AddTextButton(3, DrawPoint(380, 560), Extent(200, 22), TC_RED1, _("Back"), NormalFont);
    // "Spiel laden..."
    AddTextButton(4, DrawPoint(590, 530), Extent(200, 22), TC_GREEN2, _("Load game..."), NormalFont);
    // "Weiter"
    AddTextButton(5, DrawPoint(590, 560), Extent(200, 22), TC_GREEN2, _("Continue"), NormalFont);
    // random map generation
    AddTextButton(6, DrawPoint(380, 530), Extent(150, 22), TC_GREEN2, _("Random Map"), NormalFont);
    // random map settings
    AddTextButton(7, DrawPoint(540, 530), Extent(40, 22), TC_GREEN2, _("..."), NormalFont);

    ctrlOptionGroup* optiongroup = AddOptionGroup(10, ctrlOptionGroup::CHECK);
    Extent catBtSize = Extent(90, 22);
    // "Alte"
    DrawPoint curBtPos = DrawPoint(10, 35);
    optiongroup->AddTextButton(0, curBtPos, catBtSize, TC_GREY, _("Old maps"), NormalFont);
    curBtPos.y += catBtSize.y + 3;
    // "Neue"
    optiongroup->AddTextButton(1, curBtPos, catBtSize, TC_GREY, _("New maps"), NormalFont);
    curBtPos.y += catBtSize.y + 3;
    // "Eigene"
    optiongroup->AddTextButton(2, curBtPos, catBtSize, TC_GREY, _("Own maps"), NormalFont);
    curBtPos.y += catBtSize.y + 3;
    // "Kontinente"
    optiongroup->AddTextButton(3, curBtPos, catBtSize, TC_GREY, _("Continents"), NormalFont);
    curBtPos.y += catBtSize.y + 3;
    // "Kampagne"
    optiongroup->AddTextButton(4, curBtPos, catBtSize, TC_GREY, _("Campaign"), NormalFont);
    curBtPos.y += catBtSize.y + 3;
    // "RTTR"
    optiongroup->AddTextButton(5, curBtPos, catBtSize, TC_GREY, _("RTTR"), NormalFont);
    curBtPos.y += catBtSize.y + 3;
    // "Andere"
    optiongroup->AddTextButton(6, curBtPos, catBtSize, TC_GREY, _("Other"), NormalFont);
    curBtPos.y += catBtSize.y + 3;
    // "Andere"
    optiongroup->AddTextButton(7, curBtPos, catBtSize, TC_GREY, _("Sea"), NormalFont);
    curBtPos.y += catBtSize.y + 3;
    // "Heruntergeladene"
    optiongroup->AddTextButton(8, curBtPos, catBtSize, TC_GREY, _("Played"), NormalFont);

    AddPreviewMinimap(11, DrawPoint(110, 445), Extent(140, 140), nullptr);
    AddText(12, DrawPoint(260, 470), _("Map: "), COLOR_YELLOW, FontStyle::LEFT, NormalFont);
    AddText(13, DrawPoint(260, 490), _("Mapfile: "), COLOR_YELLOW, FontStyle::LEFT, NormalFont);

    // "Eigene" auswählen
    optiongroup->SetSelection(5, true);

    LOBBYCLIENT.AddListener(this);
    GAMECLIENT.SetInterface(this);
}

dskSelectMap::~dskSelectMap()
{
    // if(mapGenThread)
    //    mapGenThread->join();
    LOBBYCLIENT.RemoveListener(this);
    GAMECLIENT.RemoveInterface(this);
}

void dskSelectMap::Msg_OptionGroupChange(const unsigned /*ctrl_id*/, unsigned selection)
{
    auto* table = GetCtrl<ctrlTable>(1);

    // Tabelle leeren
    table->DeleteAllItems();

    // Old, New, Own, Continents, Campaign, RTTR, Other, Sea, Played
    static const std::array<unsigned, 9> ids = {{39, 40, 41, 42, 43, 52, 91, 93, 48}};

    const size_t numFaultyMapsPrior = brokenMapPaths.size();
    const std::string mapPath = RTTRCONFIG.ExpandPath(FILE_PATHS[ids[selection]]);
    FillTable(ListDir(mapPath, "swd"));
    FillTable(ListDir(mapPath, "wld"));
    // For own maps (WORLDS folder) also use the one in the installation folder as S2 does
    if(bfs::path(mapPath).filename() == "WORLDS")
    {
        const std::string worldsPath = RTTRCONFIG.ExpandPath("WORLDS");
        FillTable(ListDir(worldsPath, "swd"));
        FillTable(ListDir(worldsPath, "wld"));
    }

    if(brokenMapPaths.size() > numFaultyMapsPrior)
    {
        std::string errorTxt =
          helpers::format(_("%1% map(s) could not be loaded. Check the log for details"), brokenMapPaths.size() - numFaultyMapsPrior);
        WINDOWMANAGER.Show(std::make_unique<iwMsgbox>(_("Error"), errorTxt, this, MSB_OK, MSB_EXCLAMATIONRED, 1));
    }

    // Dann noch sortieren
    bool sortAsc = true;
    table->SortRows(0, &sortAsc);

    // und Auswahl zurücksetzen
    table->SetSelection(0);
}

/// Load a map, throw on error
static std::unique_ptr<glArchivItem_Map> loadAndVerifyMap(const std::string& path)
{
    libsiedler2::Archiv archive;
    if(int ec = libsiedler2::loader::LoadMAP(path, archive))
        throw std::runtime_error(libsiedler2::getErrorString(ec));
    auto map = libutil::dynamicUniqueCast<glArchivItem_Map>(archive.release(0));
    if(!map)
        throw std::runtime_error(_("Unexpected dynamic type of map"));
    if(map->getHeader().getWidth() > MAX_MAP_SIZE || map->getHeader().getHeight() > MAX_MAP_SIZE)
        throw std::runtime_error(helpers::format(_("Map is bigger than allowed size of %1% nodes"), MAX_MAP_SIZE));
    if(map->getHeader().getNumPlayers() > MAX_PLAYERS)
        throw std::runtime_error(helpers::format(_("Map has more than %1% players"), MAX_PLAYERS));
    return map;
}

/**
 *  Occurs when user changes the selection in the table of maps.
 */
void dskSelectMap::Msg_TableSelectItem(const unsigned ctrl_id, const int selection)
{
    if(ctrl_id != 1)
        return;

    ctrlPreviewMinimap& preview = *GetCtrl<ctrlPreviewMinimap>(11);
    ctrlText& txtMapName = *GetCtrl<ctrlText>(12);
    ctrlText& txtMapPath = *GetCtrl<ctrlText>(13);
    ctrlButton& btContinue = *GetCtrl<ctrlButton>(5);
    preview.SetMap(nullptr);
    txtMapName.SetText("");
    txtMapPath.SetText("");
    btContinue.SetEnabled(false);

    // is the selection valid?
    if(selection >= 0)
    {
        ctrlTable& table = *GetCtrl<ctrlTable>(1);
        const std::string& path = table.GetItemText(selection, 5);
        if(!path.empty())
        {
            try
            {
                std::unique_ptr<glArchivItem_Map> map = loadAndVerifyMap(path);
                RTTR_Assert(map);
                preview.SetMap(map.get());
                txtMapName.SetText(s25util::ansiToUTF8(map->getHeader().getName()));
                txtMapPath.SetText(path);
                btContinue.SetEnabled(true);
            } catch(const std::runtime_error& e)
            {
                const std::string errorTxt = helpers::format(_("Could not load map:\n%1%\n%2%"), path, e.what());
                LOG.write("%1%\n") % errorTxt;
                WINDOWMANAGER.Show(std::make_unique<iwMsgbox>(_("Error"), errorTxt, this, MSB_OK, MSB_EXCLAMATIONRED, 1));
                brokenMapPaths.insert(path);
                table.RemoveRow(selection);
            }
        }
    }
    const unsigned txtXPos = preview.GetPos().x + preview.GetSize().x + 10;
    txtMapName.SetPos(DrawPoint(txtXPos, txtMapName.GetPos().y));
    txtMapPath.SetPos(DrawPoint(txtXPos, txtMapPath.GetPos().y));
}

void dskSelectMap::GoBack()
{
    if(csi.type == ServerType::LOCAL)
        WINDOWMANAGER.Switch(std::make_unique<dskSinglePlayer>());
    else if(csi.type == ServerType::LAN)
        WINDOWMANAGER.Switch(std::make_unique<dskLAN>());
    else if(csi.type == ServerType::LOBBY && LOBBYCLIENT.IsLoggedIn())
        WINDOWMANAGER.Switch(std::make_unique<dskLobby>());
    else
        WINDOWMANAGER.Switch(std::make_unique<dskDirectIP>());
}

void dskSelectMap::Msg_ButtonClick(const unsigned ctrl_id)
{
    switch(ctrl_id)
    {
        case 3: // "Zurück"
        {
            GoBack();
        }
        break;
        case 4: // "Spiel laden..."
        {
            // Ladefenster aufrufen
            WINDOWMANAGER.ToggleWindow(std::make_unique<iwLoad>(csi));
        }
        break;
        case 5: // "Weiter"
        {
            StartServer();
        }
        break;
        case 6: // random map
        {
            if(!mapGenThread)
            {
                newRandMapPath.clear();
                waitWnd = &WINDOWMANAGER.Show(std::make_unique<iwPleaseWait>());
                // mapGenThread = new boost::thread(boost::bind(&dskSelectMap::CreateRandomMap, this));
                CreateRandomMap();
            }
        }
        break;
        case 7: // random map generator settings
        {
            WINDOWMANAGER.ToggleWindow(std::make_unique<iwMapGenerator>(rndMapSettings));
        }
        break;
    }
}

void dskSelectMap::Msg_TableChooseItem(const unsigned /*ctrl_id*/, const unsigned /*selection*/)
{
    // Doppelklick auf bestimmte Map -> weiter
    StartServer();
}

void dskSelectMap::CreateRandomMap()
{
    // setup filepath for the random map
    std::string mapPath = (bfs::path(RTTRCONFIG.ExpandPath(FILE_PATHS[48])) / "Random.swd").string();

    try
    {
        // create a random map and save filepath
        MapGenerator::Create(mapPath, rndMapSettings);
        newRandMapPath = mapPath;
    } catch(std::runtime_error& e)
    {
        newRandMapPath = std::string("!") + e.what();
    }
}

void dskSelectMap::OnMapCreated(const std::string& mapPath)
{
    if(waitWnd)
    {
        waitWnd->Close();
        waitWnd = nullptr;
    }
    // select the "played maps" entry
    auto* optionGroup = GetCtrl<ctrlOptionGroup>(10);
    optionGroup->SetSelection(8, true);

    // search for the random map entry and select it in the table
    auto* table = GetCtrl<ctrlTable>(1);
    for(int i = 0; i < table->GetNumRows(); i++)
    {
        std::string entryPath = table->GetItemText(i, 5);

        if(entryPath == mapPath)
        {
            table->SetSelection(i);
            break;
        }
    }
}

/// Startet das Spiel mit einer bestimmten Auswahl in der Tabelle
void dskSelectMap::StartServer()
{
    auto* table = GetCtrl<ctrlTable>(1);
    unsigned short selection = table->GetSelection();

    // Ist die Auswahl gültig?
    if(selection < table->GetNumRows())
    {
        // Kartenpfad aus Tabelle holen
        const std::string& mapPath = table->GetItemText(selection, 5);

        // Server starten
        if(!GAMECLIENT.HostGame(csi, mapPath, MAPTYPE_OLDMAP))
            GoBack();
        else
        {
            // Verbindungsfenster anzeigen
            WINDOWMANAGER.Show(std::make_unique<iwPleaseWait>());
        }
    }
}

void dskSelectMap::Msg_MsgBoxResult(const unsigned msgbox_id, const MsgboxResult /*mbr*/)
{
    if(msgbox_id == 0) // Verbindung zu Server verloren?
    {
        GAMECLIENT.Stop();

        if(csi.type == ServerType::LOBBY && LOBBYCLIENT.IsLoggedIn()) // steht die Lobbyverbindung noch?
            WINDOWMANAGER.Switch(std::make_unique<dskLobby>());
        else if(csi.type == ServerType::LOBBY)
            WINDOWMANAGER.Switch(std::make_unique<dskDirectIP>());
        else if(csi.type == ServerType::LAN)
            WINDOWMANAGER.Switch(std::make_unique<dskLAN>());
        else
            WINDOWMANAGER.Switch(std::make_unique<dskSinglePlayer>());
    }
}

void dskSelectMap::CI_NextConnectState(const ConnectState cs)
{
    switch(cs)
    {
        case CS_FINISHED:
        {
            std::unique_ptr<ILobbyClient> lobbyClient;
            if(csi.type == ServerType::LOBBY)
                lobbyClient = std::make_unique<RttrLobbyClient>(LOBBYCLIENT);
            WINDOWMANAGER.Switch(
              std::make_unique<dskHostGame>(csi.type, GAMECLIENT.GetGameLobby(), GAMECLIENT.GetPlayerId(), std::move(lobbyClient)));
            break;
        }
        default: break;
    }
}

void dskSelectMap::CI_Error(const ClientError ce)
{
    WINDOWMANAGER.Show(std::make_unique<iwMsgbox>(_("Error"), ClientErrorToStr(ce), this, MSB_OK, MSB_EXCLAMATIONRED, 0));
}

/**
 *  (Lobby-)Status: Benutzerdefinierter Fehler (kann auch Conn-Loss o.ä sein)
 */
void dskSelectMap::LC_Status_Error(const std::string& error)
{
    WINDOWMANAGER.Show(std::make_unique<iwMsgbox>(_("Error"), error, this, MSB_OK, MSB_EXCLAMATIONRED, 0));
}

void dskSelectMap::Draw_()
{
    if(!newRandMapPath.empty())
    {
        // mapGenThread->join();
        // mapGenThread = nullptr;
        if(newRandMapPath[0] == '!')
        {
            std::string errorTxt = _("Failed to generate random map.\nReason: ");
            WINDOWMANAGER.Show(
              std::make_unique<iwMsgbox>(_("Error"), errorTxt + newRandMapPath.substr(1), nullptr, MSB_OK, MSB_EXCLAMATIONRED));
        } else
            OnMapCreated(newRandMapPath);
        newRandMapPath.clear();
    }
    Desktop::Draw_();
}

void dskSelectMap::FillTable(const std::vector<bfs::path>& files)
{
    auto* table = GetCtrl<ctrlTable>(1);

    for(const bfs::path& filePath : files)
    {
        if(helpers::contains(brokenMapPaths, filePath))
            continue;
        // Karteninformationen laden
        libsiedler2::Archiv map;
        if(int ec = libsiedler2::loader::LoadMAP(filePath.string(), map, true))
        {
            LOG.write(_("Failed to load map %1%: %2%\n")) % filePath % libsiedler2::getErrorString(ec);
            brokenMapPaths.insert(filePath);
            continue;
        }

        const libsiedler2::ArchivItem_Map_Header& header = checkedCast<const glArchivItem_Map*>(map[0])->getHeader();

        const bfs::path luaFilepath = bfs::path(filePath).replace_extension("lua");
        const bool hasLua = bfs::is_regular_file(luaFilepath);

        // Und Zeilen vorbereiten
        std::string players = (boost::format(_("%d Player")) % static_cast<unsigned>(header.getNumPlayers())).str();
        std::string size = helpers::toString(header.getWidth()) + "x" + helpers::toString(header.getHeight());

        std::string name = s25util::ansiToUTF8(header.getName());
        if(hasLua)
            name += " (*)";
        std::string author = s25util::ansiToUTF8(header.getAuthor());

        table->AddRow({name, author, players, landscapeNames[header.getGfxSet()], size, filePath.string()});
    }
}
