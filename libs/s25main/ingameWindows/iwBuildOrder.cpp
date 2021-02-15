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

#include "iwBuildOrder.h"
#include "GamePlayer.h"
#include "Loader.h"
#include "WindowManager.h"
#include "controls/ctrlComboBox.h"
#include "controls/ctrlImage.h"
#include "controls/ctrlList.h"
#include "network/GameClient.h"
#include "world/GameWorldViewer.h"
#include "gameData/BuildingConsts.h"
#include "gameData/const_gui_ids.h"

iwBuildOrder::iwBuildOrder(const GameWorldViewer& gwv)
    : TransmitSettingsIgwAdapter(CGI_BUILDORDER, IngameWindow::posLastOrCenter, Extent(320, 300),
                                 _("Building sequence"), LOADER.GetImageN("io", 5)),
      gwv(gwv)
{
    ctrlList* list = AddList(0, DrawPoint(15, 60), Extent(150, 220), TextureColor::Grey, NormalFont);

    // Liste füllen
    pendingBuildOrder = GAMECLIENT.visual_settings.build_order;
    for(const auto buildOrder : pendingBuildOrder)
        list->AddString(_(BUILDING_NAMES[buildOrder])); //-V807

    // Nach ganz oben
    AddImageButton(1, DrawPoint(250, 194), Extent(48, 20), TextureColor::Grey, LOADER.GetImageN("io", 215), _("Top"));
    // Hoch
    AddImageButton(2, DrawPoint(250, 216), Extent(48, 20), TextureColor::Grey, LOADER.GetImageN("io", 33), _("Up"));
    // Runter
    AddImageButton(3, DrawPoint(250, 238), Extent(48, 20), TextureColor::Grey, LOADER.GetImageN("io", 34), _("Down"));
    // Nach ganz unten
    AddImageButton(4, DrawPoint(250, 260), Extent(48, 20), TextureColor::Grey, LOADER.GetImageN("io", 216),
                   _("Bottom"));

    // Bild der Auswahl
    AddImage(5, DrawPoint(240, 150),
             LOADER.GetNationImage(gwv.GetPlayer().nation, 250 + rttr::enum_cast(pendingBuildOrder[0]) * 5));

    ctrlComboBox* combo = AddComboBox(6, DrawPoint(15, 30), Extent(290, 20), TextureColor::Grey, NormalFont, 100);
    combo->AddString(_("Sequence of given order"));   // "Reihenfolge der Auftraggebung"
    combo->AddString(_("After the following order")); // "Nach folgender Reihenfolge"

    // Eintrag in Combobox auswählen
    useCustomBuildOrder = GAMECLIENT.visual_settings.useCustomBuildOrder;
    combo->SetSelection(useCustomBuildOrder ? 1 : 0);

    // Standard
    AddImageButton(10, DrawPoint(200, 250), Extent(48, 30), TextureColor::Grey, LOADER.GetImageN("io", 191),
                   _("Default"));

    list->SetSelection(0);
}

void iwBuildOrder::TransmitSettings()
{
    if(GAMECLIENT.IsReplayModeOn())
        return;
    // Wurden Einstellungen geändert?
    if(settings_changed)
    {
        // Einstellungen speichern
        useCustomBuildOrder = GetCtrl<ctrlComboBox>(6)->GetSelection() != 0u;
        if(GAMECLIENT.ChangeBuildOrder(useCustomBuildOrder, pendingBuildOrder))
        {
            GAMECLIENT.visual_settings.build_order = pendingBuildOrder;
            GAMECLIENT.visual_settings.useCustomBuildOrder = useCustomBuildOrder;
            settings_changed = false;
        }
    }
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
              LOADER.GetNationTex(gwv.GetPlayer().nation, 250 + rttr::enum_cast(pendingBuildOrder[selection]) * 5));
        }
        break;
    }
}

void iwBuildOrder::Msg_ComboSelectItem(unsigned ctrl_id, unsigned selection)
{
    if(ctrl_id == 6)
    {
        useCustomBuildOrder = selection != 0u;
        settings_changed = true;
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
                std::swap(pendingBuildOrder[selection - 1], pendingBuildOrder[selection]);
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
                std::swap(pendingBuildOrder[selection - 1], pendingBuildOrder[selection]);
                list->Swap(selection - 1, selection);
            }
            settings_changed = true;
        }
        break;
        case 3: // Runter
        {
            if(selection < numOptions - 1u)
            {
                std::swap(pendingBuildOrder[selection + 1], pendingBuildOrder[selection]);
                list->Swap(selection + 1, selection);
            }
            settings_changed = true;
        }
        break;
        case 4: // Nach ganz unten
        {
            while(selection < numOptions - 1u)
            {
                std::swap(pendingBuildOrder[selection + 1], pendingBuildOrder[selection]);
                list->Swap(selection + 1, selection);
                ++selection;
            }
            settings_changed = true;
        }
        break;
        case 10: // Standardwerte
        {
            // Baureihenfolge vom Spieler kopieren
            pendingBuildOrder = GAMECLIENT.default_settings.build_order;

            auto* list = GetCtrl<ctrlList>(0);
            list->DeleteAllItems();

            // Liste füllen
            for(unsigned char i = 0; i < 31; ++i)
                list->AddString(_(BUILDING_NAMES[pendingBuildOrder[i]]));
            list->SetSelection(0);

            GetCtrl<ctrlImage>(5)->SetImage(
              LOADER.GetNationTex(gwv.GetPlayer().nation, 250 + rttr::enum_cast(pendingBuildOrder[0]) * 5));

            settings_changed = true;
        }
        break;
    }
}

void iwBuildOrder::UpdateSettings()
{
    if(GAMECLIENT.IsReplayModeOn())
    {
        gwv.GetPlayer().FillVisualSettings(GAMECLIENT.visual_settings);
        pendingBuildOrder = GAMECLIENT.visual_settings.build_order;
        useCustomBuildOrder = GAMECLIENT.visual_settings.useCustomBuildOrder;
    }
    GetCtrl<ctrlComboBox>(6)->SetSelection(useCustomBuildOrder ? 1 : 0);
    for(unsigned char i = 0; i < 31; ++i)
        GetCtrl<ctrlList>(0)->SetString(_(BUILDING_NAMES[pendingBuildOrder[i]]), i);
}
