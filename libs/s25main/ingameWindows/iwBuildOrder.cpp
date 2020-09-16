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

#include "iwBuildOrder.h"
#include "GamePlayer.h"
#include "Loader.h"
#include "controls/ctrlComboBox.h"
#include "controls/ctrlImage.h"
#include "controls/ctrlList.h"
#include "network/GameClient.h"
#include "world/GameWorldViewer.h"
#include "gameData/BuildingConsts.h"
#include "gameData/const_gui_ids.h"

iwBuildOrder::iwBuildOrder(const GameWorldViewer& gwv)
    : IngameWindow(CGI_BUILDORDER, IngameWindow::posLastOrCenter, Extent(320, 300), _("Building sequence"),
                   LOADER.GetImageN("io", 5)),
      gwv(gwv), settings_changed(false)
{
    ctrlList* list = AddList(0, DrawPoint(15, 60), Extent(150, 220), TC_GREY, NormalFont);

    // Liste füllen
    BuildOrders buildOrders = GAMECLIENT.visual_settings.build_order;
    for(auto& buildOrder : buildOrders)
        list->AddString(_(BUILDING_NAMES[buildOrder])); //-V807

    // Nach ganz oben
    AddImageButton(1, DrawPoint(250, 194), Extent(48, 20), TC_GREY, LOADER.GetImageN("io", 215), _("Top"));
    // Hoch
    AddImageButton(2, DrawPoint(250, 216), Extent(48, 20), TC_GREY, LOADER.GetImageN("io", 33), _("Up"));
    // Runter
    AddImageButton(3, DrawPoint(250, 238), Extent(48, 20), TC_GREY, LOADER.GetImageN("io", 34), _("Down"));
    // Nach ganz unten
    AddImageButton(4, DrawPoint(250, 260), Extent(48, 20), TC_GREY, LOADER.GetImageN("io", 216), _("Bottom"));

    // Bild der Auswahl
    AddImage(5, DrawPoint(240, 150), LOADER.GetNationImage(gwv.GetPlayer().nation, 250 + buildOrders[0] * 5));

    ctrlComboBox* combo = AddComboBox(6, DrawPoint(15, 30), Extent(290, 20), TC_GREY, NormalFont, 100);
    combo->AddString(_("Sequence of given order"));   // "Reihenfolge der Auftraggebung"
    combo->AddString(_("After the following order")); // "Nach folgender Reihenfolge"

    // Eintrag in Combobox auswählen
    combo->SetSelection(GAMECLIENT.visual_settings.useCustomBuildOrder ? 1 : 0);

    // Standard
    AddImageButton(10, DrawPoint(200, 250), Extent(48, 30), TC_GREY, LOADER.GetImageN("io", 191), _("Default"));

    // Absendetimer, in 2s-Abschnitten wird jeweils das ganze als Netzwerknachricht ggf. abgeschickt
    AddTimer(11, 2000);

    list->SetSelection(0);
}

iwBuildOrder::~iwBuildOrder()
{
    try
    {
        GAMECLIENT.visual_settings.useCustomBuildOrder = GetCtrl<ctrlComboBox>(6)->GetSelection() == 1u;

        TransmitSettings();
    } catch(...)
    {
        // Ignored
    }
}

void iwBuildOrder::TransmitSettings()
{
    if(GAMECLIENT.IsReplayModeOn())
        return;
    // Wurden Einstellungen geändert?
    if(settings_changed)
    {
        // Einstellungen speichern
        GAMECLIENT.ChangeBuildOrder(GetCtrl<ctrlComboBox>(6)->GetSelection() != 0u,
                                    GAMECLIENT.visual_settings.build_order);
        settings_changed = false;
    }
}

void iwBuildOrder::Msg_Timer(const unsigned /*ctrl_id*/)
{
    if(GAMECLIENT.IsReplayModeOn())
        // Im Replay aktualisieren wir die Werte
        UpdateSettings();
    else
        // Im normalen Spielmodus schicken wir den ganzen Spaß ab
        TransmitSettings();
}

void iwBuildOrder::Msg_ListSelectItem(const unsigned ctrl_id, const int selection)
{
    if(GAMECLIENT.IsReplayModeOn())
        return;
    switch(ctrl_id)
    {
        default: break;

        case 0:
        {
            GetCtrl<ctrlImage>(5)->SetImage(
              LOADER.GetNationTex(gwv.GetPlayer().nation, 250 + GAMECLIENT.visual_settings.build_order[selection] * 5));
        }
        break;
    }
}

void iwBuildOrder::Msg_ButtonClick(const unsigned ctrl_id)
{
    if(GAMECLIENT.IsReplayModeOn())
        return;
    auto* list = GetCtrl<ctrlList>(0);

    if(!list->GetSelection())
        return;

    unsigned selection = *list->GetSelection();
    unsigned numOptions = list->GetNumLines();

    // Auswahl gültig?

    switch(ctrl_id)
    {
        default: break;

        case 1: // Nach ganz oben
        {
            while(selection > 0)
            {
                std::swap(GAMECLIENT.visual_settings.build_order[selection - 1],
                          GAMECLIENT.visual_settings.build_order[selection]);
                list->Swap(selection - 1, selection);
                --selection;
            }
            settings_changed = true;
        }
        break;
        case 2: // Hoch
        {
            if(selection > 0)
            {
                std::swap(GAMECLIENT.visual_settings.build_order[selection - 1],
                          GAMECLIENT.visual_settings.build_order[selection]);
                list->Swap(selection - 1, selection);
            }
            settings_changed = true;
        }
        break;
        case 3: // Runter
        {
            if(selection < numOptions - 1u)
            {
                std::swap(GAMECLIENT.visual_settings.build_order[selection + 1],
                          GAMECLIENT.visual_settings.build_order[selection]);
                list->Swap(selection + 1, selection);
            }
            settings_changed = true;
        }
        break;
        case 4: // Nach ganz unten
        {
            while(selection < numOptions - 1u)
            {
                std::swap(GAMECLIENT.visual_settings.build_order[selection + 1],
                          GAMECLIENT.visual_settings.build_order[selection]);
                list->Swap(selection + 1, selection);
                ++selection;
            }
            settings_changed = true;
        }
        break;
        case 10: // Standardwerte
        {
            // Baureihenfolge vom Spieler kopieren
            GAMECLIENT.visual_settings.build_order = GAMECLIENT.default_settings.build_order;

            auto* list = GetCtrl<ctrlList>(0);
            list->DeleteAllItems();

            // Liste füllen
            for(unsigned char i = 0; i < 31; ++i)
                list->AddString(_(BUILDING_NAMES[GAMECLIENT.default_settings.build_order[i]]));
            list->SetSelection(0);

            GetCtrl<ctrlImage>(5)->SetImage(
              LOADER.GetNationTex(gwv.GetPlayer().nation, 250 + GAMECLIENT.visual_settings.build_order[0] * 5));

            settings_changed = true;
        }
        break;
    }
}

void iwBuildOrder::UpdateSettings()
{
    if(GAMECLIENT.IsReplayModeOn())
        gwv.GetPlayer().FillVisualSettings(GAMECLIENT.visual_settings);
    GetCtrl<ctrlComboBox>(6)->SetSelection(GAMECLIENT.visual_settings.useCustomBuildOrder ? 1 : 0);
    for(unsigned char i = 0; i < 31; ++i)
        GetCtrl<ctrlList>(0)->SetString(_(BUILDING_NAMES[GAMECLIENT.visual_settings.build_order[i]]), i);
}
