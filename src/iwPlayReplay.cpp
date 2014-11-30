// $Id: iwPlayReplay.cpp 9517 2014-11-30 09:21:25Z marcus $
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
#include "iwPlayReplay.h"

#include "VideoDriverWrapper.h"
#include "WindowManager.h"
#include "Loader.h"
#include "controls.h"

#include "ListDir.h"
#include "GameClient.h"
#include "files.h"

#include "iwMsgbox.h"
#include "dskGameLoader.h"

///////////////////////////////////////////////////////////////////////////////
// Makros / Defines
#if defined _WIN32 && defined _DEBUG && defined _MSC_VER
#define new new(_NORMAL_BLOCK, THIS_FILE, __LINE__)
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

///////////////////////////////////////////////////////////////////////////////
/** @class iwPlayReplay
 *
 *  Klasse des Replay-Listen-Fensters.
 *
 *  @author OLiver
 */

///////////////////////////////////////////////////////////////////////////////
/**
 *  Konstruktor von @p iwPlayReplay.
 *
 *  @author OLiver
 */

iwPlayReplay::iwPlayReplay(void)
    : IngameWindow(CGI_PLAYREPLAY, 0xFFFF, 0xFFFF, 600, 300, _("Play Replay"), LOADER.GetImageN("resource", 41))
{
    ctrlTable* table = AddTable(0, 20, 30, 560, 220, TC_GREEN2, NormalFont, 5, _("Filename"), 300, ctrlTable::SRT_STRING, _("Stocktaking date"), 220, ctrlTable::SRT_DATE, _("Player"), 360, ctrlTable::SRT_STRING, _("Length"), 120, ctrlTable::SRT_NUMBER, "", 0, ctrlTable::SRT_DEFAULT);

    // Starten
    AddTextButton(1, 195, 260, 100, 22, TC_GREEN2, _("Start"), NormalFont);
    // Aufräumen
    AddTextButton(2, 305, 260, 100, 22, TC_RED1, _("Clear"), NormalFont);

    // Verzeichnis auflisten
    std::string tmp = GetFilePath(FILE_PATHS[51]);
    tmp += "*.rpl";
    ListDir(tmp, false, FillReplayTable, table);

    // Erst einmal nach Dateiname sortieren
    table->SortRows(0);

    GameClient::inst().SetInterface(NULL);
}

void iwPlayReplay::Msg_ButtonClick(const unsigned int ctrl_id)
{
    switch(ctrl_id)
    {
        default: break;
        case 1:
        {
            StartReplay();
        } break;
        case 2:
        {
            // Sicherheitsabfrage, ob der Benutzer auch wirklich alle löschen möchte
            WindowManager::inst().Show( new iwMsgbox(_("Clear"), _("Are you sure to remove all replays?"), this, MSB_YESNO, MSB_QUESTIONRED, 1) );

        } break;
    }
}

void iwPlayReplay::Msg_TableChooseItem(const unsigned ctrl_id, const unsigned short selection)
{
    StartReplay();
}

/// Startet das Replay (aktuell ausgewählter Eintrag)
void iwPlayReplay::StartReplay()
{
    // Mond malen
    LOADER.GetImageN("resource", 33)->Draw(VideoDriverWrapper::inst().GetMouseX(), VideoDriverWrapper::inst().GetMouseY() - 40, 0, 0, 0, 0, 0, 0);
    VideoDriverWrapper::inst().SwapBuffers();

    ctrlTable* table = GetCtrl<ctrlTable>(0);
    if(table->GetSelection() < table->GetRowCount())
    {
        GameWorldViewer* gwv;
        unsigned int error = GAMECLIENT.StartReplay( table->GetItemText(table->GetSelection(), 4), gwv);
        std::string replay_errors[] =
        {
            _("Error while playing replay!"),
            _("Error while opening file!"),
            _("Invalid Replay!"),
            _("Error: Replay is too old!"),
            _("Program version is too old to play that replay!"),
            "",
            _("Temporary map file was not found!")
        };

        if(error)
            WindowManager::inst().Show( new iwMsgbox(_("Error while playing replay!"), replay_errors[error], this, MSB_OK, MSB_EXCLAMATIONRED) );
        else
            WindowManager::inst().Switch(new dskGameLoader(gwv));
    }
}

void iwPlayReplay::Msg_MsgBoxResult(const unsigned msgbox_id, const MsgboxResult mbr)
{
    // Sollen alle Replays gelöscht werden?
    if(mbr == MSR_YES && msgbox_id == 1)
    {
        // Dateien löschen
        std::string tmp = GetFilePath(FILE_PATHS[51]);
        tmp += "*.rpl";
        ListDir(tmp, false, RemoveReplay, 0);

        // Tabelle leeren
        GetCtrl<ctrlTable>(0)->DeleteAllItems();
    }
}



///////////////////////////////////////////////////////////////////////////////
/**
 *  Callbackfunktion zum Eintragen einer Replay-Zeile in der Tabelle.
 *
 *  @param[in] filename Der Dateiname
 *  @param[in] param    Ein benutzerdefinierter Parameter
 *
 *  @todo Noch korrekt dokumentieren (was wird da so übersprungen usw)
 *  @todo Fehlerabfrage der freads!!!
 *
 *  @author OLiver
 */
void iwPlayReplay::FillReplayTable(const std::string& filename, void* param)
{
    Replay replay;

    // Datei laden
    if(!replay.LoadHeader(filename, false))
        return;

    // Zeitstamp benutzen
    char datestring[64];
    TIME.FormatTime(datestring, "%d.%m.%Y - %H:%i", &replay.save_time);

    // Spielernamen auslesen
    std::string tmp_players;
    unsigned char j = 0;
    for(unsigned char i = 0; i < replay.player_count; ++i)
    {
        // Was für ein State, wenn es nen KI Spieler oder ein normaler ist, muss das Zeug ausgelesen werden
        if(replay.players[i].ps == PS_OCCUPIED || replay.players[i].ps == PS_KI)
        {
            // und in unsere "Namensliste" hinzufügen (beim ersten Spieler muss kein Komma hin)
            if(j > 0)
                tmp_players += ", ";

            tmp_players += replay.players[i].name;
            ++j;
        }
    }

    // Dateiname noch rausextrahieren aus dem Pfad
    size_t pos = filename.find_last_of('/');
    if(pos == std::string::npos)
        return;
    std::string extracted_filename = filename.substr(pos + 1);

    char gfl[50];
    snprintf(gfl, 50, "%u", replay.last_gf);

    // Und das Zeug zur Tabelle hinzufügen
    static_cast<ctrlTable*>(param)->AddRow(0, extracted_filename.c_str(), datestring, tmp_players.c_str(), gfl, filename.c_str());
}


void iwPlayReplay::RemoveReplay(const std::string& filename, void* param)
{
    // und tschüss
    unlink(filename.c_str());
}
