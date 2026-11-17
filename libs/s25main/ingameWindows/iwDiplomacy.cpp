// Copyright (C) 2005 - 2026 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

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
                   Extent(500, FIRST_LINE_Y + gwv.getWorld().getNumPlayers() * (CELL_HEIGHT + SPACE_HEIGHT) + 20),
                   _("Diplomacy"), LOADER.getImageN("resource", 41)),
      gwv(gwv), gcFactory(gcFactory)
{
    // "Header" der Tabelle
    addText(0, DrawPoint(LINE_DISTANCE_TO_MARGINS + PING_FIELD_POS, HEADER_Y), _("Ping"), COLOR_YELLOW,
            FontStyle::CENTER, NormalFont);
    addText(1, DrawPoint(LINE_DISTANCE_TO_MARGINS + TREATIES_POS, HEADER_Y), _("Treaties"), COLOR_YELLOW,
            FontStyle::CENTER, NormalFont);

    DrawPoint curTxtPos(LINE_DISTANCE_TO_MARGINS + 10, FIRST_LINE_Y + CELL_HEIGHT / 2 - CELL_HEIGHT - SPACE_HEIGHT);
    for(unsigned i = 0; i < gwv.getWorld().getNumPlayers(); ++i)
    {
        const GamePlayer& player = gwv.getWorld().getPlayer(i);
        curTxtPos.y += CELL_HEIGHT + SPACE_HEIGHT;
        if(!player.isUsed())
            continue;
        // Einzelne Spielernamen
        addText(100 + i, curTxtPos, player.name, player.color, FontStyle::VCENTER, NormalFont);

        if(player.ps == PlayerState::Occupied)
        {
            // Ping
            DrawPoint pingPos(LINE_DISTANCE_TO_MARGINS + PING_FIELD_POS - PING_FIELD_SIZE.x / 2, curTxtPos.y);
            addTextDeepening(200 + i, pingPos, PING_FIELD_SIZE, TextureColor::Grey, "0", NormalFont, COLOR_YELLOW);
        }

        // An sich selber braucht man keine Bündnisse zu schließen
        if(gwv.getPlayerId() == i)
            continue;
        // Bündnisvertrag-Button
        glArchivItem_Bitmap* image = LOADER.getImageN("io", 61);
        Extent btSize(40, 40);
        DrawPoint btPos(LINE_DISTANCE_TO_MARGINS + TREATIES_POS - TREATIE_BUTTON_SPACE / 2 - (image->getWidth() + 8),
                        curTxtPos.y - btSize.y / 2);
        ctrlButton* button = addImageButton(300 + i, btPos, btSize, TextureColor::Grey, image, _("Treaty of alliance"));

        // Verbleibende Zeit unter dem Button
        DrawPoint remainingTimePos = button->getPos() + DrawPoint(btSize.x / 2, btSize.y + 4);
        addText(500 + i, remainingTimePos, "", COLOR_YELLOW, FontStyle::CENTER, SmallFont);

        // Nichtangriffspakt
        image = LOADER.getImageN("io", 100);
        btPos.x = LINE_DISTANCE_TO_MARGINS + TREATIES_POS + TREATIE_BUTTON_SPACE / 2;
        button = addImageButton(400 + i, btPos, btSize, TextureColor::Grey, image, _("Non-aggression pact"));

        // Verbleibende Zeit unter dem Button
        remainingTimePos = button->getPos() + DrawPoint(btSize.x / 2, btSize.y + 4);
        addText(600 + i, remainingTimePos, "", COLOR_YELLOW, FontStyle::CENTER, SmallFont);
    }
    // Farben festlegen
    msgPaintAfter();
}

void iwDiplomacy::msgPaintBefore()
{
    // Farben, die zu den 3 Bündnisstates gesetzt werden (0-kein Bündnis, 1-in Arbeit, 2-Bündnis abgeschlossen)
    constexpr helpers::EnumArray<unsigned, PactState> PACT_COLORS = {COLOR_RED, COLOR_YELLOW, COLOR_GREEN};

    IngameWindow::msgPaintBefore();
    // Die farbigen Zeilen malen
    DrawPoint curPos = getDrawPos() + DrawPoint(LINE_DISTANCE_TO_MARGINS, FIRST_LINE_Y);
    Rect curRect(curPos, Extent(getSize().x - 2 * LINE_DISTANCE_TO_MARGINS, CELL_HEIGHT));
    for(unsigned i = 0; i < gwv.getWorld().getNumPlayers(); ++i)
    {
        // Rechtecke in Spielerfarbe malen mit entsprechender Transparenz
        drawRectangle(curRect, setAlpha(gwv.getWorld().getPlayer(i).color, 0x40));
        curRect.move(DrawPoint(0, CELL_HEIGHT + SPACE_HEIGHT));

        // Farben der Bündnis-Buttons setzen, je nachdem wie der Status ist

        // Existiert der Button auch?
        auto* button = getCtrl<ctrlImageButton>(300 + i);
        // Bündnisvertrag
        if(button)
            // Farbe je nach Bündnisstatus setzen
            button->setModulationColor(PACT_COLORS[gwv.getPlayer().getPactState(PactType::TreatyOfAlliance, i)]);
        // Nicht-Angriffspakt
        button = getCtrl<ctrlImageButton>(400 + i);
        if(button)
            // Farbe je nach Bündnisstatus setzen
            button->setModulationColor(PACT_COLORS[gwv.getPlayer().getPactState(PactType::NonAgressionPact, i)]);

        // Ggf. Ping aktualisieren
        if(auto* pingfield = getCtrl<ctrlTextDeepening>(200 + i))
            pingfield->setText(helpers::toString(gwv.getWorld().getPlayer(i).ping));

        // Verbleibende Zeit der Bündnisse in den Text-Ctrls anzeigen
        if(getCtrl<ctrlText>(500 + i))
        {
            for(unsigned z = 0; z < 2; ++z)
            {
                unsigned duration = gwv.getPlayer().getRemainingPactTime(PactType(z), i);
                // Überhaupt ein Bündnis abgeschlossen und Bündnis nicht für die Ewigkeit?
                if(duration > 0 && duration != DURATION_INFINITE)
                    // Dann entsprechende Zeit setzen
                    getCtrl<ctrlText>(500 + z * 100 + i)->setText(GAMECLIENT.formatGFTime(duration));
                else
                    // Ansonsten leer
                    getCtrl<ctrlText>(500 + z * 100 + i)->setText("");
            }
        }
    }
}

void iwDiplomacy::msgButtonClick(const unsigned ctrl_id)
{
    if(gwv.getWorld().getGGS().lockedTeams)
    {
        WINDOWMANAGER.show(std::make_unique<iwMsgbox>(
          _("Teams locked"), _("As the teams are locked, you cannot make treaties of any kind."), nullptr,
          MsgboxButton::Ok, MsgboxIcon::ExclamationGreen, 1));
        return;
    }

    // Bündnisverträge
    if(ctrl_id >= 300 && ctrl_id < 400)
    {
        auto playerId = static_cast<unsigned char>(ctrl_id - 300);
        // Noch kein Bündnis abgeschlossen?
        if(gwv.getPlayer().getPactState(PactType::TreatyOfAlliance, playerId) == PactState::None)
            // Dann neues Bündnis vorschlagen
            WINDOWMANAGER.replaceWindow(std::make_unique<iwSuggestPact>(PactType::TreatyOfAlliance,
                                                                        gwv.getWorld().getPlayer(playerId), gcFactory));
        else
            // ansonsten Vertrag versuchen abzubrechen
            gcFactory.CancelPact(PactType::TreatyOfAlliance, playerId);
    }
    // Nichtangriffspakte
    if(ctrl_id >= 400 && ctrl_id < 500)
    {
        auto playerId = static_cast<unsigned char>(ctrl_id - 400);
        // Noch kein Bündnis abgeschlossen?
        if(gwv.getPlayer().getPactState(PactType::NonAgressionPact, playerId) == PactState::None)
            // Dann neues Bündnis vorschlagen
            WINDOWMANAGER.replaceWindow(std::make_unique<iwSuggestPact>(PactType::NonAgressionPact,
                                                                        gwv.getWorld().getPlayer(playerId), gcFactory));
        else
            // ansonsten Vertrag versuchen abzubrechen
            gcFactory.CancelPact(PactType::NonAgressionPact, playerId);
    }
}

/////////////////////////////
/////////////////////////////

/// Titel für die Fenster für unterschiedliche Bündnistypen
const helpers::EnumArray<const char*, PactType> PACT_TITLES = {
  {gettext_noop("Suggest treaty of alliance"), gettext_noop("Suggest non-aggression pact")}};

/// Anzahl der unterschiedlich möglichen Längen ("für immer" nicht mit eingerechnet!)
const unsigned NUM_DURATIONS = 3;

/// Längen für die Dauer des Vertrages (kurz-, mittel- und langfristig)
const std::array<unsigned, NUM_DURATIONS> DURATIONS = {5000, 30000, 100000};

/// Namen für diese Vertragsdauern
const std::array<const char*, NUM_DURATIONS> DURATION_NAMES = {gettext_noop("Short-run"), gettext_noop("Medium-term"),
                                                               gettext_noop("Long-run")};

iwSuggestPact::iwSuggestPact(const PactType pt, const GamePlayer& player, GameCommandFactory& gcFactory)
    : IngameWindow(CGI_SUGGESTPACT, IngameWindow::posLastOrCenter, Extent(320, 215), _(PACT_TITLES[pt]),
                   LOADER.getImageN("resource", 41)),
      pt(pt), player(player), gcFactory(gcFactory)
{
    glArchivItem_Bitmap* image = nullptr;

    switch(pt)
    {
        case PactType::TreatyOfAlliance: image = LOADER.getImageN("io", 61); break;
        case PactType::NonAgressionPact: image = LOADER.getImageN("io", 100); break;
    }

    // Bild als Orientierung, welchen Vertrag wir gerade bearbeiten
    if(image)
        this->addImage(0, DrawPoint(55, 100), image);

    addText(1, DrawPoint(100, 30), _("Contract type:"), COLOR_YELLOW, FontStyle{}, NormalFont);
    addText(2, DrawPoint(100, 45), _(PACT_NAMES[pt]), COLOR_GREEN, FontStyle{}, NormalFont);
    addText(3, DrawPoint(100, 70), _("To player:"), COLOR_YELLOW, FontStyle{}, NormalFont);
    addText(4, DrawPoint(100, 85), player.name, player.color, FontStyle{}, NormalFont);
    addText(5, DrawPoint(100, 110), _("Duration:"), COLOR_YELLOW, FontStyle{}, NormalFont);
    ctrlComboBox* combo = addComboBox(6, DrawPoint(100, 125), Extent(190, 22), TextureColor::Green2, NormalFont, 100);

    // Zeiten zur Combobox hinzufügen
    for(unsigned i = 0; i < NUM_DURATIONS; ++i)
    {
        combo->addItem(helpers::format("%s  (%s)", DURATION_NAMES[i], GAMECLIENT.formatGFTime(DURATIONS[i])));
    }
    // Erstes Item in der Combobox vorerst auswählen
    combo->setSelection(0);

    // Option "ewig" noch hinzufügen
    combo->addItem(_("Eternal"));

    addTextButton(7, DrawPoint(110, 170), Extent(100, 22), TextureColor::Green2, _("Confirm"), NormalFont);
}

void iwSuggestPact::msgButtonClick(const unsigned /*ctrl_id*/)
{
    /// Dauer auswählen (wenn id == NUM_DURATIONS, dann "für alle Ewigkeit" ausgewählt)
    const unsigned selected_id = getCtrl<ctrlComboBox>(6)->getSelection().value();
    const unsigned duration = (selected_id == NUM_DURATIONS) ? DURATION_INFINITE : DURATIONS[selected_id];
    if(gcFactory.SuggestPact(player.getPlayerId(), this->pt, duration))
        close();
}
