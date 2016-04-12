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
#include "iwBaseWarehouse.h"

#include "Loader.h"

#include "GameClient.h"
#include "iwDemolishBuilding.h"
#include "WindowManager.h"
#include "iwHelp.h"
#include "iwHQ.h"
#include "iwStorehouse.h"
#include "iwHarborBuilding.h"
#include "buildings/nobBaseWarehouse.h"
#include "buildings/nobHarborBuilding.h"
#include "buildings/nobStorehouse.h"
#include "world/GameWorldView.h"
#include "controls/ctrlButton.h"
#include "controls/ctrlGroup.h"
#include "controls/ctrlImage.h"
#include "controls/ctrlOptionGroup.h"

#include <stdexcept>

// Include last!
#include "DebugNew.h" // IWYU pragma: keep

iwBaseWarehouse::iwBaseWarehouse(GameWorldView& gwv, const std::string& title,
                                 unsigned char page_count,
                                 nobBaseWarehouse* wh) : iwWares(wh->CreateGUIID(), 0xFFFE, 0xFFFE, 167, 416, title, page_count, true, NormalFont, wh->GetInventory()), gwv(gwv),  wh(wh)
{
    wh->AddListener(this);

    // Basisinitialisierungsänderungen
    background = LOADER.GetImageN("resource", 41);

    // Auswahl für Auslagern/Einlagern Verbieten-Knöpfe
    ctrlOptionGroup* group = AddOptionGroup(10, ctrlOptionGroup::CHECK);
    // Einlagern
    group->AddImageButton(0, 16, 335, 32, 32, TC_GREY, LOADER.GetImageN("io_new", 4), _("Collect"));
    // Auslagern
    group->AddImageButton(1, 52, 335, 32, 32, TC_GREY, LOADER.GetImageN("io", 211), _("Take out of store"));
    // Einlagern verbieten
    group->AddImageButton(2, 86, 335, 32, 32, TC_GREY, LOADER.GetImageN("io", 212), _("Stop storage"));
    // nix tun auswählen
    group->SetSelection(0);

    // Alle auswählen bzw setzen!
    AddImageButton(11, 122, 335, 32, 32, TC_GREY, LOADER.GetImageN("io", 223), _("Select all"));
    // "Gehe Zu Ort"
    AddImageButton(13, 122, 369, 15, 32, TC_GREY, LOADER.GetImageN("io_new", 10), _("Go to place"));
	// Go to next warehouse
	AddImageButton(14, 139, 369, 15, 32, TC_GREY, LOADER.GetImageN("io_new", 13), _("Go to next warehouse"));

    UpdateOverlays();

    // Lagerhaus oder Hafengebäude?
    if(wh->GetGOT() == GOT_NOB_STOREHOUSE || wh->GetGOT() == GOT_NOB_HARBORBUILDING)
    {
        // Abbrennbutton hinzufügen
        // "Blättern" in Bretter stauchen und verschieben
        GetCtrl<ctrlButton>(0)->SetWidth(32);
        GetCtrl<ctrlButton>(0)->Move(86, 369, true);

        AddImageButton(1, 52, 369, 32, 32, TC_GREY, LOADER.GetImageN("io",  23), _("Demolish house"));
    }
}

iwBaseWarehouse::~iwBaseWarehouse()
{
    if(wh)
        wh->RemoveListener(this);
}

void iwBaseWarehouse::Msg_Group_ButtonClick(const unsigned int group_id, const unsigned int ctrl_id)
{
    switch(group_id)
    {
        default: // an Basis weiterleiten
        {
            iwWares::Msg_Group_ButtonClick(group_id, ctrl_id);
        } break;
        case 100: // Waren
        case 101: // Figuren
        {
            if(GAMECLIENT.IsReplayModeOn())
                return;
            RTTR_Assert(page < 2);
            ctrlOptionGroup* optiongroup = GetCtrl<ctrlOptionGroup>(10);

            EInventorySetting setting;
            switch(optiongroup->GetSelection())
            {
            case 0: setting = EInventorySetting::COLLECT; break;
            case 1: setting = EInventorySetting::SEND; break;
            case 2: setting = EInventorySetting::STOP; break;
            default:
                throw std::invalid_argument("iwBaseWarehouse::Optiongroup");
            }
            InventorySetting state = page == 0 ? wh->GetInventorySettingVisual(GoodType(ctrl_id - 100)) : wh->GetInventorySettingVisual(Job(ctrl_id - 100));
            state.Toggle(setting);
            if(GAMECLIENT.SetInventorySetting(wh->GetPos(), page != 0, ctrl_id - 100, state))
            {
                // optisch schonmal setzen
                wh->SetInventorySettingVisual(page != 0, ctrl_id - 100, state);
                UpdateOverlay(ctrl_id - 100);
            }
        } break;
    }
}

void iwBaseWarehouse::Msg_ButtonClick(const unsigned int ctrl_id)
{
    switch(ctrl_id)
    {
        case 1: // Gebäude abreißen
        {
            // Abreißen?
            Close();
            WINDOWMANAGER.Show(new iwDemolishBuilding(gwv, wh));
        } break;
        case 11: // "Alle auswählen"
        {
            if(GAMECLIENT.IsReplayModeOn())
                return;
            if(page >= 2)
                return;
            ctrlOptionGroup* optiongroup = GetCtrl<ctrlOptionGroup>(10);
            EInventorySetting data;
            switch(optiongroup->GetSelection())
            {
            case 0: data = EInventorySetting::COLLECT; break;
            case 1: data = EInventorySetting::SEND; break;
            case 2: data = EInventorySetting::STOP; break;
            default:
                throw std::invalid_argument("iwBaseWarehouse::Optiongroup");
            }
            const unsigned count = (page == 0) ? WARE_TYPES_COUNT : JOB_TYPES_COUNT;
            std::vector<InventorySetting> states;
            states.reserve(count);
            if(page == 0)
            {
                for(unsigned i = 0; i < WARE_TYPES_COUNT; i++)
                    states.push_back(wh->GetInventorySettingVisual(i == GD_WATEREMPTY ? GD_WATER : GoodType(i)));
            } else
            {
                for(unsigned i = 0; i < JOB_TYPES_COUNT; i++)
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

            if(GAMECLIENT.SetAllInventorySettings(wh->GetPos(), page != 0, states))
            {
                // optisch setzen
                for(unsigned char i = 0; i < count; ++i)
                {
                    // Status ändern
                    wh->SetInventorySettingVisual(page != 0, i, states[i]);
                }
                UpdateOverlays();
            }
        } break;
        case 12: // "Hilfe"
        {
            WINDOWMANAGER.Show(new iwHelp(GUI_ID(CGI_HELPBUILDING + wh->GetBuildingType()), _(BUILDING_NAMES[wh->GetBuildingType()]), _(BUILDING_HELP_STRINGS[wh->GetBuildingType()])));
        } break;
        case 13: // "Gehe Zu Ort"
        {
            gwv.MoveToMapPt(wh->GetPos());
        } break;
		case 14: //go to next of same type
		{
			//is there at least 1 other building of the same type?
            const std::list<nobBaseWarehouse*>& storehouses = GAMECLIENT.GetPlayer(wh->GetPlayer()).GetStorehouses();
			//go through list once we get to current building -> open window for the next one and go to next location
			for(std::list<nobBaseWarehouse*>::const_iterator it=storehouses.begin(); it != storehouses.end(); ++it)
			{
				if((*it)->GetPos()==wh->GetPos()) //got to current building in the list?
				{
					//close old window, open new window (todo: only open if it isnt already open), move to location of next building
					Close();
					++it;
					if(it == storehouses.end()) //was last entry in list -> goto first												{
						it=storehouses.begin();
					gwv.MoveToMapPt((*it)->GetPos());
					if((*it)->GetBuildingType()==BLD_HEADQUARTERS)
					{
						iwHQ* nextscrn=new iwHQ(gwv, (*it),_("Headquarters"), 3);
						nextscrn->Move(x_,y_);
						WINDOWMANAGER.Show(nextscrn);
					}
					else if((*it)->GetBuildingType()==BLD_HARBORBUILDING)
					{
						iwHarborBuilding* nextscrn = new iwHarborBuilding(gwv, dynamic_cast<nobHarborBuilding*>(*it));
						nextscrn->Move(x_,y_);
						WINDOWMANAGER.Show(nextscrn);
					}
					else if((*it)->GetBuildingType()==BLD_STOREHOUSE) 
					{
						iwStorehouse* nextscrn=new iwStorehouse(gwv, dynamic_cast<nobStorehouse*>(*it));
						nextscrn->Move(x_,y_);
						WINDOWMANAGER.Show(nextscrn);
					}
					break;
				}
			}
		} break;
        default: // an Basis weiterleiten
        {
            iwWares::Msg_ButtonClick(ctrl_id);
        } break;
    }
}

void iwBaseWarehouse::UpdateOverlay(unsigned i)
{
    UpdateOverlay(i, this->page == 0);
}

void iwBaseWarehouse::UpdateOverlay(unsigned i, bool isWare)
{
    ctrlGroup* group = GetCtrl<ctrlGroup>(isWare ? 100 : 101);
    // Einlagern verbieten-Bild (de)aktivieren
    ctrlImage* image = group->GetCtrl<ctrlImage>(400 + i);
    if(image)
        image->SetVisible(isWare ? wh->IsInventorySettingVisual(GoodType(i), EInventorySetting::STOP) : wh->IsInventorySettingVisual(Job(i), EInventorySetting::STOP));

    // Auslagern-Bild (de)aktivieren
    image = group->GetCtrl<ctrlImage>(500 + i);
    if(image)
        image->SetVisible(isWare ? wh->IsInventorySettingVisual(GoodType(i), EInventorySetting::SEND) : wh->IsInventorySettingVisual(Job(i), EInventorySetting::SEND));

    // Einlagern-Bild (de)aktivieren
    image = group->GetCtrl<ctrlImage>(700 + i);
    if(image)
        image->SetVisible(isWare ? wh->IsInventorySettingVisual(GoodType(i), EInventorySetting::COLLECT) : wh->IsInventorySettingVisual(Job(i), EInventorySetting::COLLECT));

}

void iwBaseWarehouse::UpdateOverlays()
{
    // Ein/Auslager Overlays entsprechend setzen
    for(unsigned char category = 0; category < 2; ++category)
    {
        unsigned count = (category == 0) ? WARE_TYPES_COUNT : JOB_TYPES_COUNT;
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
        wh = NULL;
        Close();
    }else if(changeId == 1)
        UpdateOverlays();
}
