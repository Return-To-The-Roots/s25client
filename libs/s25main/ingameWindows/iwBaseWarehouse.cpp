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
    : iwWares(CGI_BUILDING + MapBase::CreateGUIID(wh->GetPos()), IngameWindow::posAtMouse, Extent(167, 416),
              _(BUILDING_NAMES[wh->GetBuildingType()]), true, NormalFont, wh->GetInventory(), gwv.GetWorld().GetPlayer(wh->GetPlayer())),
      gwv(gwv), gcFactory(gcFactory), wh(wh)
{
    wh->AddListener(this);

    // Basisinitialisierungsänderungen
    background = LOADER.GetImageN("resource", 41);

    // Auswahl für Auslagern/Einlagern Verbieten-Knöpfe
    ctrlOptionGroup* group = AddOptionGroup(ID_STORE_SETTINGS_GROUP, ctrlOptionGroup::CHECK);
    // Einlagern
    group->AddImageButton(ID_COLLECT, DrawPoint(16, 335), Extent(32, 32), TC_GREY, LOADER.GetImageN("io_new", 4), _("Collect"));
    // Auslagern
    group->AddImageButton(ID_TAKEOUT, DrawPoint(52, 335), Extent(32, 32), TC_GREY, LOADER.GetImageN("io", 211), _("Take out of store"));
    // Einlagern verbieten
    group->AddImageButton(ID_STOP, DrawPoint(86, 335), Extent(32, 32), TC_GREY, LOADER.GetImageN("io", 212), _("Stop storage"));
    // nix tun auswählen
    group->SetSelection(ID_COLLECT);
    // Alle auswählen bzw setzen!
    AddImageButton(ID_SELECT_ALL, DrawPoint(122, 335), Extent(32, 32), TC_GREY, LOADER.GetImageN("io", 223), _("Select all"));

    // "Gehe Zu Ort"
    AddImageButton(ID_GOTO, DrawPoint(122, 369), Extent(15, 32), TC_GREY, LOADER.GetImageN("io_new", 10), _("Go to place"));
    // Go to next warehouse
    AddImageButton(ID_GOTO_NEXT, DrawPoint(139, 369), Extent(15, 32), TC_GREY, LOADER.GetImageN("io_new", 13), _("Go to next warehouse"));

    UpdateOverlays();

    // Lagerhaus oder Hafengebäude?
    if(wh->GetGOT() == GOT_NOB_STOREHOUSE || wh->GetGOT() == GOT_NOB_HARBORBUILDING)
    {
        // Abbrennbutton hinzufügen
        // "Blättern" in Bretter stauchen und verschieben
        GetCtrl<ctrlButton>(ID_PAGINATE)->SetWidth(32);
        GetCtrl<ctrlButton>(ID_PAGINATE)->SetPos(DrawPoint(86, 369));

        AddImageButton(ID_DEMOLISH, DrawPoint(52, 369), Extent(32, 32), TC_GREY, LOADER.GetImageN("io", 23), _("Demolish house"));
    }
}

iwBaseWarehouse::~iwBaseWarehouse()
{
    if(wh)
        wh->RemoveListener(this);
}

void iwBaseWarehouse::Msg_Group_ButtonClick(const unsigned group_id, const unsigned ctrl_id)
{
    if(group_id != warePageID && group_id != peoplePageID)
        iwWares::Msg_Group_ButtonClick(group_id, ctrl_id);
    else
    {
        if(GAMECLIENT.IsReplayModeOn())
            return;
        RTTR_Assert(GetCurPage() == peoplePageID || GetCurPage() == warePageID);
        auto* optiongroup = GetCtrl<ctrlOptionGroup>(ID_STORE_SETTINGS_GROUP);

        EInventorySetting setting;
        switch(optiongroup->GetSelection())
        {
            case ID_COLLECT: setting = EInventorySetting::COLLECT; break;
            case ID_TAKEOUT: setting = EInventorySetting::SEND; break;
            case ID_STOP: setting = EInventorySetting::STOP; break;
            default: throw std::invalid_argument("iwBaseWarehouse::Optiongroup");
        }
        InventorySetting state = GetCurPage() == warePageID ? wh->GetInventorySettingVisual(GoodType(ctrl_id - 100)) :
                                                              wh->GetInventorySettingVisual(Job(ctrl_id - 100));
        state.Toggle(setting);
        if(gcFactory.SetInventorySetting(wh->GetPos(), GetCurPage() == peoplePageID, ctrl_id - 100, state))
        {
            // optisch schonmal setzen
            wh->SetInventorySettingVisual(GetCurPage() == peoplePageID, ctrl_id - 100, state);
            UpdateOverlay(ctrl_id - 100);
        }
    }
}

void iwBaseWarehouse::Msg_ButtonClick(const unsigned ctrl_id)
{
    switch(ctrl_id)
    {
        case ID_DEMOLISH: // Gebäude abreißen
        {
            // Abreißen?
            Close();
            WINDOWMANAGER.Show(std::make_unique<iwDemolishBuilding>(gwv, wh));
        }
        break;
        case ID_SELECT_ALL: // "Alle auswählen"
        {
            if(GAMECLIENT.IsReplayModeOn())
                return;
            if(GetCurPage() != warePageID && GetCurPage() != peoplePageID)
                return;
            auto* optiongroup = GetCtrl<ctrlOptionGroup>(ID_STORE_SETTINGS_GROUP);
            EInventorySetting data;
            switch(optiongroup->GetSelection())
            {
                case ID_COLLECT: data = EInventorySetting::COLLECT; break;
                case ID_TAKEOUT: data = EInventorySetting::SEND; break;
                case ID_STOP: data = EInventorySetting::STOP; break;
                default: throw std::invalid_argument("iwBaseWarehouse::Optiongroup");
            }
            const unsigned count = (GetCurPage() == warePageID) ? NUM_WARE_TYPES : NUM_JOB_TYPES;
            std::vector<InventorySetting> states;
            states.reserve(count);
            if(GetCurPage() == warePageID)
            {
                for(unsigned i = 0; i < NUM_WARE_TYPES; i++)
                    states.push_back(wh->GetInventorySettingVisual(i == GD_WATEREMPTY ? GD_WATER : GoodType(i)));
            } else
            {
                for(unsigned i = 0; i < NUM_JOB_TYPES; i++)
                    states.push_back(wh->GetInventorySettingVisual(Job(i)));
            }
            // Check if we need to enable all or disable all
            // If at least 1 disabled is found, enable all
            bool enable = false;
            for(unsigned i = 0; i < count; i++)
            {
                if(!states[i].IsSet(data))
                {
                    enable = true;
                    break;
                }
            }
            for(unsigned i = 0; i < count; i++)
            {
                if(states[i].IsSet(data) != enable)
                    states[i].Toggle(data);
            }

            if(gcFactory.SetAllInventorySettings(wh->GetPos(), GetCurPage() == peoplePageID, states))
            {
                // optisch setzen
                for(unsigned char i = 0; i < count; ++i)
                {
                    // Status ändern
                    wh->SetInventorySettingVisual(GetCurPage() == peoplePageID, i, states[i]);
                }
                UpdateOverlays();
            }
        }
        break;
        case ID_HELP: // "Hilfe"
        {
            WINDOWMANAGER.ReplaceWindow(std::make_unique<iwHelp>(_(BUILDING_HELP_STRINGS[wh->GetBuildingType()])));
        }
        break;
        case ID_GOTO: // "Gehe Zu Ort"
        {
            gwv.MoveToMapPt(wh->GetPos());
        }
        break;
        case ID_GOTO_NEXT: // go to next of same type
        {
            // is there at least 1 other building of the same type?
            const std::list<nobBaseWarehouse*>& storehouses =
              gwv.GetWorld().GetPlayer(wh->GetPlayer()).GetBuildingRegister().GetStorehouses();
            // go through list once we get to current building -> open window for the next one and go to next location
            auto it = helpers::find_if(storehouses, [whPos = wh->GetPos()](const auto* it) { return it->GetPos() == whPos; });
            if(it != storehouses.end()) // got to current building in the list?
            {
                // close old window, open new window (todo: only open if it isnt already open), move to location of next building
                Close();
                ++it;
                if(it == storehouses.end()) // was last entry in list -> goto first
                    it = storehouses.begin();
                gwv.MoveToMapPt((*it)->GetPos());
                if((*it)->GetBuildingType() == BLD_HEADQUARTERS)
                {
                    WINDOWMANAGER.ReplaceWindow(std::make_unique<iwHQ>(gwv, gcFactory, *it)).SetPos(GetPos());
                } else if((*it)->GetBuildingType() == BLD_HARBORBUILDING)
                {
                    WINDOWMANAGER.ReplaceWindow(std::make_unique<iwHarborBuilding>(gwv, gcFactory, dynamic_cast<nobHarborBuilding*>(*it)))
                      .SetPos(GetPos());
                } else if((*it)->GetBuildingType() == BLD_STOREHOUSE)
                {
                    WINDOWMANAGER.ReplaceWindow(std::make_unique<iwBaseWarehouse>(gwv, gcFactory, dynamic_cast<nobStorehouse*>(*it)))
                      .SetPos(GetPos());
                }
                break;
            }
        }
        break;
        default: // an Basis weiterleiten
        {
            iwWares::Msg_ButtonClick(ctrl_id);
        }
        break;
    }
}

void iwBaseWarehouse::SetPage(unsigned page)
{
    iwWares::SetPage(page);
    const bool showStorageSettings = GetCurPage() == warePageID || GetCurPage() == peoplePageID;
    GetCtrl<ctrlOptionGroup>(ID_STORE_SETTINGS_GROUP)->GetButton(ID_COLLECT)->SetEnabled(showStorageSettings);
    GetCtrl<ctrlOptionGroup>(ID_STORE_SETTINGS_GROUP)->GetButton(ID_STOP)->SetEnabled(showStorageSettings);
    GetCtrl<ctrlOptionGroup>(ID_STORE_SETTINGS_GROUP)->GetButton(ID_TAKEOUT)->SetEnabled(showStorageSettings);
    GetCtrl<ctrlButton>(ID_SELECT_ALL)->SetEnabled(showStorageSettings);
}

void iwBaseWarehouse::UpdateOverlay(unsigned i)
{
    UpdateOverlay(i, GetCurPage() == warePageID);
}

void iwBaseWarehouse::UpdateOverlay(unsigned i, bool isWare)
{
    auto* group = GetCtrl<ctrlGroup>(isWare ? warePageID : peoplePageID);
    // Einlagern verbieten-Bild (de)aktivieren
    auto* image = group->GetCtrl<ctrlImage>(400 + i);
    if(image)
        image->SetVisible(isWare ? wh->IsInventorySettingVisual(GoodType(i), EInventorySetting::STOP) :
                                   wh->IsInventorySettingVisual(Job(i), EInventorySetting::STOP));

    // Auslagern-Bild (de)aktivieren
    image = group->GetCtrl<ctrlImage>(500 + i);
    if(image)
        image->SetVisible(isWare ? wh->IsInventorySettingVisual(GoodType(i), EInventorySetting::SEND) :
                                   wh->IsInventorySettingVisual(Job(i), EInventorySetting::SEND));

    // Einlagern-Bild (de)aktivieren
    image = group->GetCtrl<ctrlImage>(700 + i);
    if(image)
        image->SetVisible(isWare ? wh->IsInventorySettingVisual(GoodType(i), EInventorySetting::COLLECT) :
                                   wh->IsInventorySettingVisual(Job(i), EInventorySetting::COLLECT));
}

void iwBaseWarehouse::UpdateOverlays()
{
    // Ein/Auslager Overlays entsprechend setzen
    for(unsigned char category = 0; category < 2; ++category)
    {
        unsigned count = (category == 0) ? NUM_WARE_TYPES : NUM_JOB_TYPES;
        for(unsigned i = 0; i < count; ++i)
        {
            UpdateOverlay(i, category == 0);
        }
    }
}

void iwBaseWarehouse::OnChange(unsigned changeId)
{
    if(changeId == 0)
    {
        wh = nullptr;
        Close();
    } else if(changeId == 1)
        UpdateOverlays();
}
