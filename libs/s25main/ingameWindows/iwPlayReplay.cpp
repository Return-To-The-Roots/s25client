// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

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

namespace bfs = boost::filesystem;

namespace {
class SwitchOnStart : public ClientInterface
{
public:
    SwitchOnStart() { GAMECLIENT.setInterface(this); }
    ~SwitchOnStart() override { GAMECLIENT.removeInterface(this); }

    void CI_GameLoading(std::shared_ptr<Game> game) override
    {
        WINDOWMANAGER.switchTo(std::make_unique<dskGameLoader>(std::move(game)));
    }
};

std::vector<bfs::path> GetReplays()
{
    return ListDir(RTTRCONFIG.ExpandPath(s25::folders::replays), "rpl");
}
} // namespace

iwPlayReplay::iwPlayReplay()
    : IngameWindow(CGI_PLAYREPLAY, IngameWindow::posLastOrCenter, Extent(600, 330), _("Play Replay"),
                   LOADER.getImageN("resource", 41))
{
    using SRT = ctrlTable::SortType;
    addTable(0, DrawPoint(20, 30), Extent(560, 220), TextureColor::Green2, NormalFont,
             ctrlTable::Columns{{("Filename"), 300, SRT::String},
                                {_("Stocktaking date"), 220, SRT::Date},
                                {_("Player"), 360, SRT::String},
                                {_("Length"), 120, SRT::Time},
                                {}});

    addTextButton(2, DrawPoint(20, 260), Extent(100, 22), TextureColor::Red1, _("Clear"), NormalFont);
    addTextButton(5, DrawPoint(130, 260), Extent(160, 22), TextureColor::Red1, "Delete Invalid", NormalFont,
                  _("Removes all replays that cannot be loaded with the current game version"));
    addTextButton(3, DrawPoint(20, 290), Extent(160, 22), TextureColor::Red1, _("Delete selected"), NormalFont);
    addTextButton(4, DrawPoint(190, 290), Extent(190, 22), TextureColor::Red1, _("Back"), NormalFont);
    addTextButton(1, DrawPoint(390, 290), Extent(190, 22), TextureColor::Green2, _("Start"), NormalFont);

    populateTable();

    // Nach Zeit Sortieren
    getCtrl<ctrlTable>(0)->sortRows(1, TableSortDir::Descending);
}

void iwPlayReplay::populateTable()
{
    static bool loadedOnce = false;

    auto* table = getCtrl<ctrlTable>(0);
    int sortCol = table->getSortColumn();
    if(sortCol == -1)
        sortCol = 0;
    const auto sortDir = table->getSortDirection();
    table->deleteAllItems();

    unsigned numInvalid = 0;

    const std::vector<bfs::path> replays = GetReplays();
    for(const auto& path : replays)
    {
        Replay replay;

        // Datei laden
        if(!replay.loadHeader(path))
        {
            // Show errors only first time this is loaded
            if(!loadedOnce)
            {
                LOG.write(_("Invalid Replay %1%! Reason: %2%\n")) % path
                  % (replay.getLastErrorMsg().empty() ? _("Unknown") : replay.getLastErrorMsg());
            }
            numInvalid++;
            continue;
        }

        // Zeitstamp benutzen
        std::string dateStr = s25util::Time::FormatTime("%d.%m.%Y - %H:%i", replay.getSaveTime());

        // Spielernamen auslesen
        std::string tmp_players;
        for(const std::string& playerName : replay.getPlayerNames())
        {
            if(!tmp_players.empty())
                tmp_players += ", ";

            tmp_players += playerName;
        }

        // Dateiname noch rausextrahieren aus dem Pfad
        if(!path.has_filename())
            continue;
        std::string fileName = path.filename().string();
        std::string length = GAMECLIENT.formatGFTime(replay.getLastGF());

        // Und das Zeug zur Tabelle hinzufügen
        table->addRow({fileName, dateStr, tmp_players, length, path.string()});
    }

    // Erst einmal nach Dateiname sortieren
    table->sortRows(sortCol, sortDir);

    auto* btDelInvalid = getCtrl<ctrlTextButton>(5);
    if(numInvalid == 0)
        btDelInvalid->setVisible(false);
    else
    {
        btDelInvalid->setVisible(true);
        btDelInvalid->setText((boost::format(_("Delete Invalid (%u)")) % numInvalid).str());
    }
    loadedOnce = true;
}

void iwPlayReplay::msgButtonClick(const unsigned ctrl_id)
{
    switch(ctrl_id)
    {
        default: break;
        case 1: startReplay(); break;
        case 2:
            WINDOWMANAGER.show(std::make_unique<iwMsgbox>(_("Clear"), _("Are you sure to remove all replays?"), this,
                                                          MsgboxButton::YesNo, MsgboxIcon::QuestionRed, 1));
            break;
        case 3:
        {
            auto* table = getCtrl<ctrlTable>(0);
            if(table->getSelection())
                WINDOWMANAGER.show(std::make_unique<iwMsgbox>(_("Delete selected"),
                                                              _("Are you sure you want to remove the selected replay?"),
                                                              this, MsgboxButton::YesNo, MsgboxIcon::QuestionRed, 2));
            break;
        }
        case 4: close(); break;
        case 5:
            WINDOWMANAGER.show(std::make_unique<iwMsgbox>(_("Clear"), _("Are you sure to remove all invalid replays?"),
                                                          this, MsgboxButton::YesNo, MsgboxIcon::QuestionRed, 3));
            break;
    }
}

void iwPlayReplay::msgTableChooseItem(const unsigned /*ctrl_id*/, const unsigned /*selection*/)
{
    startReplay();
}

/// Startet das Replay (aktuell ausgewählter Eintrag)
void iwPlayReplay::startReplay()
{
    // Mond malen
    LOADER.getImageN("resource", 33)->drawFull(VIDEODRIVER.getMousePos() - DrawPoint(0, 40));
    VIDEODRIVER.swapBuffers();

    auto* table = getCtrl<ctrlTable>(0);
    if(table->getSelection())
    {
        SwitchOnStart switchOnStart;
        if(!GAMECLIENT.startReplay(table->getItemText(*table->getSelection(), 4)))
            WINDOWMANAGER.show(std::make_unique<iwMsgbox>(_("Error while playing replay!"), _("Invalid Replay!"), this,
                                                          MsgboxButton::Ok, MsgboxIcon::ExclamationRed));
    }
}

void iwPlayReplay::msgMsgBoxResult(const unsigned msgbox_id, const MsgboxResult mbr)
{
    // Sollen alle Replays gelöscht werden?
    if(msgbox_id == 1 && mbr == MsgboxResult::Yes)
    {
        const std::vector<bfs::path> replays = GetReplays();
        for(const auto& replay : replays)
        {
            boost::system::error_code ec;
            bfs::remove(replay, ec);
        }

        // Tabelle leeren
        getCtrl<ctrlTable>(0)->deleteAllItems();
    } else if(msgbox_id == 3 && mbr == MsgboxResult::Yes)
    {
        const std::vector<bfs::path> replays = GetReplays();
        for(const auto& it : replays)
        {
            Replay replay;
            if(!replay.loadHeader(it))
            {
                replay.close();
                boost::system::error_code ec;
                bfs::remove(it, ec);
            }
        }

        populateTable();
    } else if(msgbox_id == 2 && mbr == MsgboxResult::Yes)
    {
        auto* table = getCtrl<ctrlTable>(0);
        if(table->getSelection())
        {
            boost::system::error_code ec;
            bfs::remove(table->getItemText(*table->getSelection(), 4), ec);
            populateTable();
        }
    }
}
