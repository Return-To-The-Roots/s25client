// Copyright (C) 2005 - 2026 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "iwBuildOrder.h"
#include "AddonHelperFunctions.h"
#include "GamePlayer.h"
#include "GlobalGameSettings.h"
#include "LeatherLoader.h"
#include "Loader.h"
#include "WindowManager.h"
#include "WineLoader.h"
#include "addons/const_addons.h"
#include "controls/ctrlComboBox.h"
#include "controls/ctrlImage.h"
#include "controls/ctrlList.h"
#include "network/GameClient.h"
#include "world/GameWorldBase.h"
#include "world/GameWorldViewer.h"
#include "gameData/BuildingConsts.h"
#include "gameData/const_gui_ids.h"

void iwBuildOrder::fillBuildOrder(const BuildOrders& build_order)
{
    pendingBuildOrder.assign(build_order.begin(), build_order.end());
    helpers::erase_if(pendingBuildOrder, makeIsUnusedBuilding(gwv.getWorld().getGGS()));
}

iwBuildOrder::iwBuildOrder(const GameWorldViewer& gwv)
    : TransmitSettingsIgwAdapter(CGI_BUILDORDER, IngameWindow::posLastOrCenter, Extent(320, 300),
                                 _("Building sequence"), LOADER.getImageN("io", 5)),
      gwv(gwv)
{
    ctrlList* list = addList(0, DrawPoint(15, 60), Extent(150, 220), TextureColor::Grey, NormalFont);

    fillBuildOrder(GAMECLIENT.visual_settings.build_order);

    for(const auto buildOrder : pendingBuildOrder)
        list->addItem(_(BUILDING_NAMES[buildOrder])); //-V807

    // Nach ganz oben
    addImageButton(1, DrawPoint(250, 194), Extent(48, 20), TextureColor::Grey, LOADER.getImageN("io", 215), _("Top"));
    // Hoch
    addImageButton(2, DrawPoint(250, 216), Extent(48, 20), TextureColor::Grey, LOADER.getImageN("io", 33), _("Up"));
    // Runter
    addImageButton(3, DrawPoint(250, 238), Extent(48, 20), TextureColor::Grey, LOADER.getImageN("io", 34), _("Down"));
    // Nach ganz unten
    addImageButton(4, DrawPoint(250, 260), Extent(48, 20), TextureColor::Grey, LOADER.getImageN("io", 216),
                   _("Bottom"));

    // Bild der Auswahl
    addImage(5, DrawPoint(240, 150), LOADER.getBuildingTex(gwv.getPlayer().nation, pendingBuildOrder[0]));

    ctrlComboBox* combo = addComboBox(6, DrawPoint(15, 30), Extent(290, 20), TextureColor::Grey, NormalFont, 100);
    combo->addItem(_("Sequence of given order"));   // "Reihenfolge der Auftraggebung"
    combo->addItem(_("After the following order")); // "Nach folgender Reihenfolge"

    // Eintrag in Combobox auswählen
    useCustomBuildOrder = GAMECLIENT.visual_settings.useCustomBuildOrder;
    combo->setSelection(useCustomBuildOrder ? 1 : 0);

    // Standard
    addImageButton(10, DrawPoint(200, 250), Extent(48, 30), TextureColor::Grey, LOADER.getImageN("io", 191),
                   _("Default"));

    list->setSelection(0);
}

void iwBuildOrder::transmitSettings()
{
    if(GAMECLIENT.isReplayModeOn())
        return;
    // Wurden Einstellungen geändert?
    if(settings_changed)
    {
        // Einstellungen speichern
        useCustomBuildOrder = getCtrl<ctrlComboBox>(6)->getSelection() != 0u;
        BuildOrders transmitPendingBuildOrder;
        unsigned int i = 0;
        for(; i < pendingBuildOrder.size(); i++)
            transmitPendingBuildOrder[i] = pendingBuildOrder[i];

        if(!wineaddon::isAddonActive(gwv.getWorld()))
        {
            transmitPendingBuildOrder[i++] = BuildingType::Vineyard;
            transmitPendingBuildOrder[i++] = BuildingType::Winery;
            transmitPendingBuildOrder[i++] = BuildingType::Temple;
        }

        // This is not an OBB write because pendingBuildOrder does not contain the leather/wine building
        // entries when the addon is inactive. So the pendingBuildOrder list is shorter then
        // the transmitPendingBuildOrder list. See fillBuildOrder at the top of this file.
        if(!gwv.getWorld().getGGS().isEnabled(AddonId::CHARBURNER))
            transmitPendingBuildOrder[i++] = BuildingType::Charburner;

        if(!leatheraddon::isAddonActive(gwv.getWorld()))
        {
            transmitPendingBuildOrder[i++] = BuildingType::Skinner;
            transmitPendingBuildOrder[i++] = BuildingType::Tannery;
            transmitPendingBuildOrder[i++] = BuildingType::LeatherWorks;
        }

        if(GAMECLIENT.ChangeBuildOrder(useCustomBuildOrder, transmitPendingBuildOrder))
        {
            GAMECLIENT.visual_settings.build_order = transmitPendingBuildOrder;
            GAMECLIENT.visual_settings.useCustomBuildOrder = useCustomBuildOrder;
            settings_changed = false;
        }
    }
}

void iwBuildOrder::msgListSelectItem(const unsigned ctrl_id, const int selection)
{
    if(GAMECLIENT.isReplayModeOn())
        return;
    switch(ctrl_id)
    {
        default: break;

        case 0:
        {
            getCtrl<ctrlImage>(5)->setImage(
              LOADER.getBuildingTex(gwv.getPlayer().nation, pendingBuildOrder[selection]));
        }
        break;
    }
}

void iwBuildOrder::msgComboSelectItem(unsigned ctrl_id, unsigned selection)
{
    if(ctrl_id == 6)
    {
        useCustomBuildOrder = selection != 0u;
        settings_changed = true;
    }
}

void iwBuildOrder::msgButtonClick(const unsigned ctrl_id)
{
    if(GAMECLIENT.isReplayModeOn())
        return;
    auto* list = getCtrl<ctrlList>(0);

    if(!list->getSelection())
        return;

    unsigned selection = *list->getSelection();
    unsigned numOptions = list->getNumLines();

    // Auswahl gültig?

    switch(ctrl_id)
    {
        default: break;

        case 1: // Nach ganz oben
        {
            while(selection > 0)
            {
                std::swap(pendingBuildOrder[selection - 1], pendingBuildOrder[selection]);
                list->swap(selection - 1, selection);
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
                list->swap(selection - 1, selection);
            }
            settings_changed = true;
        }
        break;
        case 3: // Runter
        {
            if(selection < numOptions - 1u)
            {
                std::swap(pendingBuildOrder[selection + 1], pendingBuildOrder[selection]);
                list->swap(selection + 1, selection);
            }
            settings_changed = true;
        }
        break;
        case 4: // Nach ganz unten
        {
            while(selection < numOptions - 1u)
            {
                std::swap(pendingBuildOrder[selection + 1], pendingBuildOrder[selection]);
                list->swap(selection + 1, selection);
                ++selection;
            }
            settings_changed = true;
        }
        break;
        case 10: // Standardwerte
        {
            // Baureihenfolge vom Spieler kopieren
            fillBuildOrder(GAMECLIENT.default_settings.build_order);

            auto* list = getCtrl<ctrlList>(0);
            list->deleteAllItems();

            // Liste füllen
            for(unsigned char i = 0; i < 31; ++i)
                list->addItem(_(BUILDING_NAMES[pendingBuildOrder[i]]));
            list->setSelection(0);

            getCtrl<ctrlImage>(5)->setImage(LOADER.getBuildingTex(gwv.getPlayer().nation, pendingBuildOrder[0]));

            settings_changed = true;
        }
        break;
    }
}

void iwBuildOrder::updateSettings()
{
    if(GAMECLIENT.isReplayModeOn())
    {
        gwv.getPlayer().fillVisualSettings(GAMECLIENT.visual_settings);
        fillBuildOrder(GAMECLIENT.visual_settings.build_order);
        useCustomBuildOrder = GAMECLIENT.visual_settings.useCustomBuildOrder;
    }
    getCtrl<ctrlComboBox>(6)->setSelection(useCustomBuildOrder ? 1 : 0);
    for(unsigned char i = 0; i < pendingBuildOrder.size(); ++i)
        getCtrl<ctrlList>(0)->setItemText(i, _(BUILDING_NAMES[pendingBuildOrder[i]]));
}
