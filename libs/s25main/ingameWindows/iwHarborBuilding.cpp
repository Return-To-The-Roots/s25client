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

#include "iwHarborBuilding.h"
#include "Loader.h"
#include "buildings/nobHarborBuilding.h"
#include "controls/ctrlGroup.h"
#include "controls/ctrlImageButton.h"
#include "network/GameClient.h"
#include "ogl/FontStyle.h"

iwHarborBuilding::iwHarborBuilding(GameWorldView& gwv, GameCommandFactory& gcFactory, nobHarborBuilding* hb) : iwHQ(gwv, gcFactory, hb)
{
    SetTitle(_("Harbor building"));

    // Zusätzliche Hafenseite
    ctrlGroup& harbor_page = AddPage();
    grpIdExpedition = harbor_page.GetID();

    // "Expedition"-Überschrift
    harbor_page.AddText(0, DrawPoint(83, 70), _("Expedition"), 0xFFFFFF00, FontStyle::CENTER, NormalFont);

    // Button zum Expedition starten
    harbor_page.AddImageButton(1, DrawPoint(65, 100), Extent(30, 30), TC_GREY, LOADER.GetImageN("io", 176), _("Start expedition"));
    AdjustExpeditionButton(false);

    // "Expedition"-Überschrift
    harbor_page.AddText(2, DrawPoint(83, 140), _("Exploration expedition"), 0xFFFFFF00, FontStyle::CENTER, NormalFont);

    // Button zum Expedition starten
    harbor_page.AddImageButton(3, DrawPoint(65, 170), Extent(30, 30), TC_GREY, LOADER.GetImageN("io", 176),
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
    auto* button = GetCtrl<ctrlGroup>(grpIdExpedition)->GetCtrl<ctrlImageButton>(1);

    // Visuelle Rückmeldung, grün einfärben, wenn Expedition gestartet wurde
    // Jeweils umgekehrte Farbe nehmen, da die änderung ja spielerisch noch nicht
    // in Kraft getreten ist!
    bool exp = static_cast<const nobHarborBuilding*>(wh)->IsExpeditionActive();

    // "flip xor exp", damit korrekt geswitcht wird, falls expedition abgebrochen werden soll
    // und dies direkt dargestellt werden soll (flip)
    if((flip || exp) && !(flip && exp))
    {
        button->SetModulationColor(COLOR_WHITE);
        button->SetTooltip(_("Cancel expedition"));
    } else
    {
        button->SetModulationColor(COLOR_RED);
        button->SetTooltip(_("Start expedition"));
    }
}

/**
 *  setzt den Expeditionsknopf korrekt
 *
 *  falls @p flip gesetzt, dann umgekehrt einfärben
 */
void iwHarborBuilding::AdjustExplorationExpeditionButton(bool flip)
{
    auto* button = GetCtrl<ctrlGroup>(grpIdExpedition)->GetCtrl<ctrlImageButton>(3);

    // Visuelle Rückmeldung, grün einfärben, wenn Expedition gestartet wurde
    // Jeweils umgekehrte Farbe nehmen, da die änderung ja spielerisch noch nicht
    // in Kraft getreten ist!
    bool exp = static_cast<const nobHarborBuilding*>(wh)->IsExplorationExpeditionActive();

    // "flip xor exp", damit korrekt geswitcht wird, falls expedition abgebrochen werden soll
    // und dies direkt dargestellt werden soll (flip)
    if((flip || exp) && !(flip && exp))
    {
        button->SetModulationColor(COLOR_WHITE);
        button->SetTooltip(_("Cancel expedition"));
    } else
    {
        button->SetModulationColor(COLOR_RED);
        button->SetTooltip(_("Start expedition"));
    }
}

void iwHarborBuilding::Msg_Group_ButtonClick(const unsigned group_id, const unsigned ctrl_id)
{
    if(group_id == grpIdExpedition) // Hafengruppe?
    {
        switch(ctrl_id)
        {
            case 1: // Expedition starten
                // Entsprechenden GC senden
                if(GAMECLIENT.StartStopExpedition(wh->GetPos(), !static_cast<const nobHarborBuilding*>(wh)->IsExpeditionActive()))
                    AdjustExpeditionButton(true);
                break;
            case 3: // Expedition starten
                // Entsprechenden GC senden
                if(GAMECLIENT.StartStopExplorationExpedition(wh->GetPos(),
                                                             !static_cast<const nobHarborBuilding*>(wh)->IsExplorationExpeditionActive()))
                    AdjustExplorationExpeditionButton(true);
                break;
        }
    }

    // an Basis weiterleiten
    iwHQ::Msg_Group_ButtonClick(group_id, ctrl_id);
}
