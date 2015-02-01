// $Id: iwBaseWarehouse.cpp 9592 2015-02-01 09:39:38Z marcus $
//
// Copyright (c) 2005 - 2011 Settlers Freaks (sf-team at siedler25.org)
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
#include "main.h"
#include "iwBaseWarehouse.h"

#include "Loader.h"

#include "VideoDriverWrapper.h"
#include "controls.h"
#include "GameClient.h"
#include "iwDemolishBuilding.h"
#include "WindowManager.h"
#include "iwHelp.h"
#include "GameCommands.h"
#include "iwHQ.h"
#include "iwStorehouse.h"
#include "iwHarborBuilding.h"

#include "nobBaseWarehouse.h"
#include "nobHarborBuilding.h"
#include "nobStorehouse.h"

///////////////////////////////////////////////////////////////////////////////
// Makros / Defines
#if defined _WIN32 && defined _DEBUG && defined _MSC_VER
#define new new(_NORMAL_BLOCK, THIS_FILE, __LINE__)
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

///////////////////////////////////////////////////////////////////////////////
/**
 *  Konstruktor von @p iwHQ.
 *
 *  @author OLiver
 */
iwBaseWarehouse::iwBaseWarehouse(GameWorldViewer* const gwv, dskGameInterface* const gi, const char* const title,
                                 unsigned char page_count,
                                 nobBaseWarehouse* wh)
    : iwWares(wh->CreateGUIID(), 0xFFFE, 0xFFFE, 167, 416, title, page_count, true, NormalFont, wh->GetInventory()), gwv(gwv), gi(gi), wh(wh)
{
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
    AddImageButton(13, 122, 369, 15, 32, TC_GREY, LOADER.GetImageN("io", 107), _("Go to place"));
	// Go to next warehouse
	AddImageButton(14, 139, 369, 15, 32, TC_GREY, LOADER.GetImageN("io", 107), _("Go to next warehouse"));

    // Ein/Auslager Overlays entsprechend setzen
    // bei Replays die reellen Einstellungen nehmen, weils die visuellen da logischweise nich gibt!
    ctrlImage* image;
    for(unsigned char category = 0; category < 2; ++category)
    {
        unsigned count = (category == 0) ? 35 : 32;
        for(unsigned i = 0; i < count; ++i)
        {
            // Einlagern verbieten-Bild (de)aktivieren
            image = GetCtrl<ctrlGroup>(100 + category)->GetCtrl<ctrlImage>(400 + i);
            if(image)
                image->SetVisible(GAMECLIENT.IsReplayModeOn() ? wh->CheckRealInventorySettings(category, 2, i) :
                                  wh->CheckVisualInventorySettings(category, 2, i));

            // Auslagern-Bild (de)aktivieren
            image = GetCtrl<ctrlGroup>(100 + category)->GetCtrl<ctrlImage>(500 + i);
            if(image)
                image->SetVisible(GAMECLIENT.IsReplayModeOn() ? wh->CheckRealInventorySettings(category, 4, i) :
                                  wh->CheckVisualInventorySettings(category, 4, i));

            // Einlagern-Bild (de)aktivieren
            image = GetCtrl<ctrlGroup>(100 + category)->GetCtrl<ctrlImage>(700 + i);
            if(image)
                image->SetVisible(GAMECLIENT.IsReplayModeOn() ? wh->CheckRealInventorySettings(category, 8, i) :
                                  wh->CheckVisualInventorySettings(category, 8, i));
        }
    }

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
            ctrlOptionGroup* optiongroup = GetCtrl<ctrlOptionGroup>(10);

            unsigned int data = 0;
            switch(optiongroup->GetSelection())
            {
                case 0: data = 8; break;
                case 1: data = 4; break;
                case 2: data = 2; break;
            }
            if(data != 0)
            {
                // Nicht bei Replays setzen
                if(GAMECLIENT.AddGC(new gc::ChangeInventorySetting(wh->GetX(), wh->GetY(), page, data, ctrl_id - 100)))
                    // optisch schonmal setzen
                    ChangeOverlay(ctrl_id - 100, data);;
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
            WindowManager::inst().Show(new iwDemolishBuilding(gwv, wh));
        } break;
        case 11: // "Alle auswählen"
        {
            if(this->page < 2)
            {
                ctrlOptionGroup* optiongroup = GetCtrl<ctrlOptionGroup>(10);
                unsigned int data = 0;
                switch(optiongroup->GetSelection())
                {
                    case 0: data = 8; break;
                    case 1: data = 4; break;
                    case 2: data = 2; break;
                }
                if(data != 0)
                {
                    // Nicht bei Replays setzen
                    if(GAMECLIENT.AddGC(new gc::ChangeAllInventorySettings(wh->GetX(), wh->GetY(), page, data)))
                    {
                        // optisch setzen
                        unsigned short count = ((page == 0) ? WARE_TYPES_COUNT : JOB_TYPES_COUNT);
                        for(unsigned char i = 0; i < count; ++i)
                            ChangeOverlay(i, data);
                    }
                }

            }
        } break;
        case 12: // "Hilfe"
        {
            WindowManager::inst().Show(new iwHelp(GUI_ID(CGI_HELPBUILDING + wh->GetBuildingType()), _(BUILDING_NAMES[wh->GetBuildingType()]),
                                                  _(BUILDING_HELP_STRINGS[wh->GetBuildingType()])));
        } break;
        case 13: // "Gehe Zu Ort"
        {
            gwv->MoveToMapObject(wh->GetX(), wh->GetY());
        } break;
		case 14: //go to next of same type
		{
			//is there at least 1 other building of the same type?
			if(GameClient::inst().GetPlayer(wh->GetPlayer())->GetStorehouses().size()>1)
			{
				//go through list once we get to current building -> open window for the next one and go to next location
				for(std::list<nobBaseWarehouse*>::const_iterator it=GameClient::inst().GetPlayer(wh->GetPlayer())->GetStorehouses().begin(); it != GameClient::inst().GetPlayer(wh->GetPlayer())->GetStorehouses().end(); it++)
				{
					if((*it)->GetX()==wh->GetX() && (*it)->GetY()==wh->GetY()) //got to current building in the list?
					{
						//close old window, open new window (todo: only open if it isnt already open), move to location of next building
						Close();
						it++;
						if(it == GameClient::inst().GetPlayer(wh->GetPlayer())->GetStorehouses().end()) //was last entry in list -> goto first												{
							it=GameClient::inst().GetPlayer(wh->GetPlayer())->GetStorehouses().begin();
						gwv->MoveToMapObject((*it)->GetX(),(*it)->GetY());
						if((*it)->GetBuildingType()==BLD_HEADQUARTERS)
						{
							iwHQ* nextscrn=new iwHQ(gwv, gi, (*it),_("Headquarters"), 3);
							nextscrn->Move(x,y);
							WindowManager::inst().Show(nextscrn);
						}
						else if((*it)->GetBuildingType()==BLD_HARBORBUILDING)
						{
							iwHarborBuilding* nextscrn = new iwHarborBuilding(gwv,gi,dynamic_cast<nobHarborBuilding*>((*it)));
							nextscrn->Move(x,y);
							WindowManager::inst().Show(nextscrn);
						}
						else if((*it)->GetBuildingType()==BLD_STOREHOUSE) 
						{
							iwStorehouse* nextscrn=new iwStorehouse(gwv,gi,dynamic_cast<nobStorehouse*>((*it)));
							nextscrn->Move(x,y);
							WindowManager::inst().Show(nextscrn);
						}
						break;
					}
				}
			}
		} break;
        default: // an Basis weiterleiten
        {
            iwWares::Msg_ButtonClick(ctrl_id);
        } break;
    }
}


///////////////////////////////////////////////////////////////////////////////
/**
 *  Overlay ändern.
 *
 *  @param[in] i    Item-Nr (0-29)
 *  @param[in] what Welcher Status soll geändert werden (2 = Einlagern verbieten, 4 = Auslagern)
 *
 *  @author FloSoft
 */
void iwBaseWarehouse::ChangeOverlay(unsigned int i, unsigned int what)
{
    ctrlImage* image;

    // Status ändern
    wh->ChangeVisualInventorySettings(page, what, i);

    // Einlagern verbieten-Bild (de)aktivieren
    image = GetCtrl<ctrlGroup>(100 + this->page)->GetCtrl<ctrlImage>(400 + i);
    if(image)
        image->SetVisible(wh->CheckVisualInventorySettings(page, 2, i));

    // Auslagern-Bild (de)aktivieren
    image = GetCtrl<ctrlGroup>(100 + this->page)->GetCtrl<ctrlImage>(500 + i);
    if(image)
        image->SetVisible(wh->CheckVisualInventorySettings(page, 4, i));

    // Einlagern-Bild (de)aktivieren
    image = GetCtrl<ctrlGroup>(100 + this->page)->GetCtrl<ctrlImage>(700 + i);
    if(image)
        image->SetVisible(wh->CheckVisualInventorySettings(page, 8, i));
}
