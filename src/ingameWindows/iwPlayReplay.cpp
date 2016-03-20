// Copyright (c) 2005 - 2015 Settlers Freaks (sf-team at siedler25.org)
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
#include "iwPlayReplay.h"

#include "drivers/VideoDriverWrapper.h"
#include "WindowManager.h"
#include "Loader.h"

#include "ListDir.h"
#include "GameClient.h"
#include "files.h"
#include "fileFuncs.h"
#include "controls/ctrlTable.h"
#include "controls/ctrlButton.h"
#include "iwMsgbox.h"
#include "desktops/dskGameLoader.h"
#include "ogl/glArchivItem_Bitmap.h"
#include "gameData/const_gui_ids.h"
#include "helpers/converters.h"
#include <boost/filesystem.hpp>

// Include last!
#include "DebugNew.h" // IWYU pragma: keep
class GameWorldViewer;

std::vector<std::string> GetReplays()
{
    return ListDir(GetFilePath(FILE_PATHS[51]), "rpl");
}

iwPlayReplay::iwPlayReplay()
    : IngameWindow(CGI_PLAYREPLAY, 0xFFFF, 0xFFFF, 600, 330, _("Play Replay"), LOADER.GetImageN("resource", 41))
{
    AddTable(0, 20, 30, 560, 220, TC_GREEN2, NormalFont, 5, _("Filename"), 300, ctrlTable::SRT_STRING, _("Stocktaking date"), 220, ctrlTable::SRT_DATE, _("Player"), 360, ctrlTable::SRT_STRING, _("Length"), 120, ctrlTable::SRT_NUMBER, "", 0, ctrlTable::SRT_DEFAULT);

    AddTextButton(2, 20, 260, 100, 22, TC_RED1, _("Clear"), NormalFont);
    AddTextButton(5, 130, 260, 160, 22, TC_RED1, "Delete Invalid", NormalFont, _("Removes all replays that cannot be loaded with the current game version"));
    AddTextButton(3, 20, 290, 160, 22, TC_RED1, _("Delete selected"), NormalFont);
    AddTextButton(4, 190, 290, 190, 22, TC_RED1, _("Back"), NormalFont);
    AddTextButton(1, 390, 290, 190, 22, TC_GREEN2, _("Start"), NormalFont);

    PopulateTable();

    GAMECLIENT.SetInterface(NULL);
}

void iwPlayReplay::PopulateTable()
{
    ctrlTable* table = GetCtrl<ctrlTable>(0);
    unsigned short sortCol = table->GetSortColumn();
    if(sortCol == 0xFFFF)
        sortCol = 0;
    bool sortDir = table->GetSortDirection();
    table->DeleteAllItems();

    unsigned numInvalid = 0;

    std::vector<std::string> replays = GetReplays();
    for(std::vector<std::string>::iterator it = replays.begin(); it != replays.end(); ++it)
    {
        Replay replay;

        // Datei laden
        if(!replay.LoadHeader(*it))
        {
            numInvalid++;
            continue;
        }

        // Zeitstamp benutzen
        std::string dateStr = TIME.FormatTime("%d.%m.%Y - %H:%i", &replay.save_time);

        // Spielernamen auslesen
        std::string tmp_players;
        for(unsigned char i = 0; i < replay.GetPlayerCount(); ++i)
        {
            // Was für ein State, wenn es nen KI Spieler oder ein normaler ist, muss das Zeug ausgelesen werden
            const SavedFile::Player& curPlayer = replay.GetPlayer(i);
            if(curPlayer.ps == PS_OCCUPIED || curPlayer.ps == PS_KI)
            {
                // und in unsere "Namensliste" hinzufügen (beim ersten Spieler muss kein Komma hin)
                if(!tmp_players.empty())
                    tmp_players += ", ";

                tmp_players += curPlayer.name;
            }
        }

        // Dateiname noch rausextrahieren aus dem Pfad
        bfs::path path = *it;
        if(!path.has_filename())
            continue;
        std::string fileName = path.filename().string();
        std::string lastGF = helpers::toString(replay.lastGF_);

        // Und das Zeug zur Tabelle hinzufügen
        table->AddRow(0, fileName.c_str(), dateStr.c_str(), tmp_players.c_str(), lastGF.c_str(), it->c_str());
    }

    // Erst einmal nach Dateiname sortieren
    table->SortRows(sortCol, &sortDir);

    ctrlTextButton* btDelInvalid = GetCtrl<ctrlTextButton>(5);
    if(numInvalid == 0)
        btDelInvalid->SetVisible(false);
    else
    {
        btDelInvalid->SetVisible(true);
        char text[255];
        snprintf(text, 255, _("Delete Invalid (%u)"), numInvalid);
        btDelInvalid->SetText(text);
    }
}

void iwPlayReplay::Msg_ButtonClick(const unsigned int ctrl_id)
{
    switch(ctrl_id)
    {
        default: break;
        case 1:
            StartReplay();
            break;
        case 2:
            WINDOWMANAGER.Show( new iwMsgbox(_("Clear"), _("Are you sure to remove all replays?"), this, MSB_YESNO, MSB_QUESTIONRED, 1) );
            break;
        case 3:
        {
            ctrlTable* table = GetCtrl<ctrlTable>(0);
            if(table->GetSelection() < table->GetRowCount())
                WINDOWMANAGER.Show(new iwMsgbox(_("Delete selected"), _("Are you sure you want to remove the selected replay?"), this, MSB_YESNO, MSB_QUESTIONRED, 2));
            break;
        }
        case 4:
            Close();
            break;
        case 5:
            WINDOWMANAGER.Show(new iwMsgbox(_("Clear"), _("Are you sure to remove all invalid replays?"), this, MSB_YESNO, MSB_QUESTIONRED, 3));
            break;
    }
}

void iwPlayReplay::Msg_TableChooseItem(const unsigned ctrl_id, const unsigned selection)
{
    StartReplay();
}

/// Startet das Replay (aktuell ausgewählter Eintrag)
void iwPlayReplay::StartReplay()
{
    // Mond malen
    LOADER.GetImageN("resource", 33)->Draw(VIDEODRIVER.GetMouseX(), VIDEODRIVER.GetMouseY() - 40, 0, 0, 0, 0, 0, 0);
    VIDEODRIVER.SwapBuffers();

    ctrlTable* table = GetCtrl<ctrlTable>(0);
    if(table->GetSelection() < table->GetRowCount())
    {
        GameWorldViewer* gwv;
        unsigned int error = GAMECLIENT.StartReplay( table->GetItemText(table->GetSelection(), 4), gwv);
        boost::array<std::string, 6> replay_errors =
        {{
            _("Error while playing replay!"),
            _("Error while opening file!"),
            _("Invalid Replay!"),
            _("Error: Replay is too old!"),
            _("Program version is too old to play that replay!"),
            _("Temporary map file was not found!")
        }};

        if(error)
            WINDOWMANAGER.Show( new iwMsgbox(_("Error while playing replay!"), replay_errors[error-1], this, MSB_OK, MSB_EXCLAMATIONRED) );
        else
            WINDOWMANAGER.Switch(new dskGameLoader(gwv));
    }
}

void iwPlayReplay::Msg_MsgBoxResult(const unsigned msgbox_id, const MsgboxResult mbr)
{
    // Sollen alle Replays gelöscht werden?
    if(msgbox_id == 1 && mbr == MSR_YES)
    {
        std::vector<std::string> replays = GetReplays();
        for(std::vector<std::string>::iterator it = replays.begin(); it != replays.end(); ++it)
        {
            boost::system::error_code ec;
            bfs::remove(*it, ec);
        }

        // Tabelle leeren
        GetCtrl<ctrlTable>(0)->DeleteAllItems();
    } else if(msgbox_id == 3 && mbr == MSR_YES)
    {
        std::vector<std::string> replays = GetReplays();
        for(std::vector<std::string>::iterator it = replays.begin(); it != replays.end(); ++it)
        {
            Replay replay;
            if(!replay.LoadHeader(*it))
            {
                replay.StopRecording();
                boost::system::error_code ec;
                bfs::remove(*it, ec);
            }
        }

        PopulateTable();
    } else if(msgbox_id == 2 && mbr == MSR_YES)
    {
        ctrlTable* table = GetCtrl<ctrlTable>(0);
        if(table->GetSelection() < table->GetRowCount())
        {
            boost::system::error_code ec;
            bfs::remove(table->GetItemText(table->GetSelection(), 4), ec);
            PopulateTable();
        }
    }
}
