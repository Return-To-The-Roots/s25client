// $Id: dskSelectMap.cpp 9357 2014-04-25 15:35:25Z FloSoft $
//
// Copyright (c) 2005 - 2011 Settlers Freaks (sf-team at siedler25.org)
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
#include "main.h"
#include "dskSelectMap.h"

#include "WindowManager.h"
#include "Loader.h"
#include "controls.h"
#include "GameServer.h"
#include "GameClient.h"
#include "ListDir.h"
#include "files.h"
#include "LobbyClient.h"

#include "dskDirectIP.h"
#include "dskHostGame.h"
#include "dskLobby.h"
#include "dskSinglePlayer.h"

#include "iwMsgbox.h"
#include "iwSave.h"
#include "iwDirectIPCreate.h"
#include "iwPleaseWait.h"

///////////////////////////////////////////////////////////////////////////////
// Makros / Defines
#if defined _WIN32 && defined _DEBUG && defined _MSC_VER
#define new new(_NORMAL_BLOCK, THIS_FILE, __LINE__)
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

///////////////////////////////////////////////////////////////////////////////
/** @class dskSelectMap
 *
 *  Klasse des Map-Auswahl Desktops.
 *
 *  @author OLiver
 */

///////////////////////////////////////////////////////////////////////////////
/** @var dskSelectMap::type
 *
 *  Typ des Servers.
 *
 *  @author OLiver
 */

///////////////////////////////////////////////////////////////////////////////
/** @var dskSelectMap::name
 *
 *  Name des Servers.
 *
 *  @author OLiver
 */

///////////////////////////////////////////////////////////////////////////////
/** @var dskSelectMap::pass
 *
 *  Passwort des Servers.
 *
 *  @author OLiver
 */

///////////////////////////////////////////////////////////////////////////////
/**
 *  Konstruktor von @p dskSelectMap.
 *
 *  @param[in] type Typ des Servers
 *  @param[in] name Server-Name
 *  @param[in] pass Server-Passwort
 *
 *  @author OLiver
 *  @author FloSoft
 */
dskSelectMap::dskSelectMap(const CreateServerInfo& csi)
    : Desktop(LOADER.GetImageN("setup015", 0)),
      csi(csi)
{
    // Die Tabelle für die Maps
    AddTable( 1, 110,  35, 680, 400, TC_GREY, NormalFont, 6, _("Name"), 250, ctrlTable::SRT_STRING, _("Author"), 216, ctrlTable::SRT_STRING, _("Player"), 170, ctrlTable::SRT_NUMBER, _("Type"), 180, ctrlTable::SRT_STRING, _("Size"), 134, ctrlTable::SRT_MAPSIZE, "", 0, ctrlTable::SRT_STRING);

    // "Karten Auswahl"
    AddText(  2, 400,   5, _("Selection of maps"), COLOR_YELLOW, glArchivItem_Font::DF_CENTER, LargeFont);

    // "Zurück"
    AddTextButton(3, 380, 560, 200, 22, TC_RED1, _("Back"), NormalFont);
    // "Spiel laden..."
    AddTextButton(4, 590, 530, 200, 22, TC_GREEN2, _("Load game..."), NormalFont);
    // "Weiter"
    AddTextButton(5, 590, 560, 200, 22, TC_GREEN2, _("Continue"), NormalFont);

    ctrlOptionGroup* optiongroup = AddOptionGroup(10, ctrlOptionGroup::CHECK, scale);
    // "Alte"
    optiongroup->AddTextButton(0, 10,  35, 90,  22, TC_GREY, _("Old maps"), NormalFont);
    // "Neue"
    optiongroup->AddTextButton(1, 10,  60, 90,  22, TC_GREY, _("New maps"), NormalFont);
    // "Eigene"
    optiongroup->AddTextButton(2, 10,  85, 90,  22, TC_GREY, _("Own maps"), NormalFont);
    // "Kontinente"
    optiongroup->AddTextButton(3, 10, 110, 90,  22, TC_GREY, _("Continents"), NormalFont);
    // "Kampagne"
    optiongroup->AddTextButton(4, 10, 135, 90,  22, TC_GREY, _("Campaign"), NormalFont);
    // "RTTR"
    optiongroup->AddTextButton(5, 10, 160, 90,  22, TC_GREY, _("RTTR"), NormalFont);
    // "Andere"
    optiongroup->AddTextButton(6, 10, 185, 90,  22, TC_GREY, _("Other"), NormalFont);
    // "Andere"
    optiongroup->AddTextButton(7, 10, 210, 90,  22, TC_GREY, _("Sea"), NormalFont);
    // "Heruntergeladene"
    optiongroup->AddTextButton(8, 10, 235, 90,  22, TC_GREY, _("Played"), NormalFont);

    AddPreviewMinimap(11, 110, 445, 140, 140, NULL);
    AddText(12, 260, 470, _("Map: "), COLOR_YELLOW, glArchivItem_Font::DF_LEFT, NormalFont);
    AddText(13, 260, 490, _("Mapfile: "), COLOR_YELLOW, glArchivItem_Font::DF_LEFT, NormalFont);

    // "Eigene" auswählen
    optiongroup->SetSelection(5, true);

    LOBBYCLIENT.SetInterface(this);
    GAMECLIENT.SetInterface(this);
}

///////////////////////////////////////////////////////////////////////////////
/**
 *
 *  @author OLiver
 */
dskSelectMap::~dskSelectMap()
{
}

///////////////////////////////////////////////////////////////////////////////
/**
 *
 *  @author OLiver
 *  @author FloSoft
 */
void dskSelectMap::Msg_OptionGroupChange(const unsigned int ctrl_id, const unsigned short selection)
{
    ctrlTable* table = GetCtrl<ctrlTable>(1);

    // Tabelle leeren
    table->DeleteAllItems();

    static const unsigned int ids[] = { 39, 40, 41, 42, 43, 52, 91, 93, 48 };
    char path[4096];

    // Und wieder füllen lassen
    snprintf(path, 4096, "%s*.swd", GetFilePath(FILE_PATHS[ids[selection]]).c_str());
    ListDir(path, false, FillTable, (void*)table );

    // Nach beiden Kartentypen suchen
    snprintf(path, 4096, "%s*.wld", GetFilePath(FILE_PATHS[ids[selection]]).c_str());
    ListDir(path, false, FillTable, (void*)table );

    // Dann noch sortieren
    table->SortRows(0);

    // und Auswahl zurücksetzen
    table->SetSelection(0);
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  Occurs when user changes the selection in the table of maps.
 *
 *  @author FloSoft
 */
void dskSelectMap::Msg_TableSelectItem(const unsigned int ctrl_id, const unsigned short selection)
{
    switch(ctrl_id)
    {
        case 1:
        {
            ctrlTable* table = GetCtrl<ctrlTable>(1);

            // is the selection valid?
            if(selection < table->GetRowCount())
            {
                // get path to map from table
                std::string path = table->GetItemText(selection, 5);

                libsiedler2::ArchivInfo ai;
                // load map data
                if(libsiedler2::loader::LoadMAP(path.c_str(), &ai) == 0)
                {
                    glArchivItem_Map* map = dynamic_cast<glArchivItem_Map*>(ai.get(0));
                    if(map)
                    {
                        ctrlPreviewMinimap* preview = GetCtrl<ctrlPreviewMinimap>(11);
                        preview->SetMap(map);

                        ctrlText* text = GetCtrl<ctrlText>(12);
                        text->SetText(std::string(map->getHeader().getName()) );
                        text->Move(preview->GetX(true) + preview->GetWidth() + 10, text->GetY(true), true);

                        text = GetCtrl<ctrlText>(13);
                        text->SetText(path.c_str());
                        text->Move(preview->GetX(true) + preview->GetWidth() + 10, text->GetY(true), true);
                    }
                }
            }
        } break;
    }
}

///////////////////////////////////////////////////////////////////////////////
/**
 *
 *  @author OLiver
 */
void dskSelectMap::Msg_ButtonClick(const unsigned int ctrl_id)
{
    switch(ctrl_id)
    {
        case 3: // "Zurück"
        {
            if(csi.type == NP_LOCAL)
                WindowManager::inst().Switch(new dskSinglePlayer);
            else
            {
                if(LOBBYCLIENT.LoggedIn())
                    WindowManager::inst().Switch(new dskLobby);
                else
                    WindowManager::inst().Switch(new dskDirectIP);
            }
        } break;
        case 4: // "Spiel laden..."
        {
            // Ladefenster aufrufen
            WindowManager::inst().Show(new iwLoad(csi));
        } break;
        case 5: // "Weiter"
        {
            StartServer();
        } break;
    }
}

void dskSelectMap::Msg_TableChooseItem(const unsigned ctrl_id, const unsigned short selection)
{
    // Doppelklick auf bestimmte Map -> weiter
    StartServer();
}

/// Startet das Spiel mit einer bestimmten Auswahl in der Tabelle
void dskSelectMap::StartServer()
{
    ctrlTable* table = GetCtrl<ctrlTable>(1);
    unsigned short selection = table->GetSelection();

    // Ist die Auswahl gültig?
    if(selection < table->GetRowCount())
    {
        // Kartenpfad aus Tabelle holen
        map_path = table->GetItemText(selection, 5);

        // Server starten
        if(!GAMESERVER.TryToStart(csi, map_path, MAPTYPE_OLDMAP))
        {
            // Falls es ein lokal Spiel werden sollte, zurück zum SP-Menü
            if (csi.type == NP_LOCAL)
            {
                WindowManager::inst().Switch(new dskSinglePlayer);
            }
            else if(LOBBYCLIENT.LoggedIn())
                // Lobby zeigen, wenn das nich ging
                WindowManager::inst().Switch(new dskLobby);
            else
                // Ansonsten DirekteIP
                WindowManager::inst().Switch(new dskDirectIP);
        }
        else
        {
            // Verbindungsfenster anzeigen
            WindowManager::inst().Show(new iwPleaseWait);
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/**
 *
 *  @author OLiver
 */
void dskSelectMap::Msg_MsgBoxResult(const unsigned msgbox_id, const MsgboxResult mbr)
{
    if(msgbox_id == 0) // Verbindung zu Server verloren?
    {
        GAMECLIENT.Stop();

        if(LOBBYCLIENT.LoggedIn()) // steht die Lobbyverbindung noch?
            WindowManager::inst().Switch(new dskLobby);
        else
            WindowManager::inst().Switch(new dskDirectIP);
    }
}

///////////////////////////////////////////////////////////////////////////////
/**
 *
 *  @author OLiver
 */
void dskSelectMap::CI_NextConnectState(const ConnectState cs)
{
    switch(cs)
    {
        case CS_FINISHED:
        {
            WindowManager::inst().Switch(new dskHostGame((csi.type == NP_LOCAL)));
        } break;
        default:
            break;
    }

}

///////////////////////////////////////////////////////////////////////////////
/**
 *
 *  @author OLiver
 */
void dskSelectMap::CI_Error(const ClientError ce)
{
    switch(ce)
    {
        case CE_INCOMPLETEMESSAGE:
        case CE_CONNECTIONLOST:
        {
            // Verbindung zu Server/Lobby abgebrochen
            const std::string errors[] =
            {
                _("Incomplete message was received!"),
                "",
                "",
                _("Lost connection to server!")
            };

            WindowManager::inst().Show(new iwMsgbox(_("Error"), errors[ce], this, MSB_OK, MSB_EXCLAMATIONRED, id));
        } break;
        default: break;
    }
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  (Lobby-)Status: Benutzerdefinierter Fehler (kann auch Conn-Loss o.ä sein)
 *
 *  @author FloSoft
 */
void dskSelectMap::LC_Status_Error(const std::string& error)
{
    WindowManager::inst().Show(new iwMsgbox(_("Error"), error, this, MSB_OK, MSB_EXCLAMATIONRED, 0));
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  (Lobby-)Server wurde erstellt.
 *
 *  @author FloSoft
 */
void dskSelectMap::LC_Created(void)
{
    // ggf. im nächstes Stadium weiter
    GAMESERVER.Start();
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  Callbackfunktion zum Eintragen einer Karte in der Tabelle.
 *
 *  @param[in] filename Der Dateiname
 *  @param[in] param    Ein aufrufsabhängiger Parameter
 *
 *  @author OLiver
 */
void dskSelectMap::FillTable(const std::string& filename, void* param)
{
    ctrlTable* tabelle = (ctrlTable*)param;
    char players[64], size[32];
    libsiedler2::ArchivInfo map;

    // Ist die Tabelle gültig?
    if(tabelle == NULL)
        return;

    // Karteninformationen laden
    if(libsiedler2::loader::LoadMAP(filename.c_str(), &map, true) == 0)
    {
        const libsiedler2::ArchivItem_Map_Header* header = &(dynamic_cast<const glArchivItem_Map*>(map.get(0))->getHeader());
        assert(header);

        if (header->getPlayer() > MAX_PLAYERS)
            return;

        // Und Zeilen vorbereiten
        snprintf(players, 64, _("%d Player"), header->getPlayer());
        snprintf(size, 32, "%dx%d", header->getWidth(), header->getHeight());

        // und einfügen
        const std::string landscapes[3] =
        {
            _("Greenland"),
            _("Wasteland"),
            _("Winter world")
        };

        tabelle->AddRow(0, header->getName(), header->getAuthor(), players, landscapes[header->getGfxSet()].c_str(), size, filename.c_str());
    }
}

