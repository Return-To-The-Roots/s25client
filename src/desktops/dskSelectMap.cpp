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
#include "dskSelectMap.h"
#include "ListDir.h"
#include "Loader.h"
#include "RttrConfig.h"
#include "WindowManager.h"
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
#include "helpers/converters.h"
#include "ingameWindows/iwDirectIPCreate.h"
#include "ingameWindows/iwMapGenerator.h"
#include "ingameWindows/iwMsgbox.h"
#include "ingameWindows/iwPleaseWait.h"
#include "ingameWindows/iwSave.h"
#include "mapGenerator/MapGenerator.h"
#include "network/GameClient.h"
#include "network/GameServer.h"
#include "ogl/FontStyle.h"
#include "ogl/glArchivItem_Map.h"
#include "liblobby/LobbyClient.h"
#include "libsiedler2/ArchivItem_Map_Header.h"
#include "libsiedler2/prototypen.h"
#include "libutil/ucString.h"
#include <boost/bind.hpp>
#include <boost/filesystem.hpp>
#include <boost/foreach.hpp>
//#include <boost/thread.hpp>

/** @class dskSelectMap
 *
 *  Klasse des Map-Auswahl Desktops.
 */

/** @var dskSelectMap::type
 *
 *  Typ des Servers.
 */

/** @var dskSelectMap::name
 *
 *  Name des Servers.
 */

/** @var dskSelectMap::pass
 *
 *  Passwort des Servers.
 */

/**
 *  Konstruktor von @p dskSelectMap.
 *
 *  @param[in] type Typ des Servers
 *  @param[in] name Server-Name
 *  @param[in] pass Server-Passwort
 */
dskSelectMap::dskSelectMap(const CreateServerInfo& csi)
    : Desktop(LOADER.GetImageN("setup015", 0)), csi(csi), mapGenThread(NULL), waitWnd(NULL)
{
    // Die Tabelle für die Maps
    AddTable(1, DrawPoint(110, 35), Extent(680, 400), TC_GREY, NormalFont, 6, _("Name"), 250, ctrlTable::SRT_STRING, _("Author"), 216,
             ctrlTable::SRT_STRING, _("Player"), 170, ctrlTable::SRT_NUMBER, _("Type"), 180, ctrlTable::SRT_STRING, _("Size"), 134,
             ctrlTable::SRT_MAPSIZE, "", 0, ctrlTable::SRT_STRING);

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

    AddPreviewMinimap(11, DrawPoint(110, 445), Extent(140, 140), NULL);
    AddText(12, DrawPoint(260, 470), _("Map: "), COLOR_YELLOW, FontStyle::LEFT, NormalFont);
    AddText(13, DrawPoint(260, 490), _("Mapfile: "), COLOR_YELLOW, FontStyle::LEFT, NormalFont);

    // "Eigene" auswählen
    optiongroup->SetSelection(5, true);

    LOBBYCLIENT.SetInterface(this);
    GAMECLIENT.SetInterface(this);
}

dskSelectMap::~dskSelectMap()
{
    // if(mapGenThread)
    //    mapGenThread->join();
    LOBBYCLIENT.RemoveInterface(this);
    GAMECLIENT.RemoveInterface(this);
}

void dskSelectMap::Msg_OptionGroupChange(const unsigned /*ctrl_id*/, const int selection)
{
    ctrlTable* table = GetCtrl<ctrlTable>(1);

    // Tabelle leeren
    table->DeleteAllItems();

    // Old, New, Own, Continents, Campaign, RTTR, Other, Sea, Played
    static const boost::array<unsigned, 9> ids = {{39, 40, 41, 42, 43, 52, 91, 93, 48}};

    // Und wieder füllen lassen
    FillTable(ListDir(RTTRCONFIG.ExpandPath(FILE_PATHS[ids[selection]]), "swd"));
    FillTable(ListDir(RTTRCONFIG.ExpandPath(FILE_PATHS[ids[selection]]), "wld"));

    // Dann noch sortieren
    bool sortAsc = true;
    table->SortRows(0, &sortAsc);

    // und Auswahl zurücksetzen
    table->SetSelection(0);
}

/**
 *  Occurs when user changes the selection in the table of maps.
 */
void dskSelectMap::Msg_TableSelectItem(const unsigned ctrl_id, const int selection)
{
    switch(ctrl_id)
    {
        case 1:
        {
            ctrlTable* table = GetCtrl<ctrlTable>(1);

            // is the selection valid?
            if(selection < table->GetNumRows())
            {
                // get path to map from table
                std::string path = table->GetItemText(selection, 5);

                libsiedler2::Archiv ai;
                // load map data
                if(libsiedler2::loader::LoadMAP(path, ai) == 0)
                {
                    glArchivItem_Map* map = dynamic_cast<glArchivItem_Map*>(ai.get(0));
                    if(map)
                    {
                        ctrlPreviewMinimap* preview = GetCtrl<ctrlPreviewMinimap>(11);
                        preview->SetMap(map);

                        ctrlText* text = GetCtrl<ctrlText>(12);
                        text->SetText(cvStringToUTF8(map->getHeader().getName()));
                        DrawPoint txtPos = text->GetPos();
                        txtPos.x = preview->GetPos().x + preview->GetSize().x + 10;
                        text->SetPos(txtPos);

                        text = GetCtrl<ctrlText>(13);
                        text->SetText(path);
                        txtPos = text->GetPos();
                        txtPos.x = preview->GetPos().x + preview->GetSize().x + 10;
                        text->SetPos(txtPos);
                    }
                }
            }
        }
        break;
    }
}

void dskSelectMap::GoBack()
{
    if(csi.type == ServerType::LOCAL)
        WINDOWMANAGER.Switch(new dskSinglePlayer);
    else if(csi.type == ServerType::LAN)
        WINDOWMANAGER.Switch(new dskLAN);
    else if(csi.type == ServerType::LOBBY && LOBBYCLIENT.IsLoggedIn())
        WINDOWMANAGER.Switch(new dskLobby);
    else
        WINDOWMANAGER.Switch(new dskDirectIP);
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
            WINDOWMANAGER.Show(new iwLoad(csi));
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
                waitWnd = new iwPleaseWait;
                WINDOWMANAGER.Show(waitWnd);
                // mapGenThread = new boost::thread(boost::bind(&dskSelectMap::CreateRandomMap, this));
                CreateRandomMap();
            }
        }
        break;
        case 7: // random map generator settings
        {
            WINDOWMANAGER.Show(new iwMapGenerator(rndMapSettings));
        }
        break;
    }
}

void dskSelectMap::Msg_TableChooseItem(const unsigned ctrl_id, const unsigned selection)
{
    // Doppelklick auf bestimmte Map -> weiter
    StartServer();
}

void dskSelectMap::CreateRandomMap()
{
    // setup filepath for the random map
    std::string mapPath = RTTRCONFIG.ExpandPath(FILE_PATHS[48]) + "/Random.swd";

    // create a random map and save filepath
    MapGenerator::Create(mapPath, rndMapSettings);

    newRandMapPath = mapPath;
}

void dskSelectMap::OnMapCreated(const std::string& mapPath)
{
    if(waitWnd)
    {
        waitWnd->Close();
        waitWnd = NULL;
    }
    // select the "played maps" entry
    ctrlOptionGroup* optionGroup = GetCtrl<ctrlOptionGroup>(10);
    optionGroup->SetSelection(8, true);

    // search for the random map entry and select it in the table
    ctrlTable* table = GetCtrl<ctrlTable>(1);
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
    ctrlTable* table = GetCtrl<ctrlTable>(1);
    unsigned short selection = table->GetSelection();

    // Ist die Auswahl gültig?
    if(selection < table->GetNumRows())
    {
        // Kartenpfad aus Tabelle holen
        std::string mapPath = table->GetItemText(selection, 5);

        // Server starten
        if(!GAMESERVER.TryToStart(csi, mapPath, MAPTYPE_OLDMAP))
        {
            GoBack();
        } else
        {
            // Verbindungsfenster anzeigen
            WINDOWMANAGER.Show(new iwPleaseWait);
        }
    }
}

void dskSelectMap::Msg_MsgBoxResult(const unsigned msgbox_id, const MsgboxResult /*mbr*/)
{
    if(msgbox_id == 0) // Verbindung zu Server verloren?
    {
        GAMECLIENT.Stop();

        if(csi.type == ServerType::LOBBY && LOBBYCLIENT.IsLoggedIn()) // steht die Lobbyverbindung noch?
            WINDOWMANAGER.Switch(new dskLobby);
        else if(csi.type == ServerType::LOBBY)
            WINDOWMANAGER.Switch(new dskDirectIP);
        else if(csi.type == ServerType::LAN)
            WINDOWMANAGER.Switch(new dskLAN);
        else
            WINDOWMANAGER.Switch(new dskSinglePlayer);
    }
}

void dskSelectMap::CI_NextConnectState(const ConnectState cs)
{
    switch(cs)
    {
        case CS_FINISHED: { WINDOWMANAGER.Switch(new dskHostGame(csi.type, GAMECLIENT.GetGameLobby(), GAMECLIENT.GetPlayerId()));
        }
        break;
        default: break;
    }
}

void dskSelectMap::CI_Error(const ClientError ce)
{
    // Error messages, CE_* values cannot be gotten here but are added to avoid memory access errors
    const boost::array<std::string, 8> errors = {{_("Incomplete message was received!"), _("This Server is full!"), "CE_WRONGPW",
                                                  _("Lost connection to server!"), "CE_INVALIDSERVERTYPE",
                                                  _("Map transmission was corrupt!"), "CE_WRONGVERSION", "CE_LOBBYFULL"}};

    WINDOWMANAGER.Show(new iwMsgbox(_("Error"), errors[ce], this, MSB_OK, MSB_EXCLAMATIONRED, 0));
}

/**
 *  (Lobby-)Status: Benutzerdefinierter Fehler (kann auch Conn-Loss o.ä sein)
 */
void dskSelectMap::LC_Status_Error(const std::string& error)
{
    WINDOWMANAGER.Show(new iwMsgbox(_("Error"), error, this, MSB_OK, MSB_EXCLAMATIONRED, 0));
}

/**
 *  (Lobby-)Server wurde erstellt.
 */
void dskSelectMap::LC_Created()
{
    // ggf. im nächstes Stadium weiter
    GAMESERVER.Start();
}

void dskSelectMap::Draw_()
{
    if(!newRandMapPath.empty())
    {
        // mapGenThread->join();
        // mapGenThread = NULL;
        OnMapCreated(newRandMapPath);
        newRandMapPath.clear();
    }
    Desktop::Draw_();
}

void dskSelectMap::FillTable(const std::vector<std::string>& files)
{
    ctrlTable* table = GetCtrl<ctrlTable>(1);

    BOOST_FOREACH(const std::string& filePath, files)
    {
        // Karteninformationen laden
        libsiedler2::Archiv map;
        if(libsiedler2::loader::LoadMAP(filePath, map, true) != 0)
            continue;

        const libsiedler2::ArchivItem_Map_Header& header = checkedCast<const glArchivItem_Map*>(map.get(0))->getHeader();

        if(header.getNumPlayers() > MAX_PLAYERS)
            continue;

        const bfs::path luaFilepath = bfs::path(filePath).replace_extension("lua");
        const bool hasLua = bfs::is_regular_file(luaFilepath);

        // Und Zeilen vorbereiten
        std::string players = (boost::format(_("%d Player")) % static_cast<unsigned>(header.getNumPlayers())).str();
        std::string size = helpers::toString(header.getWidth()) + "x" + helpers::toString(header.getWidth());

        // und einfügen
        const std::string landscapes[3] = {_("Greenland"), _("Wasteland"), _("Winter world")};

        std::string name = cvStringToUTF8(header.getName());
        if(hasLua)
            name += " (*)";
        std::string author = cvStringToUTF8(header.getAuthor());

        table->AddRow(0, name.c_str(), author.c_str(), players.c_str(), landscapes[header.getGfxSet()].c_str(), size.c_str(),
                      filePath.c_str());
    }
}
