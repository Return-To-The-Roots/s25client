// $Id: iwSave.cpp 9357 2014-04-25 15:35:25Z FloSoft $
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
#include "iwSave.h"

#include "WindowManager.h"
#include "Loader.h"
#include "controls.h"

#include "ListDir.h"
#include "GameClient.h"
#include "files.h"
#include "GameServer.h"
#include "LobbyClient.h"
#include "dskLobby.h"
#include "dskHostGame.h"

#include "iwPleaseWait.h"
#include "iwMsgbox.h"

#include "Settings.h"

///////////////////////////////////////////////////////////////////////////////
// Makros / Defines
#if defined _WIN32 && defined _DEBUG && defined _MSC_VER
#define new new(_NORMAL_BLOCK, THIS_FILE, __LINE__)
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


const unsigned AUTO_SAVE_INTERVALS_COUNT = 6;

const unsigned AUTO_SAVE_INTERVALS[AUTO_SAVE_INTERVALS_COUNT] =
{
    500, 1000, 5000, 10000, 50000, 100000
};

///////////////////////////////////////////////////////////////////////////////
/**
 *  Konstruktor von @p iwSaveLoad.
 *
 *  @author OLiver
 */
iwSaveLoad::iwSaveLoad(const unsigned short add_height, const std::string& window_title)
    : IngameWindow(CGI_SAVE, 0xFFFF, 0xFFFF, 600, 400 + add_height, window_title, LOADER.GetImageN("resource", 41))
{
    AddTable(0, 20, 30, 560, 300, TC_GREEN2, NormalFont, 5, _("Filename"), 270, ctrlTable::SRT_STRING, _("Map"), 250, ctrlTable::SRT_STRING, _("Time"), 250, ctrlTable::SRT_DATE, _("Start GF"), 320, ctrlTable::SRT_NUMBER,  "", 0, ctrlTable::SRT_STRING);


    // Tabelle ausfüllen beim Start
    RefreshTable();
}

///////////////////////////////////////////////////////////////////////////////
/**
 *
 *
 *  @author OLiver
 */
void iwSaveLoad::Msg_EditEnter(const unsigned int ctrl_id)
{
    SaveLoad();
}

///////////////////////////////////////////////////////////////////////////////
/**
 *
 *
 *  @author OLiver
 */
void iwSaveLoad::Msg_ButtonClick(const unsigned int ctrl_id)
{
    SaveLoad();
}

///////////////////////////////////////////////////////////////////////////////
/**
 *
 *
 *  @author OLiver
 */
void iwSaveLoad::Msg_TableSelectItem(const unsigned int ctrl_id, const unsigned short selection)
{
    // Dateiname ins Edit schreiben, wenn wir entsprechende Einträge auswählen
    GetCtrl<ctrlEdit>(1)->SetText
    (GetCtrl<ctrlTable>(0)->GetItemText(selection, 0));
}

///////////////////////////////////////////////////////////////////////////////
/**
 *
 *
 *  @author OLiver
 */
void iwSaveLoad::RefreshTable()
{
    GetCtrl<ctrlTable>(0)->DeleteAllItems();

    // Verzeichnis auflisten
    std::string tmp = GetFilePath(FILE_PATHS[85]);
    tmp += "*.sav";
    ListDir(tmp.c_str(), false, iwSave::FillSaveTable, GetCtrl<ctrlTable>(0));

    // qx: Nach Zeit Sortieren
    GetCtrl<ctrlTable>(0)->SortRows(2);
}

///////////////////////////////////////////////////////////////////////////////
/**
 *
 *
 *  @author OLiver
 */
void iwSaveLoad::FillSaveTable(const std::string& filename, void* param)
{
    char datestring[64];

    Savegame save;

    // Datei öffnen
    if(!save.Load(filename, false, false))
        return;

    // Zeitstring erstellen
    TIME.FormatTime(datestring, "%d.%m.%Y - %H:%i", &save.save_time);

    // Dateiname noch rausextrahieren aus dem Pfad
    size_t pos = filename.find_last_of('/');
    if(pos == std::string::npos)
        return;
    std::string extracted_filename = filename.substr(pos + 1);

    // ".sav" am Ende weg
    assert(extracted_filename.length() >= 4);
    extracted_filename.erase(extracted_filename.length() - 4);

    char start_gf[32];
    sprintf(start_gf, "%u", save.start_gf);

    // Und das Zeug zur Tabelle hinzufügen
    static_cast<ctrlTable*>(param)->AddRow(0, extracted_filename.c_str(), save.map_name.c_str(), datestring, start_gf, filename.c_str());
}

///////////////////////////////////////////////////////////////////////////////
/**
 *
 *
 *  @author OLiver
 */
void iwSave::SaveLoad()
{
    // Speichern
    std::string tmp = GetFilePath(FILE_PATHS[85]);
    tmp += GetCtrl<ctrlEdit>(1)->GetText();
    tmp += ".sav";

    // Speichern
    GAMECLIENT.WriteSaveHeader(tmp);

    // Aktualisieren
    RefreshTable();

    // Edit wieder leeren
    GetCtrl<ctrlEdit>(1)->SetText("");
}

///////////////////////////////////////////////////////////////////////////////
/**
 *
 *
 *  @author OLiver
 */
iwSave::iwSave() : iwSaveLoad(40, _("Save game!"))
{
    AddEdit(1, 20, 390, 510, 22, TC_GREEN2, NormalFont);
    AddImageButton(2, 540, 386, 40, 40, TC_GREEN2, LOADER.GetImageN("io", 47));

    // Autospeicherzeug
    AddText(3, 20, 350, _("Auto-Save every:"), 0xFFFFFF00, 0, NormalFont);
    ctrlComboBox* combo = AddComboBox(4, 270, 345, 130, 22, TC_GREEN2, NormalFont, 100);

    /// Combobox füllen
    combo->AddString(_("Disabled")); // deaktiviert

    // Die Intervalle
    for(unsigned i = 0; i < AUTO_SAVE_INTERVALS_COUNT; ++i)
    {
        char str[64];
        sprintf(str, "%u GF", AUTO_SAVE_INTERVALS[i]);
        combo->AddString(str);
    }

    // Richtigen Eintrag auswählen
    bool found = false;
    for(unsigned i = 0; i < AUTO_SAVE_INTERVALS_COUNT; ++i)
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
}

///////////////////////////////////////////////////////////////////////////////
/**
 *
 *
 *  @author OLiver
 */
void iwSave::Msg_ComboSelectItem(const unsigned int ctrl_id, const unsigned short selection)
{

    // Erster Eintrag --> deaktiviert
    if(selection == 0)
        SETTINGS.interface.autosave_interval = 0;
    else
        // ansonsten jeweilige GF-Zahl eintragen
        SETTINGS.interface.autosave_interval = AUTO_SAVE_INTERVALS[selection - 1];
}

///////////////////////////////////////////////////////////////////////////////
/**
 *
 *
 *  @author OLiver
 */
iwLoad::iwLoad(const CreateServerInfo& csi) : iwSaveLoad(0, _("Load game!")),  csi(csi)
{
    AddEdit(1, 20, 350, 510, 22, TC_GREEN2, NormalFont);
    AddImageButton(2, 540, 346, 40, 40, TC_GREEN2, LOADER.GetImageN("io", 48));
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  Spiel laden.
 *
 *  @author OLiver
 */
void iwLoad::SaveLoad(void)
{
    // Server starten
    ctrlTable* table = GetCtrl<ctrlTable>(0);

    if(!GAMESERVER.TryToStart(csi, table->GetItemText(table->GetSelection(), 4), MAPTYPE_SAVEGAME))
    {
        // Server starten
        if(LOBBYCLIENT.LoggedIn())
            // Lobby zeigen, wenn wenn das nich ging und man im Lobby-Modus ist
            WindowManager::inst().Switch(new dskLobby);
        else
            // Ansonsten schließen
            Close();
    }
    else
    {
        // Verbindungsfenster anzeigen
        WindowManager::inst().Show(new iwPleaseWait);
    }

}


/// Handle double click on the table
void iwLoad::Msg_TableChooseItem(const unsigned ctrl_id, const unsigned short selection)
{
    SaveLoad();
}



