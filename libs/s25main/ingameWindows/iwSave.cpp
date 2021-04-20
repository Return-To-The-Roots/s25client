// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "iwSave.h"
#include "ListDir.h"
#include "Loader.h"
#include "RttrConfig.h"
#include "Savegame.h"
#include "Settings.h"
#include "WindowManager.h"
#include "controls/ctrlComboBox.h"
#include "controls/ctrlEdit.h"
#include "controls/ctrlTable.h"
#include "desktops/dskLobby.h"
#include "files.h"
#include "helpers/make_array.h"
#include "helpers/toString.h"
#include "iwPleaseWait.h"
#include "network/GameClient.h"
#include "gameData/GameConsts.h"
#include "gameData/const_gui_ids.h"
#include "liblobby/LobbyClient.h"
#include "s25util/Log.h"
#include <utility>

using namespace std::chrono_literals;
const auto AUTO_SAVE_INTERVALS = helpers::make_array(1min, 5min, 10min, 15min, 30min, 60min, 90min);

iwSaveLoad::iwSaveLoad(const unsigned short add_height, const std::string& window_title)
    : IngameWindow(CGI_SAVE, IngameWindow::posLastOrCenter, Extent(600, 400 + add_height), window_title,
                   LOADER.GetImageN("resource", 41))
{
    using SRT = ctrlTable::SortType;
    AddTable(0, DrawPoint(20, 30), Extent(560, 300), TextureColor::Green2, NormalFont,
             ctrlTable::Columns{{_("Filename"), 270, SRT::String},
                                {_("Map"), 250, SRT::String},
                                {_("Time"), 250, SRT::Date},
                                {_("Game Time"), 320, SRT::Time},
                                {}});
}

void iwSaveLoad::Msg_EditEnter(const unsigned /*ctrl_id*/)
{
    SaveLoad();
}

void iwSaveLoad::Msg_ButtonClick(const unsigned /*ctrl_id*/)
{
    SaveLoad();
}

void iwSaveLoad::Msg_TableSelectItem(const unsigned /*ctrl_id*/, const boost::optional<unsigned>& selection)
{
    // Dateiname ins Edit schreiben, wenn wir entsprechende Einträge auswählen
    GetCtrl<ctrlEdit>(1)->SetText(selection ? GetCtrl<ctrlTable>(0)->GetItemText(*selection, 0) : "");
}

void iwSaveLoad::RefreshTable()
{
    static bool loadedOnce = false;

    GetCtrl<ctrlTable>(0)->DeleteAllItems();

    std::vector<boost::filesystem::path> saveFiles = ListDir(RTTRCONFIG.ExpandPath(s25::folders::save), "sav");
    for(const auto& saveFile : saveFiles)
    {
        Savegame save;

        // Datei öffnen
        if(!save.Load(saveFile, SaveGameDataToLoad::Header))
        {
            // Show errors only first time this is loaded
            if(!loadedOnce)
            {
                LOG.write(_("Invalid Savegame %1%! Reason: %2%\n")) % saveFile
                  % (save.GetLastErrorMsg().empty() ? _("Unknown") : save.GetLastErrorMsg());
            }
            continue;
        }

        // Zeitstring erstellen
        std::string dateStr = s25util::Time::FormatTime("%d.%m.%Y - %H:%i", save.GetSaveTime());

        // Dateiname noch rausextrahieren aus dem Pfad
        if(!saveFile.has_filename())
            continue;
        // Just filename w/o extension
        const auto fileName = saveFile.stem().string();

        std::string gameTime = GAMECLIENT.FormatGFTime(save.start_gf);

        // Und das Zeug zur Tabelle hinzufügen
        GetCtrl<ctrlTable>(0)->AddRow({fileName, save.GetMapName(), dateStr, gameTime, saveFile.string()});
    }

    // Nach Zeit Sortieren
    GetCtrl<ctrlTable>(0)->SortRows(2, TableSortDir::Descending);
    loadedOnce = true;
}

void iwSaveLoad::FillSaveTable(const boost::filesystem::path&, void*) {}

void iwSave::SaveLoad()
{
    // Speichern
    const boost::filesystem::path savePath =
      RTTRCONFIG.ExpandPath(s25::folders::save) / (GetCtrl<ctrlEdit>(1)->GetText() + ".sav");

    // Speichern
    GAMECLIENT.SaveToFile(savePath);

    // Aktualisieren
    RefreshTable();

    // Edit wieder leeren
    GetCtrl<ctrlEdit>(1)->SetText("");
}

iwSave::iwSave() : iwSaveLoad(40, _("Save game!"))
{
    AddEdit(1, DrawPoint(20, 390), Extent(510, 22), TextureColor::Green2, NormalFont);
    AddImageButton(2, DrawPoint(540, 386), Extent(40, 40), TextureColor::Green2, LOADER.GetImageN("io", 47));

    // Autospeicherzeug
    AddText(3, DrawPoint(20, 350), _("Auto-Save every:"), 0xFFFFFF00, FontStyle{}, NormalFont);
    ctrlComboBox* combo = AddComboBox(4, DrawPoint(270, 345), Extent(130, 22), TextureColor::Green2, NormalFont, 100);

    /// Combobox füllen
    combo->AddString(_("Disabled")); // deaktiviert

    // Die Intervalle
    for(const std::chrono::minutes interval : AUTO_SAVE_INTERVALS)
        combo->AddString((boost::format(_("%1% min")) % interval.count()).str());

    // Last entry is only for debugging
    if(SETTINGS.global.debugMode)
    {
        combo->AddString(_("Every GF"));
    }

    // Richtigen Eintrag auswählen
    bool found = false;
    for(unsigned i = 0; i < AUTO_SAVE_INTERVALS.size(); ++i)
    {
        if(SETTINGS.interface.autosave_interval == AUTO_SAVE_INTERVALS[i] / SPEED_GF_LENGTHS[referenceSpeed])
        {
            combo->SetSelection(i + 1);
            found = true;
            break;
        }
    }
    if(SETTINGS.interface.autosave_interval == 1)
    {
        combo->SetSelection(AUTO_SAVE_INTERVALS.size() + 1);
        found = true;
    }

    // Ungültig oder 0 --> Deaktiviert auswählen
    if(!found)
        combo->SetSelection(0);

    // Tabelle ausfüllen beim Start
    RefreshTable();
}

void iwSave::Msg_ComboSelectItem(const unsigned /*ctrl_id*/, const unsigned selection)
{
    // Erster Eintrag --> deaktiviert
    if(selection == 0)
        SETTINGS.interface.autosave_interval = 0;
    else if(selection >= AUTO_SAVE_INTERVALS.size())
        SETTINGS.interface.autosave_interval = 1;
    else
    {
        // ansonsten jeweilige GF-Zahl eintragen
        SETTINGS.interface.autosave_interval = AUTO_SAVE_INTERVALS[selection - 1] / SPEED_GF_LENGTHS[referenceSpeed];
    }
}

iwLoad::iwLoad(CreateServerInfo csi) : iwSaveLoad(0, _("Load game!")), csi(std::move(csi))
{
    AddEdit(1, DrawPoint(20, 350), Extent(510, 22), TextureColor::Green2, NormalFont);
    AddImageButton(2, DrawPoint(540, 346), Extent(40, 40), TextureColor::Green2, LOADER.GetImageN("io", 48));
    // Tabelle ausfüllen beim Start
    RefreshTable();
}

/**
 *  Spiel laden.
 */
void iwLoad::SaveLoad()
{
    // Server starten
    auto* table = GetCtrl<ctrlTable>(0);
    if(!table->GetSelection())
        return;

    if(!GAMECLIENT.HostGame(csi, table->GetItemText(*table->GetSelection(), 4), MapType::Savegame))
    {
        // Server starten
        if(LOBBYCLIENT.IsLoggedIn())
            // Lobby zeigen, wenn wenn das nich ging und man im Lobby-Modus ist
            WINDOWMANAGER.Switch(std::make_unique<dskLobby>());
        else
            // Ansonsten schließen
            Close();
    } else
    {
        // Verbindungsfenster anzeigen
        WINDOWMANAGER.Show(std::make_unique<iwPleaseWait>());
    }
}

/// Handle double click on the table
void iwLoad::Msg_TableChooseItem(const unsigned /*ctrl_id*/, const unsigned /*selection*/)
{
    SaveLoad();
}
