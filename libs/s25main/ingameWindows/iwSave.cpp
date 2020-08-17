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
#include "helpers/toString.h"
#include "iwPleaseWait.h"
#include "network/GameClient.h"
#include "gameData/const_gui_ids.h"
#include "liblobby/LobbyClient.h"
#include "s25util/Log.h"
#include <utility>

const unsigned NUM_AUTO_SAVE_INTERVALS = 7;

const std::array<unsigned, NUM_AUTO_SAVE_INTERVALS> AUTO_SAVE_INTERVALS = {500, 1000, 5000, 10000, 50000, 100000, 1 /*  */};

iwSaveLoad::iwSaveLoad(const unsigned short add_height, const std::string& window_title)
    : IngameWindow(CGI_SAVE, IngameWindow::posLastOrCenter, Extent(600, 400 + add_height), window_title, LOADER.GetImageN("resource", 41))
{
    using SRT = ctrlTable::SortType;
    AddTable(0, DrawPoint(20, 30), Extent(560, 300), TC_GREEN2, NormalFont,
             ctrlTable::Columns{{_("Filename"), 270, SRT::String},
                                {_("Map"), 250, SRT::String},
                                {_("Time"), 250, SRT::Date},
                                {_("Start GF"), 320, SRT::Number},
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

        std::string startGF = helpers::toString(save.start_gf);

        // Und das Zeug zur Tabelle hinzufügen
        GetCtrl<ctrlTable>(0)->AddRow({fileName, save.GetMapName(), dateStr, startGF, saveFile.string()});
    }

    // Nach Zeit Sortieren
    GetCtrl<ctrlTable>(0)->SortRows(2, TableSortDir::Descending);
    loadedOnce = true;
}

void iwSaveLoad::FillSaveTable(const boost::filesystem::path&, void*) {}

void iwSave::SaveLoad()
{
    // Speichern
    const boost::filesystem::path savePath = RTTRCONFIG.ExpandPath(s25::folders::save) / (GetCtrl<ctrlEdit>(1)->GetText() + ".sav");

    // Speichern
    GAMECLIENT.SaveToFile(savePath);

    // Aktualisieren
    RefreshTable();

    // Edit wieder leeren
    GetCtrl<ctrlEdit>(1)->SetText("");
}

iwSave::iwSave() : iwSaveLoad(40, _("Save game!"))
{
    AddEdit(1, DrawPoint(20, 390), Extent(510, 22), TC_GREEN2, NormalFont);
    AddImageButton(2, DrawPoint(540, 386), Extent(40, 40), TC_GREEN2, LOADER.GetImageN("io", 47));

    // Autospeicherzeug
    AddText(3, DrawPoint(20, 350), _("Auto-Save every:"), 0xFFFFFF00, FontStyle{}, NormalFont);
    ctrlComboBox* combo = AddComboBox(4, DrawPoint(270, 345), Extent(130, 22), TC_GREEN2, NormalFont, 100);

    /// Combobox füllen
    combo->AddString(_("Disabled")); // deaktiviert

    // Last entry is only for debugging
    const unsigned numIntervalls = SETTINGS.global.debugMode ? NUM_AUTO_SAVE_INTERVALS : NUM_AUTO_SAVE_INTERVALS - 1;

    // Die Intervalle
    for(unsigned i = 0; i < numIntervalls; ++i)
        combo->AddString(helpers::toString(AUTO_SAVE_INTERVALS[i]) + " GF");

    // Richtigen Eintrag auswählen
    bool found = false;
    for(unsigned i = 0; i < numIntervalls; ++i)
    {
        if(SETTINGS.interface.autosave_interval == AUTO_SAVE_INTERVALS[i])
        {
            combo->SetSelection(i + 1);
            found = true;
            break;
        }
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
    else
        // ansonsten jeweilige GF-Zahl eintragen
        SETTINGS.interface.autosave_interval = AUTO_SAVE_INTERVALS[selection - 1];
}

iwLoad::iwLoad(CreateServerInfo csi) : iwSaveLoad(0, _("Load game!")), csi(std::move(csi))
{
    AddEdit(1, DrawPoint(20, 350), Extent(510, 22), TC_GREEN2, NormalFont);
    AddImageButton(2, DrawPoint(540, 346), Extent(40, 40), TC_GREEN2, LOADER.GetImageN("io", 48));
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

    if(!GAMECLIENT.HostGame(csi, table->GetItemText(*table->GetSelection(), 4), MAPTYPE_SAVEGAME))
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
