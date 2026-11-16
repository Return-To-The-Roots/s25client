// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "iwHarborBuilding.h"
#include "Loader.h"
#include "buildings/nobHarborBuilding.h"
#include "controls/ctrlGroup.h"
#include "controls/ctrlImageButton.h"
#include "network/GameClient.h"
#include "ogl/FontStyle.h"

iwHarborBuilding::iwHarborBuilding(GameWorldView& gwv, GameCommandFactory& gcFactory, nobHarborBuilding* hb)
    : iwHQ(gwv, gcFactory, hb)
{
    setTitle(_("Harbor building"));

    // Zusätzliche Hafenseite
    ctrlGroup& harbor_page = AddPage();
    grpIdExpedition = harbor_page.getID();

    // "Expedition"-Überschrift
    harbor_page.AddText(0, DrawPoint(83, 70), _("Expedition"), 0xFFFFFF00, FontStyle::CENTER, NormalFont);

    // Button zum Expedition starten
    harbor_page.AddImageButton(1, DrawPoint(65, 100), Extent(30, 30), TextureColor::Grey, LOADER.getImageN("io", 176),
                               _("Start expedition"));
    AdjustExpeditionButton(false);

    // "Expedition"-Überschrift
    harbor_page.AddText(2, DrawPoint(83, 140), _("Exploration expedition"), 0xFFFFFF00, FontStyle::CENTER, NormalFont);

    // Button zum Expedition starten
    harbor_page.AddImageButton(3, DrawPoint(65, 170), Extent(30, 30), TextureColor::Grey, LOADER.getImageN("io", 176),
                               _("Start exporation expedition"));
    AdjustExplorationExpeditionButton(false);
}

/**
 *  setzt den Expeditionsknopf korrekt
 *
 *  falls @p flip gesetzt, dann umgekehrt einfärben
 */
void iwHarborBuilding::AdjustExpeditionButton(bool flip)
{
    auto* button = getCtrl<ctrlGroup>(grpIdExpedition)->getCtrl<ctrlImageButton>(1);

    // Visuelle Rückmeldung, grün einfärben, wenn Expedition gestartet wurde
    // Jeweils umgekehrte Farbe nehmen, da die änderung ja spielerisch noch nicht
    // in Kraft getreten ist!
    bool exp = static_cast<const nobHarborBuilding*>(wh)->isExpeditionActive();

    // "flip xor exp", damit korrekt geswitcht wird, falls expedition abgebrochen werden soll
    // und dies direkt dargestellt werden soll (flip)
    if((flip || exp) && !(flip && exp))
    {
        button->setModulationColor(COLOR_WHITE);
        button->setTooltip(_("Cancel expedition"));
    } else
    {
        button->setModulationColor(COLOR_RED);
        button->setTooltip(_("Start expedition"));
    }
}

/**
 *  setzt den Expeditionsknopf korrekt
 *
 *  falls @p flip gesetzt, dann umgekehrt einfärben
 */
void iwHarborBuilding::AdjustExplorationExpeditionButton(bool flip)
{
    auto* button = getCtrl<ctrlGroup>(grpIdExpedition)->getCtrl<ctrlImageButton>(3);

    // Visuelle Rückmeldung, grün einfärben, wenn Expedition gestartet wurde
    // Jeweils umgekehrte Farbe nehmen, da die änderung ja spielerisch noch nicht
    // in Kraft getreten ist!
    bool exp = static_cast<const nobHarborBuilding*>(wh)->isExplorationExpeditionActive();

    // "flip xor exp", damit korrekt geswitcht wird, falls expedition abgebrochen werden soll
    // und dies direkt dargestellt werden soll (flip)
    if((flip || exp) && !(flip && exp))
    {
        button->setModulationColor(COLOR_WHITE);
        button->setTooltip(_("Cancel expedition"));
    } else
    {
        button->setModulationColor(COLOR_RED);
        button->setTooltip(_("Start expedition"));
    }
}

void iwHarborBuilding::msgGroupButtonClick(const unsigned group_id, const unsigned ctrl_id)
{
    if(group_id == grpIdExpedition) // Hafengruppe?
    {
        switch(ctrl_id)
        {
            case 1: // Expedition starten
                // Entsprechenden GC senden
                if(GAMECLIENT.StartStopExpedition(wh->getPos(),
                                                  !static_cast<const nobHarborBuilding*>(wh)->isExpeditionActive()))
                    AdjustExpeditionButton(true);
                break;
            case 3: // Expedition starten
                // Entsprechenden GC senden
                if(GAMECLIENT.StartStopExplorationExpedition(
                     wh->getPos(), !static_cast<const nobHarborBuilding*>(wh)->isExplorationExpeditionActive()))
                    AdjustExplorationExpeditionButton(true);
                break;
        }
    }

    // an Basis weiterleiten
    iwHQ::msgGroupButtonClick(group_id, ctrl_id);
}
