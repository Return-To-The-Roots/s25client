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
#include "iwSave.h"

#include "WindowManager.h"
#include "Loader.h"

#include "ListDir.h"
#include "GameClient.h"
#include "files.h"
#include "fileFuncs.h"
#include "GameServer.h"
#include "LobbyClient.h"
#include "desktops/dskLobby.h"
#include "GameSavegame.h"
#include "controls/ctrlComboBox.h"
#include "controls/ctrlEdit.h"
#include "controls/ctrlTable.h"
#include "iwPleaseWait.h"
#include "gameData/const_gui_ids.h"
#include "Settings.h"
#include "helpers/converters.h"
#include <boost/filesystem.hpp>

// Include last!
#include "DebugNew.h" // IWYU pragma: keep


const unsigned AUTO_SAVE_INTERVALS_COUNT = 7;

const unsigned AUTO_SAVE_INTERVALS[AUTO_SAVE_INTERVALS_COUNT] =
{
    500, 1000, 5000, 10000, 50000, 100000, 1
};

iwSaveLoad::iwSaveLoad(const unsigned short add_height, const std::string& window_title)
    : IngameWindow(CGI_SAVE, 0xFFFF, 0xFFFF, 600, 400 + add_height, window_title, LOADER.GetImageN("resource", 41))
{
    AddTable(0, 20, 30, 560, 300, TC_GREEN2, NormalFont, 5, _("Filename"), 270, ctrlTable::SRT_STRING, _("Map"), 250, ctrlTable::SRT_STRING, _("Time"), 250, ctrlTable::SRT_DATE, _("Start GF"), 320, ctrlTable::SRT_NUMBER,  "", 0, ctrlTable::SRT_STRING);
}

void iwSaveLoad::Msg_EditEnter(const unsigned int  /*ctrl_id*/)
{
    SaveLoad();
}

void iwSaveLoad::Msg_ButtonClick(const unsigned int  /*ctrl_id*/)
{
    SaveLoad();
}

void iwSaveLoad::Msg_TableSelectItem(const unsigned int  /*ctrl_id*/, const int selection)
{
    // Dateiname ins Edit schreiben, wenn wir entsprechende Einträge auswählen
    GetCtrl<ctrlEdit>(1)->SetText(GetCtrl<ctrlTable>(0)->GetItemText(selection, 0));
}

void iwSaveLoad::RefreshTable()
{
    GetCtrl<ctrlTable>(0)->DeleteAllItems();

    std::vector<std::string> saveFiles = ListDir(GetFilePath(FILE_PATHS[85]), "sav");
    for(std::vector<std::string>::iterator it = saveFiles.begin(); it != saveFiles.end(); ++it)
    {
        Savegame save;

        // Datei öffnen
        if(!save.Load(*it, false, false))
            continue;

        // Zeitstring erstellen
        std::string dateStr = TIME.FormatTime("%d.%m.%Y - %H:%i", &save.save_time);

        // Dateiname noch rausextrahieren aus dem Pfad
        bfs::path path = *it;
        if(!path.has_filename())
            continue;
        bfs::path fileName = path.filename();

        // ".sav" am Ende weg
        RTTR_Assert(fileName.has_extension());
        fileName.replace_extension();

        std::string fileNameStr = cvWideStringToUTF8(fileName.wstring());
        std::string startGF = helpers::toString(save.start_gf);

        // Und das Zeug zur Tabelle hinzufügen
        GetCtrl<ctrlTable>(0)->AddRow(0, fileNameStr.c_str(), save.mapName.c_str(), dateStr.c_str(), startGF.c_str(), it->c_str());
    }

    // Nach Zeit Sortieren
    GetCtrl<ctrlTable>(0)->SortRows(2);
}

void iwSaveLoad::FillSaveTable(const std::string& filePath, void* param)
{
    
}

void iwSave::SaveLoad()
{
    // Speichern
    std::string tmp = GetFilePath(FILE_PATHS[85]);
    tmp += GetCtrl<ctrlEdit>(1)->GetText();
    tmp += ".sav";

    // Speichern
    GAMECLIENT.SaveToFile(tmp);

    // Aktualisieren
    RefreshTable();

    // Edit wieder leeren
    GetCtrl<ctrlEdit>(1)->SetText("");
}

iwSave::iwSave() : iwSaveLoad(40, _("Save game!"))
{
    AddEdit(1, 20, 390, 510, 22, TC_GREEN2, NormalFont);
    AddImageButton(2, 540, 386, 40, 40, TC_GREEN2, LOADER.GetImageN("io", 47));

    // Autospeicherzeug
    AddText(3, 20, 350, _("Auto-Save every:"), 0xFFFFFF00, 0, NormalFont);
    ctrlComboBox* combo = AddComboBox(4, 270, 345, 130, 22, TC_GREEN2, NormalFont, 100);

    /// Combobox füllen
    combo->AddString(_("Disabled")); // deaktiviert
    
    // Last entry is only for debugging
    const unsigned numIntervalls = SETTINGS.global.debugMode ? AUTO_SAVE_INTERVALS_COUNT : AUTO_SAVE_INTERVALS_COUNT - 1;

    // Die Intervalle
    for(unsigned i = 0; i < numIntervalls; ++i)
    {
        char str[64];
        sprintf(str, "%u GF", AUTO_SAVE_INTERVALS[i]);
        combo->AddString(str);
    }

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

void iwSave::Msg_ComboSelectItem(const unsigned int  /*ctrl_id*/, const int selection)
{

    // Erster Eintrag --> deaktiviert
    if(selection == 0)
        SETTINGS.interface.autosave_interval = 0;
    else
        // ansonsten jeweilige GF-Zahl eintragen
        SETTINGS.interface.autosave_interval = AUTO_SAVE_INTERVALS[selection - 1];
}

iwLoad::iwLoad(const CreateServerInfo& csi) : iwSaveLoad(0, _("Load game!")),  csi(csi)
{
    AddEdit(1, 20, 350, 510, 22, TC_GREEN2, NormalFont);
    AddImageButton(2, 540, 346, 40, 40, TC_GREEN2, LOADER.GetImageN("io", 48));
    // Tabelle ausfüllen beim Start
    RefreshTable();
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  Spiel laden.
 *
 *  @author OLiver
 */
void iwLoad::SaveLoad()
{
    // Server starten
    ctrlTable* table = GetCtrl<ctrlTable>(0);

    if(!GAMESERVER.TryToStart(csi, table->GetItemText(table->GetSelection(), 4), MAPTYPE_SAVEGAME))
    {
        // Server starten
        if(LOBBYCLIENT.LoggedIn())
            // Lobby zeigen, wenn wenn das nich ging und man im Lobby-Modus ist
            WINDOWMANAGER.Switch(new dskLobby);
        else
            // Ansonsten schließen
            Close();
    }
    else
    {
        // Verbindungsfenster anzeigen
        WINDOWMANAGER.Show(new iwPleaseWait);
    }

}


/// Handle double click on the table
void iwLoad::Msg_TableChooseItem(const unsigned ctrl_id, const unsigned selection)
{
    SaveLoad();
}
