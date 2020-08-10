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

#include "iwDiplomacy.h"
#include "GamePlayer.h"
#include "GlobalGameSettings.h"
#include "Loader.h"
#include "WindowManager.h"
#include "controls/ctrlComboBox.h"
#include "controls/ctrlImageButton.h"
#include "controls/ctrlText.h"
#include "controls/ctrlTextDeepening.h"
#include "helpers/format.hpp"
#include "helpers/toString.h"
#include "iwMsgbox.h"
#include "network/GameClient.h"
#include "ogl/FontStyle.h"
#include "ogl/glArchivItem_Bitmap.h"
#include "world/GameWorldBase.h"
#include "world/GameWorldViewer.h"
#include "gameData/const_gui_ids.h"

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
const Extent PING_FIELD_SIZE(40, 22);
/// Position der Pingfelder vom linken Rand aus (relativ zur Mitte)
const unsigned short PING_FIELD_POS = 150;
/// Position der Bündnisse vom linken Rand aus (relativ zur Mitte)
const unsigned short TREATIES_POS = 240;
/// Abstand zwischen den beiden Bündnis-Buttons (Achtung: von Mittelpunkten aus!)
const unsigned short TREATIE_BUTTON_SPACE = 20;

iwDiplomacy::iwDiplomacy(const GameWorldViewer& gwv, GameCommandFactory& gcFactory)
    : IngameWindow(CGI_DIPLOMACY, IngameWindow::posLastOrCenter,
                   Extent(500, FIRST_LINE_Y + gwv.GetWorld().GetNumPlayers() * (CELL_HEIGHT + SPACE_HEIGHT) + 20), _("Diplomacy"),
                   LOADER.GetImageN("resource", 41)),
      gwv(gwv), gcFactory(gcFactory)
{
    // "Header" der Tabelle
    AddText(0, DrawPoint(LINE_DISTANCE_TO_MARGINS + PING_FIELD_POS, HEADER_Y), _("Ping"), COLOR_YELLOW, FontStyle::CENTER, NormalFont);
    AddText(1, DrawPoint(LINE_DISTANCE_TO_MARGINS + TREATIES_POS, HEADER_Y), _("Treaties"), COLOR_YELLOW, FontStyle::CENTER, NormalFont);

    DrawPoint curTxtPos(LINE_DISTANCE_TO_MARGINS + 10, FIRST_LINE_Y + CELL_HEIGHT / 2 - CELL_HEIGHT - SPACE_HEIGHT);
    for(unsigned i = 0; i < gwv.GetWorld().GetNumPlayers(); ++i)
    {
        const GamePlayer& player = gwv.GetWorld().GetPlayer(i);
        curTxtPos.y += CELL_HEIGHT + SPACE_HEIGHT;
        if(!player.isUsed())
            continue;
        // Einzelne Spielernamen
        AddText(100 + i, curTxtPos, player.name, player.color, FontStyle::VCENTER, NormalFont);

        if(player.ps == PS_OCCUPIED)
        {
            // Ping
            DrawPoint pingPos(LINE_DISTANCE_TO_MARGINS + PING_FIELD_POS - PING_FIELD_SIZE.x / 2, curTxtPos.y);
            AddTextDeepening(200 + i, pingPos, PING_FIELD_SIZE, TC_GREY, "0", NormalFont, COLOR_YELLOW);
        }

        // An sich selber braucht man keine Bündnisse zu schließen
        if(gwv.GetPlayerId() == i)
            continue;
        // Bündnisvertrag-Button
        glArchivItem_Bitmap* image = LOADER.GetImageN("io", 61);
        Extent btSize(40, 40);
        DrawPoint btPos(LINE_DISTANCE_TO_MARGINS + TREATIES_POS - TREATIE_BUTTON_SPACE / 2 - (image->getWidth() + 8),
                        curTxtPos.y - btSize.y / 2);
        ctrlButton* button = AddImageButton(300 + i, btPos, btSize, TC_GREY, image, _("Treaty of alliance"));

        // Verbleibende Zeit unter dem Button
        DrawPoint remainingTimePos = button->GetPos() + DrawPoint(btSize.x / 2, btSize.y + 4);
        AddText(500 + i, remainingTimePos, "", COLOR_YELLOW, FontStyle::CENTER, SmallFont);

        // Nichtangriffspakt
        image = LOADER.GetImageN("io", 100);
        btPos.x = LINE_DISTANCE_TO_MARGINS + TREATIES_POS + TREATIE_BUTTON_SPACE / 2;
        button = AddImageButton(400 + i, btPos, btSize, TC_GREY, image, _("Non-aggression pact"));

        // Verbleibende Zeit unter dem Button
        remainingTimePos = button->GetPos() + DrawPoint(btSize.x / 2, btSize.y + 4);
        AddText(600 + i, remainingTimePos, "", COLOR_YELLOW, FontStyle::CENTER, SmallFont);
    }
    // Farben festlegen
    Msg_PaintAfter();
}

void iwDiplomacy::Msg_PaintBefore()
{
    // Farben, die zu den 3 Bündnisstates gesetzt werden (0-kein Bündnis, 1-in Arbeit, 2-Bündnis abgeschlossen)
    const std::array<unsigned, 3> PACT_COLORS = {COLOR_RED, COLOR_YELLOW, COLOR_GREEN};

    IngameWindow::Msg_PaintBefore();
    // Die farbigen Zeilen malen
    DrawPoint curPos = GetDrawPos() + DrawPoint(LINE_DISTANCE_TO_MARGINS, FIRST_LINE_Y);
    Rect curRect(curPos, Extent(GetSize().x - 2 * LINE_DISTANCE_TO_MARGINS, CELL_HEIGHT));
    for(unsigned i = 0; i < gwv.GetWorld().GetNumPlayers(); ++i)
    {
        // Rechtecke in Spielerfarbe malen mit entsprechender Transparenz
        DrawRectangle(curRect, SetAlpha(gwv.GetWorld().GetPlayer(i).color, 0x40));
        curRect.move(DrawPoint(0, CELL_HEIGHT + SPACE_HEIGHT));

        // Farben der Bündnis-Buttons setzen, je nachdem wie der Status ist

        // Existiert der Button auch?
        auto* button = GetCtrl<ctrlImageButton>(300 + i);
        // Bündnisvertrag
        if(button)
            // Farbe je nach Bündnisstatus setzen
            button->SetModulationColor(PACT_COLORS[gwv.GetPlayer().GetPactState(TREATY_OF_ALLIANCE, i)]);
        // Nicht-Angriffspakt
        button = GetCtrl<ctrlImageButton>(400 + i);
        if(button)
            // Farbe je nach Bündnisstatus setzen
            button->SetModulationColor(PACT_COLORS[gwv.GetPlayer().GetPactState(NON_AGGRESSION_PACT, i)]);

        // Ggf. Ping aktualisieren
        if(auto* pingfield = GetCtrl<ctrlTextDeepening>(200 + i))
            pingfield->SetText(helpers::toString(gwv.GetWorld().GetPlayer(i).ping));

        // Verbleibende Zeit der Bündnisse in den Text-Ctrls anzeigen
        if(GetCtrl<ctrlText>(500 + i))
        {
            for(unsigned z = 0; z < 2; ++z)
            {
                unsigned duration = gwv.GetPlayer().GetRemainingPactTime(PactType(z), i);
                // Überhaupt ein Bündnis abgeschlossen und Bündnis nicht für die Ewigkeit?
                if(duration > 0 && duration != DURATION_INFINITE)
                    // Dann entsprechende Zeit setzen
                    GetCtrl<ctrlText>(500 + z * 100 + i)->SetText(GAMECLIENT.FormatGFTime(duration));
                else
                    // Ansonsten leer
                    GetCtrl<ctrlText>(500 + z * 100 + i)->SetText("");
            }
        }
    }
}

void iwDiplomacy::Msg_ButtonClick(const unsigned ctrl_id)
{
    if(gwv.GetWorld().GetGGS().lockedTeams)
    {
        WINDOWMANAGER.Show(std::make_unique<iwMsgbox>(_("Teams locked"),
                                                      _("As the teams are locked, you cannot make treaties of any kind."), nullptr, MSB_OK,
                                                      MSB_EXCLAMATIONGREEN, 1));
        return;
    }

    // Bündnisverträge
    if(ctrl_id >= 300 && ctrl_id < 400)
    {
        auto playerId = static_cast<unsigned char>(ctrl_id - 300);
        // Noch kein Bündnis abgeschlossen?
        if(gwv.GetPlayer().GetPactState(TREATY_OF_ALLIANCE, playerId) == GamePlayer::NO_PACT)
            // Dann neues Bündnis vorschlagen
            WINDOWMANAGER.ReplaceWindow(std::make_unique<iwSuggestPact>(TREATY_OF_ALLIANCE, gwv.GetWorld().GetPlayer(playerId), gcFactory));
        else
            // ansonsten Vertrag versuchen abzubrechen
            gcFactory.CancelPact(TREATY_OF_ALLIANCE, playerId);
    }
    // Nichtangriffspakte
    if(ctrl_id >= 400 && ctrl_id < 500)
    {
        auto playerId = static_cast<unsigned char>(ctrl_id - 400);
        // Noch kein Bündnis abgeschlossen?
        if(gwv.GetPlayer().GetPactState(NON_AGGRESSION_PACT, playerId) == GamePlayer::NO_PACT)
            // Dann neues Bündnis vorschlagen
            WINDOWMANAGER.ReplaceWindow(
              std::make_unique<iwSuggestPact>(NON_AGGRESSION_PACT, gwv.GetWorld().GetPlayer(playerId), gcFactory));
        else
            // ansonsten Vertrag versuchen abzubrechen
            gcFactory.CancelPact(NON_AGGRESSION_PACT, playerId);
    }
}

/////////////////////////////
/////////////////////////////

/// Titel für die Fenster für unterschiedliche Bündnistypen
const std::array<const char*, NUM_PACTS> PACT_TITLES = {gettext_noop("Suggest treaty of alliance"),
                                                        gettext_noop("Suggest non-aggression pact")};

/// Anzahl der unterschiedlich möglichen Längen ("für immer" nicht mit eingerechnet!)
const unsigned NUM_DURATIONS = 3;

/// Längen für die Dauer des Vertrages (kurz-, mittel- und langfristig)
const std::array<unsigned, NUM_DURATIONS> DURATIONS = {5000, 30000, 100000};

/// Namen für diese Vertragsdauern
const std::array<const char*, NUM_DURATIONS> DURATION_NAMES = {gettext_noop("Short-run"), gettext_noop("Medium-term"),
                                                               gettext_noop("Long-run")};

iwSuggestPact::iwSuggestPact(const PactType pt, const GamePlayer& player, GameCommandFactory& gcFactory)
    : IngameWindow(CGI_SUGGESTPACT, IngameWindow::posLastOrCenter, Extent(320, 215), _(PACT_TITLES[pt]), LOADER.GetImageN("resource", 41)),
      pt(pt), player(player), gcFactory(gcFactory)
{
    glArchivItem_Bitmap* image;

    switch(pt)
    {
        case TREATY_OF_ALLIANCE: image = LOADER.GetImageN("io", 61); break;
        case NON_AGGRESSION_PACT: image = LOADER.GetImageN("io", 100); break;
        default: image = nullptr;
    }

    // Bild als Orientierung, welchen Vertrag wir gerade bearbeiten
    if(image)
        this->AddImage(0, DrawPoint(55, 100), image);

    AddText(1, DrawPoint(100, 30), _("Contract type:"), COLOR_YELLOW, FontStyle{}, NormalFont);
    AddText(2, DrawPoint(100, 45), _(PACT_NAMES[pt]), COLOR_GREEN, FontStyle{}, NormalFont);
    AddText(3, DrawPoint(100, 70), _("To player:"), COLOR_YELLOW, FontStyle{}, NormalFont);
    AddText(4, DrawPoint(100, 85), player.name, player.color, FontStyle{}, NormalFont);
    AddText(5, DrawPoint(100, 110), _("Duration:"), COLOR_YELLOW, FontStyle{}, NormalFont);
    ctrlComboBox* combo = AddComboBox(6, DrawPoint(100, 125), Extent(190, 22), TC_GREEN2, NormalFont, 100);

    // Zeiten zur Combobox hinzufügen
    for(unsigned i = 0; i < NUM_DURATIONS; ++i)
    {
        combo->AddString(helpers::format("%s  (%s)", DURATION_NAMES[i], GAMECLIENT.FormatGFTime(DURATIONS[i])));
    }
    // Erstes Item in der Combobox vorerst auswählen
    combo->SetSelection(0);

    // Option "ewig" noch hinzufügen
    combo->AddString(_("Eternal"));

    AddTextButton(7, DrawPoint(110, 170), Extent(100, 22), TC_GREEN2, _("Confirm"), NormalFont);
}

void iwSuggestPact::Msg_ButtonClick(const unsigned /*ctrl_id*/)
{
    /// Dauer auswählen (wenn id == NUM_DURATIONS, dann "für alle Ewigkeit" ausgewählt)
    unsigned selected_id = GetCtrl<ctrlComboBox>(6)->GetSelection().get();
    unsigned duration = (selected_id == NUM_DURATIONS) ? DURATION_INFINITE : DURATIONS[selected_id];
    gcFactory.SuggestPact(player.GetPlayerId(), this->pt, duration);
    Close();
}
