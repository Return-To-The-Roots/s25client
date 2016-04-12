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
#include "iwHarborBuilding.h"

#include "Loader.h"
#include "buildings/nobHarborBuilding.h"
#include "controls/ctrlGroup.h"
#include "GameClient.h"
#include "controls/ctrlButton.h"
#include "ogl/glArchivItem_Font.h"

// Include last!
#include "DebugNew.h" // IWYU pragma: keep
class GameWorldViewer;

iwHarborBuilding::iwHarborBuilding(GameWorldView& gwv, nobHarborBuilding* hb)
    : iwHQ(gwv, hb, _("Harbor building"), 4)
{
    // Zusätzliche Hafenseite
    ctrlGroup* harbor_page = AddGroup(103);

    // "Expedition"-Überschrift
    harbor_page->AddText(0, 83, 70, _("Expedition"), 0xFFFFFF00, glArchivItem_Font::DF_CENTER, NormalFont);

    // Button zum Expedition starten
    harbor_page->AddImageButton(1, 65, 100, 30, 30, TC_GREY, LOADER.GetImageN("io", 176), _("Start expedition"));
    AdjustExpeditionButton(false);

    // "Expedition"-Überschrift
    harbor_page->AddText(2, 83, 140, _("Exploration expedition"), 0xFFFFFF00, glArchivItem_Font::DF_CENTER, NormalFont);

    // Button zum Expedition starten
    harbor_page->AddImageButton(3, 65, 170, 30, 30, TC_GREY, LOADER.GetImageN("io", 176), _("Start exporation expedition"));
    AdjustExplorationExpeditionButton(false);

    harbor_page->SetVisible(false);
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  setzt den Expeditionsknopf korrekt
 *
 *  falls @p flip gesetzt, dann umgekehrt einfärben
 *
 *  @author FloSoft
 */
void iwHarborBuilding::AdjustExpeditionButton(bool flip)
{
    ctrlImageButton* button = GetCtrl<ctrlGroup>(103)->GetCtrl<ctrlImageButton>(1);

    // Visuelle Rückmeldung, grün einfärben, wenn Expedition gestartet wurde
    // Jeweils umgekehrte Farbe nehmen, da die änderung ja spielerisch noch nicht
    // in Kraft getreten ist!
    bool exp = static_cast<nobHarborBuilding*>(wh)->IsExpeditionActive();

    // "flip xor exp", damit korrekt geswitcht wird, falls expedition abgebrochen werden soll
    // und dies direkt dargestellt werden soll (flip)
    if( (flip || exp) && !(flip && exp))
    {
        button->SetModulationColor(COLOR_WHITE);
        button->SetTooltip(_("Cancel expedition"));
    }
    else
    {
        button->SetModulationColor(COLOR_RED);
        button->SetTooltip(_("Start expedition"));
    }
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  setzt den Expeditionsknopf korrekt
 *
 *  falls @p flip gesetzt, dann umgekehrt einfärben
 *
 *  @author OLiver
 */
void iwHarborBuilding::AdjustExplorationExpeditionButton(bool flip)
{
    ctrlImageButton* button = GetCtrl<ctrlGroup>(103)->GetCtrl<ctrlImageButton>(3);

    // Visuelle Rückmeldung, grün einfärben, wenn Expedition gestartet wurde
    // Jeweils umgekehrte Farbe nehmen, da die änderung ja spielerisch noch nicht
    // in Kraft getreten ist!
    bool exp = static_cast<nobHarborBuilding*>(wh)->IsExplorationExpeditionActive();

    // "flip xor exp", damit korrekt geswitcht wird, falls expedition abgebrochen werden soll
    // und dies direkt dargestellt werden soll (flip)
    if( (flip || exp) && !(flip && exp))
    {
        button->SetModulationColor(COLOR_WHITE);
        button->SetTooltip(_("Cancel expedition"));
    }
    else
    {
        button->SetModulationColor(COLOR_RED);
        button->SetTooltip(_("Start expedition"));
    }
}

void iwHarborBuilding::Msg_Group_ButtonClick(const unsigned int group_id, const unsigned int ctrl_id)
{
    switch(group_id)
    {
        default:
            break;

        case 103: // Hafengruppe?
        {
            switch(ctrl_id)
            {
                default:
                    break;

                case 1: // Expedition starten
                {
                    // Entsprechenden GC senden
                    if(GAMECLIENT.StartExpedition(wh->GetPos()))
                        AdjustExpeditionButton(true);
                } break;
                case 3: // Expedition starten
                {
                    // Entsprechenden GC senden
                    if(GAMECLIENT.StartExplorationExpedition(wh->GetPos()))
                        AdjustExplorationExpeditionButton(true);
                } break;
            }
        } break;
    }

    // an Basis weiterleiten
    iwHQ::Msg_Group_ButtonClick(group_id, ctrl_id);
}

