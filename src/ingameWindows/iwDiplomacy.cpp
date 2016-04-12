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
#include "iwDiplomacy.h"

#include "Loader.h"
#include "GameClient.h"
#include "WindowManager.h"
#include "GameClientPlayer.h"
#include "iwMsgbox.h"
#include "controls/ctrlButton.h"
#include "controls/ctrlComboBox.h"
#include "controls/ctrlDeepening.h"
#include "controls/ctrlText.h"
#include "ogl/glArchivItem_Font.h"
#include "gameData/const_gui_ids.h"

// Include last!
#include "DebugNew.h" // IWYU pragma: keep

/// Position des Headers der Tabelle (Y)
const unsigned short HEADER_Y = 30;
/// Position der ersten Zeile (Y)
const unsigned short FIRST_LINE_Y = 55;
/// Höhe der einzelnen farbigen Zeilen
const unsigned short CELL_HEIGHT = 60;
/// Abstand zwischen den farbigen Zeilen
const unsigned short SPACE_HEIGHT = 10;
/// Abstand vom Rand der Zeilen
const unsigned short LINE_DISTANCE_TO_MARGINS = 20;
/// Größe der Pingfelder
const unsigned short PING_FIELD_WIDTH = 40;
/// Position der Pingfelder vom linken Rand aus (relativ zur Mitte)
const unsigned short PING_FIELD_POS = 150;
/// Position der Bündnisse vom linken Rand aus (relativ zur Mitte)
const unsigned short TREATIES_POS = 240;
/// Abstand zwischen den beiden Bündnis-Buttons (Achtung: von Mittelpunkten aus!)
const unsigned short TREATIE_BUTTON_SPACE = 20;

iwDiplomacy::iwDiplomacy()
    : IngameWindow(CGI_DIPLOMACY, (unsigned short) - 1, (unsigned short) - 1, 500, FIRST_LINE_Y + GAMECLIENT.GetPlayerCount() * (CELL_HEIGHT + SPACE_HEIGHT) + 20, _("Diplomacy"),
                   LOADER.GetImageN("resource", 41))
{
    // "Header" der Tabelle
    AddText(0, LINE_DISTANCE_TO_MARGINS + PING_FIELD_POS, HEADER_Y, _("Ping"), COLOR_YELLOW, glArchivItem_Font::DF_CENTER, NormalFont);
    AddText(1, LINE_DISTANCE_TO_MARGINS + TREATIES_POS, HEADER_Y, _("Treaties"), COLOR_YELLOW, glArchivItem_Font::DF_CENTER, NormalFont);



    for(unsigned i = 0; i < GAMECLIENT.GetPlayerCount(); ++i)
    {
        GameClientPlayer& player = GAMECLIENT.GetPlayer(i);
        if(player.isUsed())
        {
            // Einzelne Spielernamen
            AddText(100 + i, LINE_DISTANCE_TO_MARGINS + 10, FIRST_LINE_Y + i * (CELL_HEIGHT + SPACE_HEIGHT) + CELL_HEIGHT / 2,
                    player.name, player.color, glArchivItem_Font::DF_VCENTER,
                    NormalFont);

            if(player.ps == PS_OCCUPIED)
                // Ping
                AddDeepening(200 + i, LINE_DISTANCE_TO_MARGINS + PING_FIELD_POS - PING_FIELD_WIDTH / 2,
                             FIRST_LINE_Y + i * (CELL_HEIGHT + SPACE_HEIGHT) + CELL_HEIGHT / 2 - 11, PING_FIELD_WIDTH, 22, TC_GREY, "0", NormalFont, COLOR_YELLOW);

            // An sich selber braucht man keine Bündnisse zu schließen
            if(GAMECLIENT.GetPlayerID() != i)
            {
                // Bündnisvertrag-Button
                glArchivItem_Bitmap* image = LOADER.GetImageN("io", 61);
                ctrlButton* button = AddImageButton(300 + i, LINE_DISTANCE_TO_MARGINS + TREATIES_POS - TREATIE_BUTTON_SPACE / 2 - (image->getWidth() + 8),
                                                    FIRST_LINE_Y + i * (CELL_HEIGHT + SPACE_HEIGHT) + CELL_HEIGHT / 2 - 40 / 2, 40,
                                                    40, TC_GREY, image, _("Treaty of alliance"));

                // Verbleibende Zeit unter dem Button
                AddText(500 + i, button->GetX(false) + button->GetWidth() / 2, button->GetY(false) + button->GetHeight() + 4, "", COLOR_YELLOW, glArchivItem_Font::DF_CENTER, SmallFont);

                // Nichtangriffspakt
                image = LOADER.GetImageN("io", 100);
                button = AddImageButton(400 + i, LINE_DISTANCE_TO_MARGINS + TREATIES_POS + TREATIE_BUTTON_SPACE / 2,
                                        FIRST_LINE_Y + i * (CELL_HEIGHT + SPACE_HEIGHT) + CELL_HEIGHT / 2 - 40 / 2, 40,
                                        40, TC_GREY, image, _("Non-aggression pact"));

                // Verbleibende Zeit unter dem Button
                AddText(600 + i, button->GetX(false) + button->GetWidth() / 2, button->GetY(false) + button->GetHeight() + 4, "", COLOR_YELLOW, glArchivItem_Font::DF_CENTER, SmallFont);

            }
        }
    }

    // Farben festlegen
    Msg_PaintAfter();
}

void iwDiplomacy::Msg_PaintBefore()
{
    // Die farbigen Zeilen malen
    for(unsigned i = 0; i < GAMECLIENT.GetPlayerCount(); ++i)
    {
        // Rechtecke in Spielerfarbe malen mit entsprechender Transparenz
        Window::DrawRectangle(GetX() + LINE_DISTANCE_TO_MARGINS, GetY() + FIRST_LINE_Y + i * (CELL_HEIGHT + SPACE_HEIGHT), width_ - 2 * LINE_DISTANCE_TO_MARGINS, CELL_HEIGHT,
                              (GAMECLIENT.GetPlayer(i).color & 0x00FFFFFF) | 0x40000000);
    }
}

void iwDiplomacy::Msg_PaintAfter()
{
    // Farben, die zu den 3 Bündnisstates gesetzt werden (0-kein Bündnis, 1-in Arbeit, 2-Bündnis abgeschlossen)
    const unsigned PACT_COLORS[3] =
    {
        COLOR_RED, COLOR_YELLOW, COLOR_GREEN
    };


    for(unsigned i = 0; i < GAMECLIENT.GetPlayerCount(); ++i)
    {
        // Farben der Bündnis-Buttons setzen, je nachdem wie der Status ist

        // Existiert der Button auch?
        ctrlImageButton* button = GetCtrl<ctrlImageButton>(300 + i);
        // Bündnisvertrag
        if(button)
            // Farbe je nach Bündnisstatus setzen
            button->SetModulationColor(PACT_COLORS[GAMECLIENT.GetLocalPlayer().GetPactState(TREATY_OF_ALLIANCE, i)]);
        // Nicht-Angriffspakt
        button = GetCtrl<ctrlImageButton>(400 + i);
        if(button)
            // Farbe je nach Bündnisstatus setzen
            button->SetModulationColor(PACT_COLORS[GAMECLIENT.GetLocalPlayer().GetPactState(NON_AGGRESSION_PACT, i)]);

        // Ggf. Ping aktualisieren
        if(ctrlDeepening* pingfield = GetCtrl<ctrlDeepening>(200 + i))
        {
            char ping[64];
            sprintf(ping, "%u", GAMECLIENT.GetPlayer(i).ping);
            pingfield->SetText(ping);
        }

        // Verbleibende Zeit der Bündnisse in den Text-Ctrls anzeigen
        if(GetCtrl<ctrlText>(500 + i))
        {
            for(unsigned z = 0; z < 2; ++z)
            {
                unsigned duration = GAMECLIENT.GetLocalPlayer().GetRemainingPactTime(PactType(z), i);
                // Überhaupt ein Bündnis abgeschlossen und Bündnis nicht für die Ewigkeit?
                if(duration > 0 && duration != 0xFFFFFFFF)
                    // Dann entsprechende Zeit setzen
                    GetCtrl<ctrlText>(500 + z * 100 + i)->SetText(GAMECLIENT.FormatGFTime(duration));
                else
                    // Ansonsten leer
                    GetCtrl<ctrlText>(500 + z * 100 + i)->SetText("");
            }
        }
    }
}

void iwDiplomacy::Msg_ButtonClick(const unsigned int ctrl_id)
{
    if (GAMECLIENT.GetGGS().lock_teams)
    {
        WINDOWMANAGER.Show(new iwMsgbox(_("Teams locked"), _("As the teams are locked, you cannot make treaties of any kind."), NULL, MSB_OK, MSB_EXCLAMATIONGREEN, 1));
        return;
    }

    // Bündnisverträge
    if(ctrl_id >= 300 && ctrl_id < 400)
    {
        unsigned char player_id = static_cast<unsigned char>(ctrl_id - 300);
        // Noch kein Bündnis abgeschlossen?
        if(GAMECLIENT.GetLocalPlayer().GetPactState(TREATY_OF_ALLIANCE, player_id) == GameClientPlayer::NO_PACT)
            // Dann neues Bündnis vorschlagen
            WINDOWMANAGER.Show(new iwSuggestPact(TREATY_OF_ALLIANCE, player_id));
        else
            // ansonsten Vertrag versuchen abzubrechen
            GAMECLIENT.CancelPact(TREATY_OF_ALLIANCE, player_id);
    }
    // Nichtangriffspakte
    if(ctrl_id >= 400 && ctrl_id < 500)
    {
        unsigned char player_id = static_cast<unsigned char>(ctrl_id - 400);
        // Noch kein Bündnis abgeschlossen?
        if(GAMECLIENT.GetLocalPlayer().GetPactState(NON_AGGRESSION_PACT, player_id) == GameClientPlayer::NO_PACT)
            // Dann neues Bündnis vorschlagen
            WINDOWMANAGER.Show(new iwSuggestPact(NON_AGGRESSION_PACT, player_id));
        else
            // ansonsten Vertrag versuchen abzubrechen
            GAMECLIENT.CancelPact(NON_AGGRESSION_PACT, player_id);
    }

}

/////////////////////////////
/////////////////////////////

/// Titel für die Fenster für unterschiedliche Bündnistypen
const char* const PACT_TITLES[PACTS_COUNT] =
{
    gettext_noop("Suggest treaty of alliance"),
    gettext_noop("Suggest non-aggression pact")
};


/// Anzahl der unterschiedlich möglichen Längen ("für immer" nicht mit eingerechnet!)
const unsigned DURATION_COUNT = 3;

/// Längen für die Dauer des Vertrages (kurz-, mittel- und langfristig)
const unsigned DURATIONS[DURATION_COUNT] =
{
    5000,
    30000,
    100000
};

/// Namen für diese Vertragsdauern
const char* const DURATION_NAMES[DURATION_COUNT] =
{
    gettext_noop("Short-run"),
    gettext_noop("Medium-term"),
    gettext_noop("Long-run")
};


iwSuggestPact::iwSuggestPact(const PactType pt, const unsigned char player) : IngameWindow(CGI_SUGGESTPACT, (unsigned short) - 1,
            (unsigned short) - 1, 320, 215, _(PACT_TITLES[pt]), LOADER.GetImageN("resource", 41)), pt(pt), player(player)
{
    glArchivItem_Bitmap* image;

    switch(pt)
    {
        case TREATY_OF_ALLIANCE: image = LOADER.GetImageN("io", 61); break;
        case NON_AGGRESSION_PACT: image = LOADER.GetImageN("io", 100); break;
        default: image = NULL;
    }

    // Bild als Orientierung, welchen Vertrag wir gerade bearbeiten
    if(image)
        this->AddImage(0, 55, 100, image);

    AddText(1, 100, 30, _("Contract type:"), COLOR_YELLOW, 0, NormalFont);
    AddText(2, 100, 45, _(PACT_NAMES[pt]), COLOR_GREEN, 0, NormalFont);
    AddText(3, 100, 70, _("To player:"), COLOR_YELLOW, 0, NormalFont);
    AddText(4, 100, 85, GAMECLIENT.GetPlayer(player).name, GAMECLIENT.GetPlayer(player).color, 0, NormalFont);
    AddText(5, 100, 110, _("Duration:"), COLOR_YELLOW, 0, NormalFont);
    ctrlComboBox* combo = AddComboBox(6, 100, 125, 190, 22, TC_GREEN2, NormalFont, 100);

    // Zeiten zur Combobox hinzufügen
    for(unsigned i = 0; i < DURATION_COUNT; ++i)
    {
        char str[256];
        sprintf(str, "%s  (%s)", DURATION_NAMES[i], GAMECLIENT.FormatGFTime(DURATIONS[i]).c_str());
        combo->AddString(str);
    }
    // Erstes Item in der Combobox vorerst auswählen
    combo->SetSelection(0);

    // Option "ewig" noch hinzufügen
    combo->AddString(_("Eternal"));

    AddTextButton(7, 110, 170, 100, 22, TC_GREEN2, _("Confirm"), NormalFont);
}


void iwSuggestPact::Msg_ButtonClick(const unsigned int  /*ctrl_id*/)
{
    /// Dauer auswählen (wenn id == DURATION_COUNT, dann "für alle Ewigkeit" ausgewählt)
    unsigned selected_id = GetCtrl<ctrlComboBox>(6)->GetSelection();
    unsigned duration = (selected_id == DURATION_COUNT) ? 0xFFFFFFFF : DURATIONS[selected_id];
    GAMECLIENT.SuggestPact(player, this->pt, duration);
    Close();
}
