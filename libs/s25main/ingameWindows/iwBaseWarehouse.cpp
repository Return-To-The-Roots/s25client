// Copyright (C) 2005 - 2026 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "iwBaseWarehouse.h"
#include "GamePlayer.h"
#include "Loader.h"
#include "WindowManager.h"
#include "buildings/nobBaseWarehouse.h"
#include "buildings/nobHarborBuilding.h"
#include "buildings/nobStorehouse.h"
#include "controls/ctrlButton.h"
#include "controls/ctrlGroup.h"
#include "controls/ctrlImage.h"
#include "controls/ctrlOptionGroup.h"
#include "helpers/containerUtils.h"
#include "iwDemolishBuilding.h"
#include "iwHQ.h"
#include "iwHarborBuilding.h"
#include "iwHelp.h"
#include "network/GameClient.h"
#include "world/GameWorldBase.h"
#include "world/GameWorldView.h"
#include "gameData/BuildingConsts.h"
#include "gameData/const_gui_ids.h"
#include <stdexcept>

namespace {
enum
{
    // From iwWares
    ID_PAGINATE = 0,
    ID_HELP = 12,
    // New
    ID_STORE_SETTINGS_GROUP,
    ID_COLLECT,
    ID_TAKEOUT,
    ID_STOP,
    ID_SELECT_ALL,
    ID_GOTO,
    ID_GOTO_NEXT,
    ID_DEMOLISH
};
}

iwBaseWarehouse::iwBaseWarehouse(GameWorldView& gwv, GameCommandFactory& gcFactory, nobBaseWarehouse* wh)
    : iwWares(CGI_BUILDING + MapBase::createGUIID(wh->getPos()), IngameWindow::posAtMouse, 40,
              _(BUILDING_NAMES[wh->getBuildingType()]), true, NormalFont, wh->getInventory(),
              gwv.getWorld().getPlayer(wh->getPlayer())),
      gwv(gwv), gcFactory(gcFactory), wh(wh)
{
    wh->addListener(this);

    // Basisinitialisierungsänderungen
    background = LOADER.getImageN("resource", 41);

    // Auswahl für Auslagern/Einlagern Verbieten-Knöpfe
    ctrlOptionGroup* group = addOptionGroup(ID_STORE_SETTINGS_GROUP, GroupSelectType::Check);
    // Einlagern
    group->addImageButton(ID_COLLECT, DrawPoint(16, getFullSize().y - 81), Extent(32, 32), TextureColor::Grey,
                          LOADER.getImageN("io_new", 4), _("Collect"));
    // Auslagern
    group->addImageButton(ID_TAKEOUT, DrawPoint(52, getFullSize().y - 81), Extent(32, 32), TextureColor::Grey,
                          LOADER.getImageN("io", 211), _("Take out of store"));
    // Einlagern verbieten
    group->addImageButton(ID_STOP, DrawPoint(86, getFullSize().y - 81), Extent(32, 32), TextureColor::Grey,
                          LOADER.getImageN("io", 212), _("Stop storage"));
    // nix tun auswählen
    group->setSelection(ID_COLLECT);
    // Alle auswählen bzw setzen!
    addImageButton(ID_SELECT_ALL, DrawPoint(122, getFullSize().y - 81), Extent(32, 32), TextureColor::Grey,
                   LOADER.getImageN("io", 223), _("Select all"));

    // "Gehe Zu Ort"
    addImageButton(ID_GOTO, DrawPoint(122, getFullSize().y - 47), Extent(15, 32), TextureColor::Grey,
                   LOADER.getImageN("io_new", 10), _("Go to place"));
    // Go to next warehouse
    addImageButton(ID_GOTO_NEXT, DrawPoint(139, getFullSize().y - 47), Extent(15, 32), TextureColor::Grey,
                   LOADER.getImageN("io_new", 13), _("Go to next warehouse"));

    updateOverlays();

    // Add demolish button if not HQ
    if(wh->getGOT() != GO_Type::NobHq)
    {
        // Make paginate button smaller and move to make space for demolish button
        getCtrl<ctrlButton>(ID_PAGINATE)->setWidth(32);
        getCtrl<ctrlButton>(ID_PAGINATE)->setPos(DrawPoint(86, getFullSize().y - 47));

        addImageButton(ID_DEMOLISH, DrawPoint(52, getFullSize().y - 47), Extent(32, 32), TextureColor::Grey,
                       LOADER.getImageN("io", 23), _("Demolish house"));
    }
}

iwBaseWarehouse::~iwBaseWarehouse()
{
    if(wh)
        wh->removeListener(this);
}

void iwBaseWarehouse::msgGroupButtonClick(const unsigned group_id, const unsigned ctrl_id)
{
    if(group_id != warePageID && group_id != peoplePageID)
        iwWares::msgGroupButtonClick(group_id, ctrl_id);
    else
    {
        if(GAMECLIENT.isReplayModeOn())
            return;
        RTTR_Assert(getCurPage() == peoplePageID || getCurPage() == warePageID);
        auto* optiongroup = getCtrl<ctrlOptionGroup>(ID_STORE_SETTINGS_GROUP);

        EInventorySetting setting;
        switch(optiongroup->getSelection())
        {
            case ID_COLLECT: setting = EInventorySetting::Collect; break;
            case ID_TAKEOUT: setting = EInventorySetting::Send; break;
            case ID_STOP: setting = EInventorySetting::Stop; break;
            default: throw std::invalid_argument("iwBaseWarehouse::Optiongroup");
        }
        auto setSetting = [this](auto what, EInventorySetting setting) {
            InventorySetting state = wh->getInventorySettingVisual(what);
            state.toggle(setting);
            if(gcFactory.SetInventorySetting(wh->getPos(), what, state))
            {
                // optisch schonmal setzen
                wh->setInventorySettingVisual(what, state);
                updateOverlay(rttr::enum_cast(what));
            }
        };
        if(getCurPage() == warePageID)
            setSetting(GoodType(ctrl_id - 100), setting);
        else
            setSetting(Job(ctrl_id - 100), setting);
    }
}

void iwBaseWarehouse::msgButtonClick(const unsigned ctrl_id)
{
    switch(ctrl_id)
    {
        case ID_DEMOLISH: // Gebäude abreißen
        {
            // Abreißen?
            close();
            WINDOWMANAGER.show(std::make_unique<iwDemolishBuilding>(gwv, wh));
        }
        break;
        case ID_SELECT_ALL: // "Alle auswählen"
        {
            if(GAMECLIENT.isReplayModeOn())
                return;
            if(getCurPage() != warePageID && getCurPage() != peoplePageID)
                return;
            auto* optiongroup = getCtrl<ctrlOptionGroup>(ID_STORE_SETTINGS_GROUP);
            EInventorySetting data;
            switch(optiongroup->getSelection())
            {
                case ID_COLLECT: data = EInventorySetting::Collect; break;
                case ID_TAKEOUT: data = EInventorySetting::Send; break;
                case ID_STOP: data = EInventorySetting::Stop; break;
                default: throw std::invalid_argument("iwBaseWarehouse::Optiongroup");
            }
            const unsigned count =
              (getCurPage() == warePageID) ? helpers::NumEnumValues_v<GoodType> : helpers::NumEnumValues_v<Job>;
            std::vector<InventorySetting> states;
            states.reserve(count);
            if(getCurPage() == warePageID)
            {
                for(const auto i : helpers::enumRange<GoodType>())
                    states.push_back(wh->getInventorySettingVisual(i == GoodType::WaterEmpty ? GoodType::Water : i));
            } else
            {
                for(const auto i : helpers::enumRange<Job>())
                    states.push_back(wh->getInventorySettingVisual(i));
            }
            // Check if we need to enable all or disable all
            // If at least 1 disabled is found, enable all
            bool enable = false;
            for(unsigned i = 0; i < count; i++)
            {
                if(!states[i].isSet(data))
                {
                    enable = true;
                    break;
                }
            }
            for(unsigned i = 0; i < count; i++)
            {
                if(states[i].isSet(data) != enable)
                    states[i].toggle(data);
            }

            if(gcFactory.SetAllInventorySettings(wh->getPos(), getCurPage() == peoplePageID, states))
            {
                // optisch setzen
                if(getCurPage() == warePageID)
                {
                    for(const auto i : helpers::enumRange<GoodType>())
                        wh->setInventorySettingVisual(i, states[rttr::enum_cast(i)]);
                } else
                {
                    for(const auto i : helpers::enumRange<Job>())
                        wh->setInventorySettingVisual(i, states[rttr::enum_cast(i)]);
                }
                updateOverlays();
            }
        }
        break;
        case ID_HELP: // "Hilfe"
        {
            WINDOWMANAGER.replaceWindow(std::make_unique<iwHelp>(_(BUILDING_HELP_STRINGS[wh->getBuildingType()])));
        }
        break;
        case ID_GOTO: // "Gehe Zu Ort"
        {
            gwv.moveToMapPt(wh->getPos());
        }
        break;
        case ID_GOTO_NEXT: // go to next of same type
        {
            // is there at least 1 other building of the same type?
            const std::list<nobBaseWarehouse*>& storehouses =
              gwv.getWorld().getPlayer(wh->getPlayer()).getBuildingRegister().getStorehouses();
            // go through list once we get to current building -> open window for the next one and go to next location
            auto it =
              helpers::find_if(storehouses, [whPos = wh->getPos()](const auto* it) { return it->getPos() == whPos; });
            if(it != storehouses.end()) // got to current building in the list?
            {
                // close old window, open new window (todo: only open if it isnt already open), move to location of next
                // building
                close();
                ++it;
                if(it == storehouses.end()) // was last entry in list -> goto first
                    it = storehouses.begin();
                gwv.moveToMapPt((*it)->getPos());
                if((*it)->getBuildingType() == BuildingType::Headquarters)
                {
                    WINDOWMANAGER.replaceWindow(std::make_unique<iwHQ>(gwv, gcFactory, *it)).setPos(getPos());
                } else if((*it)->getBuildingType() == BuildingType::HarborBuilding)
                {
                    WINDOWMANAGER
                      .replaceWindow(
                        std::make_unique<iwHarborBuilding>(gwv, gcFactory, dynamic_cast<nobHarborBuilding*>(*it)))
                      .setPos(getPos());
                } else if((*it)->getBuildingType() == BuildingType::Storehouse)
                {
                    WINDOWMANAGER
                      .replaceWindow(
                        std::make_unique<iwBaseWarehouse>(gwv, gcFactory, dynamic_cast<nobStorehouse*>(*it)))
                      .setPos(getPos());
                }
                break;
            }
        }
        break;
        default: // an Basis weiterleiten
        {
            iwWares::msgButtonClick(ctrl_id);
        }
        break;
    }
}

void iwBaseWarehouse::setPage(unsigned page)
{
    iwWares::setPage(page);
    const bool showStorageSettings = getCurPage() == warePageID || getCurPage() == peoplePageID;
    getCtrl<ctrlOptionGroup>(ID_STORE_SETTINGS_GROUP)->getButton(ID_COLLECT)->setEnabled(showStorageSettings);
    getCtrl<ctrlOptionGroup>(ID_STORE_SETTINGS_GROUP)->getButton(ID_STOP)->setEnabled(showStorageSettings);
    getCtrl<ctrlOptionGroup>(ID_STORE_SETTINGS_GROUP)->getButton(ID_TAKEOUT)->setEnabled(showStorageSettings);
    getCtrl<ctrlButton>(ID_SELECT_ALL)->setEnabled(showStorageSettings);
}

void iwBaseWarehouse::updateOverlay(unsigned i)
{
    updateOverlay(i, getCurPage() == warePageID);
}

void iwBaseWarehouse::updateOverlay(unsigned i, bool isWare)
{
    auto* group = getCtrl<ctrlGroup>(isWare ? warePageID : peoplePageID);
    // Einlagern verbieten-Bild (de)aktivieren
    auto* image = group->getCtrl<ctrlImage>(400 + i);
    if(image)
        image->setVisible(isWare ? wh->isInventorySettingVisual(GoodType(i), EInventorySetting::Stop) :
                                   wh->isInventorySettingVisual(Job(i), EInventorySetting::Stop));

    // Auslagern-Bild (de)aktivieren
    image = group->getCtrl<ctrlImage>(500 + i);
    if(image)
        image->setVisible(isWare ? wh->isInventorySettingVisual(GoodType(i), EInventorySetting::Send) :
                                   wh->isInventorySettingVisual(Job(i), EInventorySetting::Send));

    // Einlagern-Bild (de)aktivieren
    image = group->getCtrl<ctrlImage>(700 + i);
    if(image)
        image->setVisible(isWare ? wh->isInventorySettingVisual(GoodType(i), EInventorySetting::Collect) :
                                   wh->isInventorySettingVisual(Job(i), EInventorySetting::Collect));
}

void iwBaseWarehouse::updateOverlays()
{
    // Ein/Auslager Overlays entsprechend setzen
    for(unsigned char category = 0; category < 2; ++category)
    {
        unsigned count = (category == 0) ? helpers::NumEnumValues_v<GoodType> : helpers::NumEnumValues_v<Job>;
        for(unsigned i = 0; i < count; ++i)
        {
            updateOverlay(i, category == 0);
        }
    }
}

void iwBaseWarehouse::onChange(unsigned changeId)
{
    if(changeId == 0)
    {
        wh = nullptr;
        close();
    } else if(changeId == 1)
        updateOverlays();
}
