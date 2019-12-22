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
#include "iwPlayReplay.h"
#include "ListDir.h"
#include "Loader.h"
#include "Replay.h"
#include "RttrConfig.h"
#include "WindowManager.h"
#include "controls/ctrlTable.h"
#include "controls/ctrlTextButton.h"
#include "desktops/dskGameLoader.h"
#include "drivers/VideoDriverWrapper.h"
#include "files.h"
#include "helpers/toString.h"
#include "iwMsgbox.h"
#include "network/GameClient.h"
#include "ogl/glArchivItem_Bitmap.h"
#include "gameData/const_gui_ids.h"
#include "s25util/Log.h"
#include <boost/filesystem.hpp>

namespace {
class SwitchOnStart : public ClientInterface
{
public:
    SwitchOnStart() { GAMECLIENT.SetInterface(this); }
    ~SwitchOnStart() override { GAMECLIENT.RemoveInterface(this); }

    void CI_GameLoading(const std::shared_ptr<Game>& game) override { WINDOWMANAGER.Switch(std::make_unique<dskGameLoader>(game)); }
};

std::vector<std::string> GetReplays()
{
    return ListDir(RTTRCONFIG.ExpandPath(FILE_PATHS[51]), "rpl");
}
} // namespace

iwPlayReplay::iwPlayReplay()
    : IngameWindow(CGI_PLAYREPLAY, IngameWindow::posLastOrCenter, Extent(600, 330), _("Play Replay"), LOADER.GetImageN("resource", 41))
{
    using SRT = ctrlTable::SortType;
    AddTable(0, DrawPoint(20, 30), Extent(560, 220), TC_GREEN2, NormalFont,
             ctrlTable::Columns{{("Filename"), 300, SRT::String},
                                {_("Stocktaking date"), 220, SRT::Date},
                                {_("Player"), 360, SRT::String},
                                {_("Length"), 120, SRT::Number},
                                {}});

    AddTextButton(2, DrawPoint(20, 260), Extent(100, 22), TC_RED1, _("Clear"), NormalFont);
    AddTextButton(5, DrawPoint(130, 260), Extent(160, 22), TC_RED1, "Delete Invalid", NormalFont,
                  _("Removes all replays that cannot be loaded with the current game version"));
    AddTextButton(3, DrawPoint(20, 290), Extent(160, 22), TC_RED1, _("Delete selected"), NormalFont);
    AddTextButton(4, DrawPoint(190, 290), Extent(190, 22), TC_RED1, _("Back"), NormalFont);
    AddTextButton(1, DrawPoint(390, 290), Extent(190, 22), TC_GREEN2, _("Start"), NormalFont);

    PopulateTable();

    // Nach Zeit Sortieren
    bool bFalse = false;
    GetCtrl<ctrlTable>(0)->SortRows(1, &bFalse);
}

void iwPlayReplay::PopulateTable()
{
    static bool loadedOnce = false;

    auto* table = GetCtrl<ctrlTable>(0);
    unsigned short sortCol = table->GetSortColumn();
    if(sortCol == 0xFFFF)
        sortCol = 0;
    bool sortDir = table->GetSortDirection();
    table->DeleteAllItems();

    unsigned numInvalid = 0;

    std::vector<std::string> replays = GetReplays();
    for(auto& it : replays)
    {
        Replay replay;

        // Datei laden
        if(!replay.LoadHeader(it, false))
        {
            // Show errors only first time this is loaded
            if(!loadedOnce)
            {
                LOG.write(_("Invalid Replay %1%! Reason: %2%\n")) % it
                  % (replay.GetLastErrorMsg().empty() ? _("Unknown") : replay.GetLastErrorMsg());
            }
            numInvalid++;
            continue;
        }

        // Zeitstamp benutzen
        std::string dateStr = s25util::Time::FormatTime("%d.%m.%Y - %H:%i", replay.GetSaveTime());

        // Spielernamen auslesen
        std::string tmp_players;
        for(const std::string& playerName : replay.GetPlayerNames())
        {
            if(!tmp_players.empty())
                tmp_players += ", ";

            tmp_players += playerName;
        }

        // Dateiname noch rausextrahieren aus dem Pfad
        bfs::path path = it;
        if(!path.has_filename())
            continue;
        std::string fileName = path.filename().string();
        std::string lastGF = helpers::toString(replay.GetLastGF());

        // Und das Zeug zur Tabelle hinzufügen
        table->AddRow({fileName, dateStr, tmp_players, lastGF, it});
    }

    // Erst einmal nach Dateiname sortieren
    table->SortRows(sortCol, &sortDir);

    auto* btDelInvalid = GetCtrl<ctrlTextButton>(5);
    if(numInvalid == 0)
        btDelInvalid->SetVisible(false);
    else
    {
        btDelInvalid->SetVisible(true);
        btDelInvalid->SetText((boost::format(_("Delete Invalid (%u)")) % numInvalid).str());
    }
    loadedOnce = true;
}

void iwPlayReplay::Msg_ButtonClick(const unsigned ctrl_id)
{
    switch(ctrl_id)
    {
        default: break;
        case 1: StartReplay(); break;
        case 2:
            WINDOWMANAGER.Show(
              std::make_unique<iwMsgbox>(_("Clear"), _("Are you sure to remove all replays?"), this, MSB_YESNO, MSB_QUESTIONRED, 1));
            break;
        case 3:
        {
            auto* table = GetCtrl<ctrlTable>(0);
            if(table->GetSelection() < table->GetNumRows())
                WINDOWMANAGER.Show(std::make_unique<iwMsgbox>(
                  _("Delete selected"), _("Are you sure you want to remove the selected replay?"), this, MSB_YESNO, MSB_QUESTIONRED, 2));
            break;
        }
        case 4: Close(); break;
        case 5:
            WINDOWMANAGER.Show(std::make_unique<iwMsgbox>(_("Clear"), _("Are you sure to remove all invalid replays?"), this, MSB_YESNO,
                                                          MSB_QUESTIONRED, 3));
            break;
    }
}

void iwPlayReplay::Msg_TableChooseItem(const unsigned /*ctrl_id*/, const unsigned /*selection*/)
{
    StartReplay();
}

/// Startet das Replay (aktuell ausgewählter Eintrag)
void iwPlayReplay::StartReplay()
{
    // Mond malen
    LOADER.GetImageN("resource", 33)->DrawFull(VIDEODRIVER.GetMousePos() - DrawPoint(0, 40));
    VIDEODRIVER.SwapBuffers();

    auto* table = GetCtrl<ctrlTable>(0);
    if(table->GetSelection() < table->GetNumRows())
    {
        SwitchOnStart switchOnStart;
        if(!GAMECLIENT.StartReplay(table->GetItemText(table->GetSelection(), 4)))
            WINDOWMANAGER.Show(
              std::make_unique<iwMsgbox>(_("Error while playing replay!"), _("Invalid Replay!"), this, MSB_OK, MSB_EXCLAMATIONRED));
    }
}

void iwPlayReplay::Msg_MsgBoxResult(const unsigned msgbox_id, const MsgboxResult mbr)
{
    // Sollen alle Replays gelöscht werden?
    if(msgbox_id == 1 && mbr == MSR_YES)
    {
        std::vector<std::string> replays = GetReplays();
        for(auto& replay : replays)
        {
            boost::system::error_code ec;
            bfs::remove(replay, ec);
        }

        // Tabelle leeren
        GetCtrl<ctrlTable>(0)->DeleteAllItems();
    } else if(msgbox_id == 3 && mbr == MSR_YES)
    {
        std::vector<std::string> replays = GetReplays();
        for(auto& it : replays)
        {
            Replay replay;
            if(!replay.LoadHeader(it, false))
            {
                replay.Close();
                boost::system::error_code ec;
                bfs::remove(it, ec);
            }
        }

        PopulateTable();
    } else if(msgbox_id == 2 && mbr == MSR_YES)
    {
        auto* table = GetCtrl<ctrlTable>(0);
        if(table->GetSelection() < table->GetNumRows())
        {
            boost::system::error_code ec;
            bfs::remove(table->GetItemText(table->GetSelection(), 4), ec);
            PopulateTable();
        }
    }
}
