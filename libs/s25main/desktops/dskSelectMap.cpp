// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "dskSelectMap.h"
#include "ListDir.h"
#include "Loader.h"
#include "RttrConfig.h"
#include "RttrLobbyClient.hpp"
#include "WindowManager.h"
#include "commonDefines.h"
#include "controls/ctrlButton.h"
#include "controls/ctrlOptionGroup.h"
#include "controls/ctrlPreviewMinimap.h"
#include "controls/ctrlTable.h"
#include "controls/ctrlText.h"
#include "desktops/dskDirectIP.h"
#include "desktops/dskLAN.h"
#include "desktops/dskLobby.h"
#include "desktops/dskSinglePlayer.h"
#include "files.h"
#include "helpers/containerUtils.h"
#include "helpers/format.hpp"
#include "helpers/toString.h"
#include "ingameWindows/iwConnecting.h"
#include "ingameWindows/iwMapGenerator.h"
#include "ingameWindows/iwMsgbox.h"
#include "ingameWindows/iwPleaseWait.h"
#include "ingameWindows/iwSave.h"
#include "lua/GameDataLoader.h"
#include "mapGenerator/RandomMap.h"
#include "network/GameClient.h"
#include "ogl/FontStyle.h"
#include "gameData/MapConsts.h"
#include "gameData/WorldDescription.h"
#include "liblobby/LobbyClient.h"
#include "libsiedler2/ArchivItem_Map.h"
#include "libsiedler2/ArchivItem_Map_Header.h"
#include "libsiedler2/ErrorCodes.h"
#include "libsiedler2/prototypen.h"
#include "s25util/Log.h"
#include "s25util/utf8.h"
#include <boost/filesystem/operations.hpp>
#include <boost/pointer_cast.hpp>
#include <stdexcept>
#include <utility>

namespace bfs = boost::filesystem;
constexpr unsigned ID_msgBoxError = 0;

/**
 *  Konstruktor von @p dskSelectMap.
 *
 *  @param[in] type Typ des Servers
 *  @param[in] name Server-Name
 *  @param[in] pass Server-Passwort
 */
dskSelectMap::dskSelectMap(CreateServerInfo csi)
    : Desktop(LOADER.getImageN("setup015", 0)), csi(std::move(csi)), mapGenThread(nullptr), waitWnd(nullptr)
{
    WorldDescription desc;
    GameDataLoader gdLoader(desc);
    if(!gdLoader.load())
    {
        LC_Status_Error(_("Failed to load game data!"));
        return;
    }

    for(const auto& l : desc.landscapes)
        landscapeNames[l.s2Id] = _(l.name);

    // Die Tabelle für die Maps
    using SRT = ctrlTable::SortType;
    addTable(1, DrawPoint(110, 35), Extent(680, 400), TextureColor::Grey, NormalFont,
             ctrlTable::Columns{{_("Name"), 250, SRT::String},
                                {_("Author"), 216, SRT::String},
                                {_("Player"), 170, SRT::Number},
                                {_("Type"), 180, SRT::String},
                                {_("Size"), 134, SRT::MapSize},
                                {"", 0, SRT::Default}});

    // "Karten Auswahl"
    addText(2, DrawPoint(400, 5), _("Selection of maps"), COLOR_YELLOW, FontStyle::CENTER, LargeFont);

    // "Zurück"
    addTextButton(3, DrawPoint(380, 560), Extent(200, 22), TextureColor::Red1, _("Back"), NormalFont);
    // "Spiel laden..."
    addTextButton(4, DrawPoint(590, 530), Extent(200, 22), TextureColor::Green2, _("Load game..."), NormalFont);
    // "Weiter"
    addTextButton(5, DrawPoint(590, 560), Extent(200, 22), TextureColor::Green2, _("Continue"), NormalFont);
    // random map generation
    addTextButton(6, DrawPoint(380, 530), Extent(150, 22), TextureColor::Green2, _("Random Map"), NormalFont);
    // random map settings
    addTextButton(7, DrawPoint(540, 530), Extent(40, 22), TextureColor::Green2, _("..."), NormalFont);

    ctrlOptionGroup* optiongroup = addOptionGroup(10, GroupSelectType::Check);
    Extent catBtSize = Extent(90, 22);
    // "Alte"
    DrawPoint curBtPos = DrawPoint(10, 35);
    optiongroup->addTextButton(0, curBtPos, catBtSize, TextureColor::Grey, _("Old maps"), NormalFont);
    curBtPos.y += catBtSize.y + 3;
    // "Neue"
    optiongroup->addTextButton(1, curBtPos, catBtSize, TextureColor::Grey, _("New maps"), NormalFont);
    curBtPos.y += catBtSize.y + 3;
    // "Eigene"
    optiongroup->addTextButton(2, curBtPos, catBtSize, TextureColor::Grey, _("Own maps"), NormalFont);
    curBtPos.y += catBtSize.y + 3;
    // "Kontinente"
    optiongroup->addTextButton(3, curBtPos, catBtSize, TextureColor::Grey, _("Continents"), NormalFont);
    curBtPos.y += catBtSize.y + 3;
    // "Kampagne"
    optiongroup->addTextButton(4, curBtPos, catBtSize, TextureColor::Grey, _("Campaign"), NormalFont);
    curBtPos.y += catBtSize.y + 3;
    // "RTTR"
    optiongroup->addTextButton(5, curBtPos, catBtSize, TextureColor::Grey, _("RTTR"), NormalFont);
    curBtPos.y += catBtSize.y + 3;
    // "Andere"
    optiongroup->addTextButton(6, curBtPos, catBtSize, TextureColor::Grey, _("Other"), NormalFont);
    curBtPos.y += catBtSize.y + 3;
    // "Andere"
    optiongroup->addTextButton(7, curBtPos, catBtSize, TextureColor::Grey, _("Sea"), NormalFont);
    curBtPos.y += catBtSize.y + 3;
    // "Heruntergeladene"
    optiongroup->addTextButton(8, curBtPos, catBtSize, TextureColor::Grey, _("Played"), NormalFont);

    addPreviewMinimap(11, DrawPoint(110, 445), Extent(140, 140), nullptr);
    addText(12, DrawPoint(260, 470), _("Map: "), COLOR_YELLOW, FontStyle::LEFT, NormalFont);
    addText(13, DrawPoint(260, 490), _("Mapfile: "), COLOR_YELLOW, FontStyle::LEFT, NormalFont);

    // "Eigene" auswählen
    optiongroup->setSelection(5, true);

    LOBBYCLIENT.addListener(this);
}

dskSelectMap::~dskSelectMap()
{
    // if(mapGenThread)
    //    mapGenThread->join();
    LOBBYCLIENT.removeListener(this);
}

void dskSelectMap::msgOptionGroupChange(const unsigned /*ctrl_id*/, unsigned selection)
{
    auto* table = getCtrl<ctrlTable>(1);

    // Tabelle leeren
    table->deleteAllItems();

    // Old, New, Own, Continents, Campaign, RTTR, Other, Sea, Played
    static const std::array<std::string, 9> ids = {{s25::folders::mapsOld, s25::folders::mapsNew, s25::folders::mapsOwn,
                                                    s25::folders::mapsContinents, s25::folders::mapsCampaign,
                                                    s25::folders::mapsRttr, s25::folders::mapsOther,
                                                    s25::folders::mapsSea, s25::folders::mapsPlayed}};

    const size_t numFaultyMapsPrior = brokenMapPaths.size();
    const bfs::path mapPath = RTTRCONFIG.expandPath(ids[selection]);
    fillTable(listDir(mapPath, "swd"));
    fillTable(listDir(mapPath, "wld"));
    // For own maps (WORLDS folder) also use the one in the installation folder as S2 does
    if(mapPath.filename() == "WORLDS")
    {
        const bfs::path worldsPath = RTTRCONFIG.expandPath("WORLDS");
        fillTable(listDir(worldsPath, "swd"));
        fillTable(listDir(worldsPath, "wld"));
    }

    if(brokenMapPaths.size() > numFaultyMapsPrior)
    {
        std::string errorTxt = helpers::format(_("%1% map(s) could not be loaded. Check the log for details"),
                                               brokenMapPaths.size() - numFaultyMapsPrior);
        WINDOWMANAGER.show(
          std::make_unique<iwMsgbox>(_("Error"), errorTxt, this, MsgboxButton::Ok, MsgboxIcon::ExclamationRed, 1));
    }

    // Dann noch sortieren
    table->sortRows(0, TableSortDir::Ascending);

    // und Auswahl zurücksetzen
    table->setSelection(std::nullopt);
}

/// Load a map, throw on error
static std::unique_ptr<libsiedler2::ArchivItem_Map> loadAndVerifyMap(const std::string& path)
{
    libsiedler2::Archiv archive;
    if(int ec = libsiedler2::loader::LoadMAP(path, archive))
        throw std::runtime_error(libsiedler2::getErrorString(ec));
    auto map = boost::dynamic_pointer_cast<libsiedler2::ArchivItem_Map>(archive.release(0));
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
void dskSelectMap::msgTableSelectItem(const unsigned ctrl_id, const std::optional<unsigned>& selection)
{
    if(ctrl_id != 1)
        return;

    ctrlPreviewMinimap& preview = *getCtrl<ctrlPreviewMinimap>(11);
    ctrlText& txtMapName = *getCtrl<ctrlText>(12);
    ctrlText& txtMapPath = *getCtrl<ctrlText>(13);
    ctrlButton& btContinue = *getCtrl<ctrlButton>(5);
    preview.setMap(nullptr);
    txtMapName.setText("");
    txtMapPath.setText("");
    btContinue.setEnabled(false);

    // is the selection valid?
    if(selection)
    {
        ctrlTable& table = *getCtrl<ctrlTable>(1);
        const std::string& path = table.getItemText(*selection, 5);
        if(!path.empty())
        {
            try
            {
                std::unique_ptr<libsiedler2::ArchivItem_Map> map = loadAndVerifyMap(path);
                RTTR_Assert(map);
                preview.setMap(map.get());
                txtMapName.setText(s25util::ansiToUTF8(map->getHeader().getName()));
                txtMapPath.setText(path);
                btContinue.setEnabled(true);
            } catch(const std::runtime_error& e)
            {
                const std::string errorTxt = helpers::format(_("Could not load map:\n%1%\n%2%"), path, e.what());
                LOG.write("%1%\n") % errorTxt;
                WINDOWMANAGER.show(std::make_unique<iwMsgbox>(_("Error"), errorTxt, this, MsgboxButton::Ok,
                                                              MsgboxIcon::ExclamationRed, 1));
                brokenMapPaths.insert(path);
                table.removeRow(*selection);
            }
        }
    }
    const unsigned txtXPos = preview.getPos().x + preview.getSize().x + 10;
    txtMapName.setPos(DrawPoint(txtXPos, txtMapName.getPos().y));
    txtMapPath.setPos(DrawPoint(txtXPos, txtMapPath.getPos().y));
}

void dskSelectMap::goBack() const
{
    if(csi.type == ServerType::Local)
        WINDOWMANAGER.switchTo(std::make_unique<dskSinglePlayer>());
    else if(csi.type == ServerType::LAN)
        WINDOWMANAGER.switchTo(std::make_unique<dskLAN>());
    else if(csi.type == ServerType::Lobby && LOBBYCLIENT.isLoggedIn())
        WINDOWMANAGER.switchTo(std::make_unique<dskLobby>());
    else
        WINDOWMANAGER.switchTo(std::make_unique<dskDirectIP>());
}

void dskSelectMap::msgButtonClick(const unsigned ctrl_id)
{
    switch(ctrl_id)
    {
        case 3: // "Zurück"
        {
            goBack();
        }
        break;
        case 4: // "Spiel laden..."
        {
            // Ladefenster aufrufen
            WINDOWMANAGER.toggleWindow(std::make_unique<iwLoad>(csi));
        }
        break;
        case 5: // "Weiter"
        {
            startServer();
        }
        break;
        case 6: // random map
        {
            if(!mapGenThread)
            {
                newRandMapPath.clear();
                randMapGenError.clear();
                waitWnd = &WINDOWMANAGER.show(std::make_unique<iwPleaseWait>());
                // mapGenThread = new boost::thread(boost::bind(&dskSelectMap::CreateRandomMap, this));
                createRandomMap();
            }
        }
        break;
        case 7: // random map generator settings
        {
            WINDOWMANAGER.toggleWindow(std::make_unique<iwMapGenerator>(rndMapSettings));
        }
        break;
    }
}

void dskSelectMap::msgTableChooseItem(const unsigned /*ctrl_id*/, const unsigned /*selection*/)
{
    // Doppelklick auf bestimmte Map -> weiter
    startServer();
}

void dskSelectMap::createRandomMap()
{
    // setup filepath for the random map
    const auto mapPath = RTTRCONFIG.expandPath(s25::folders::mapsPlayed) / "Random.swd";

    try
    {
        // create a random map and save filepath
        rttr::mapGenerator::createRandomMap(mapPath, rndMapSettings);
        newRandMapPath = mapPath;
    } catch(const std::runtime_error& e)
    {
        randMapGenError = e.what();
    }
}

void dskSelectMap::onMapCreated(const boost::filesystem::path& mapPath)
{
    // select the "played maps" entry
    auto* optionGroup = getCtrl<ctrlOptionGroup>(10);
    optionGroup->setSelection(8, true);

    // search for the random map entry and select it in the table
    auto* table = getCtrl<ctrlTable>(1);
    const auto& mapPathString = mapPath.string();
    for(int i = 0; i < table->getNumRows(); i++)
    {
        if(table->getItemText(i, 5) == mapPathString)
        {
            table->setSelection(i);
            break;
        }
    }
}

/// Startet das Spiel mit einer bestimmten Auswahl in der Tabelle
void dskSelectMap::startServer()
{
    auto* table = getCtrl<ctrlTable>(1);
    const auto& selection = table->getSelection();

    // Ist die Auswahl gültig?
    if(selection)
    {
        // Kartenpfad aus Tabelle holen
        const std::string& mapPath = table->getItemText(*selection, 5);

        // Server starten
        if(!GAMECLIENT.hostGame(csi, {mapPath, MapType::OldMap}))
            goBack();
        else
        {
            std::unique_ptr<ILobbyClient> lobbyClient;
            if(csi.type == ServerType::Lobby)
                lobbyClient = std::make_unique<RttrLobbyClient>(LOBBYCLIENT);
            iwConnecting& wnd = WINDOWMANAGER.show(std::make_unique<iwConnecting>(csi.type, std::move(lobbyClient)));
            onErrorConnection_ = wnd.onError.connect([this](ClientError error) {
                WINDOWMANAGER.show(std::make_unique<iwMsgbox>(_("Error"), clientErrorToStr(error), this,
                                                              MsgboxButton::Ok, MsgboxIcon::ExclamationRed,
                                                              ID_msgBoxError));
            });
        }
    }
}

void dskSelectMap::msgMsgBoxResult(const unsigned msgbox_id, const MsgboxResult /*mbr*/)
{
    if(msgbox_id == ID_msgBoxError)
    {
        GAMECLIENT.stop();

        if(csi.type == ServerType::Lobby && LOBBYCLIENT.isLoggedIn()) // steht die Lobbyverbindung noch?
            WINDOWMANAGER.switchTo(std::make_unique<dskLobby>());
        else if(csi.type == ServerType::Lobby)
            WINDOWMANAGER.switchTo(std::make_unique<dskDirectIP>());
        else if(csi.type == ServerType::LAN)
            WINDOWMANAGER.switchTo(std::make_unique<dskLAN>());
        else
            WINDOWMANAGER.switchTo(std::make_unique<dskSinglePlayer>());
    }
}

/**
 *  (Lobby-)Status: Benutzerdefinierter Fehler (kann auch Conn-Loss o.ä sein)
 */
void dskSelectMap::LC_Status_Error(const std::string& error)
{
    WINDOWMANAGER.show(std::make_unique<iwMsgbox>(_("Error"), error, this, MsgboxButton::Ok, MsgboxIcon::ExclamationRed,
                                                  ID_msgBoxError));
}

void dskSelectMap::drawImpl()
{
    if(!newRandMapPath.empty() || !randMapGenError.empty())
    {
        if(waitWnd)
        {
            waitWnd->close();
            waitWnd = nullptr;
        }
        if(!randMapGenError.empty())
        {
            const std::string errorTxt = _("Failed to generate random map.\nReason: ") + randMapGenError;
            WINDOWMANAGER.show(
              std::make_unique<iwMsgbox>(_("Error"), errorTxt, nullptr, MsgboxButton::Ok, MsgboxIcon::ExclamationRed));
        } else
            onMapCreated(newRandMapPath);
        newRandMapPath.clear();
        randMapGenError.clear();
    }
    Desktop::drawImpl();
}

void dskSelectMap::fillTable(const std::vector<bfs::path>& files)
{
    auto* table = getCtrl<ctrlTable>(1);

    for(const bfs::path& filePath : files)
    {
        if(helpers::contains(brokenMapPaths, filePath))
            continue;
        // Karteninformationen laden
        libsiedler2::Archiv map;
        if(int ec = libsiedler2::loader::LoadMAP(filePath, map, true))
        {
            LOG.write(_("Failed to load map %1%: %2%\n")) % filePath % libsiedler2::getErrorString(ec);
            brokenMapPaths.insert(filePath);
            continue;
        }

        const libsiedler2::ArchivItem_Map_Header& header =
          checkedCast<const libsiedler2::ArchivItem_Map*>(map[0])->getHeader();

        const bfs::path luaFilepath = bfs::path(filePath).replace_extension("lua");
        const bool hasLua = bfs::is_regular_file(luaFilepath);

        // Und Zeilen vorbereiten
        std::string players = (boost::format(_("%d Player")) % static_cast<unsigned>(header.getNumPlayers())).str();
        std::string size = helpers::toString(header.getWidth()) + "x" + helpers::toString(header.getHeight());

        std::string name = s25util::ansiToUTF8(header.getName());
        if(hasLua)
            name += " (*)";
        std::string author = s25util::ansiToUTF8(header.getAuthor());

        table->addRow({name, author, players, landscapeNames[header.getGfxSet()], size, filePath.string()});
    }
}
