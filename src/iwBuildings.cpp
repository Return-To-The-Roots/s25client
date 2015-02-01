// $Id: iwBuildings.cpp 9595 2015-02-01 09:40:54Z marcus $
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
#include "iwBuildings.h"
#include "Loader.h"
#include "GameConsts.h"
#include "GameClient.h"
#include "WindowManager.h"
#include "nobMilitary.h"
#include "iwMilitaryBuilding.h"
#include "nobUsual.h"
#include "iwBuilding.h"
#include "nobBaseWarehouse.h"
#include "nobStorehouse.h"
#include "nobHarborBuilding.h"
#include "iwStorehouse.h"
#include "iwHarborBuilding.h"


const unsigned BUILDINGS_COUNT = 32;

/// Reihenfolge der Gebäude
const BuildingType bts[BUILDINGS_COUNT] =
{
    BLD_BARRACKS,
    BLD_GUARDHOUSE,
    BLD_WATCHTOWER,
    BLD_FORTRESS,
    BLD_GRANITEMINE,
    BLD_COALMINE,
    BLD_IRONMINE,
    BLD_GOLDMINE,
    BLD_LOOKOUTTOWER,
    BLD_CATAPULT,
    BLD_WOODCUTTER,
    BLD_FISHERY,
    BLD_QUARRY,
    BLD_FORESTER,
    BLD_SLAUGHTERHOUSE,
    BLD_HUNTER,
    BLD_BREWERY,
    BLD_ARMORY,
    BLD_METALWORKS,
    BLD_IRONSMELTER,
    BLD_PIGFARM,
    BLD_STOREHOUSE, // entry 21
    BLD_MILL,
    BLD_BAKERY,
    BLD_SAWMILL,
    BLD_MINT,
    BLD_WELL,
    BLD_SHIPYARD,
    BLD_FARM,
    BLD_DONKEYBREEDER,
    BLD_CHARBURNER,
    BLD_HARBORBUILDING // entry 31
};


// Abstand des ersten Icons vom linken oberen Fensterrand
const unsigned short first_x = 30;
const unsigned short first_y = 40;
// Abstand der einzelnen Symbole untereinander
const unsigned short icon_distance_x = 40;
const unsigned short icon_distance_y = 48;
// Abstand der Schriften unter den Icons
const unsigned short font_distance_y = 20;


/// Konstruktor von @p iwMilitary.
iwBuildings::iwBuildings(GameWorldViewer* const gwv, dskGameInterface* const gi) : IngameWindow(CGI_BUILDINGS, 0xFFFE, 0xFFFE, 185, 480, _("Buildings"), LOADER.GetImageN("resource", 41)),gwv(gwv),gi(gi)
{
    // Symbole für die einzelnen Gebäude erstellen
    for(unsigned short y = 0; y < BUILDINGS_COUNT / 4 + (BUILDINGS_COUNT % 4 > 0 ? 1 : 0); ++y)
    {
        for(unsigned short x = 0; x < ((y == BUILDINGS_COUNT / 4) ? BUILDINGS_COUNT % 4 : 4); ++x)
        {
			if(bts[y*4+x] != BLD_CHARBURNER)
				AddImageButton(y * 4 + x, first_x - 16 + icon_distance_x * x, first_y - 16 + icon_distance_y * y,32,32,TC_GREY,LOADER.GetImageN(NATION_ICON_IDS[GameClient::inst().GetLocalPlayer()->nation], bts[y * 4 + x]), _(BUILDING_NAMES[bts[y * 4 + x]]));
			else
				AddImageButton(y * 4 + x, first_x - 16 + icon_distance_x * x, first_y - 16  + icon_distance_y * y,32,32,TC_GREY,LOADER.GetImageN("charburner", GameClient::inst().GetLocalPlayer()->nation * 8 + 8) , _(BUILDING_NAMES[bts[y * 4 + x]]));
        }
    }

    // Hilfe-Button
    AddImageButton(32, width - 14 - 30, height - 20 - 32, 30, 32, TC_GREY, LOADER.GetImageN("io", 21), _("Help"));

}

/// Anzahlen der Gebäude zeichnen
void iwBuildings::Msg_PaintAfter()
{
    // Anzahlen herausfinden
    BuildingCount bc;

    GameClient::inst().GetLocalPlayer()->GetBuildingCount(bc);

    // Anzahlen unter die Gebäude schreiben
    for(unsigned short y = 0; y < BUILDINGS_COUNT / 4 + (BUILDINGS_COUNT % 4 > 0 ? 1 : 0); ++y)
    {
        for(unsigned short x = 0; x < ((y == BUILDINGS_COUNT / 4) ? BUILDINGS_COUNT % 4 : 4); ++x)
        {
            char txt[64];
            sprintf(txt, "%u/%u", bc.building_counts[bts[y * 4 + x]], bc.building_site_counts[bts[y * 4 + x]]);
            NormalFont->Draw(GetX() + first_x + icon_distance_x * x, GetY() + first_y + icon_distance_y * y + font_distance_y, txt,
                             glArchivItem_Font::DF_CENTER, COLOR_YELLOW);

        }
    }
}

void iwBuildings::Msg_ButtonClick(const unsigned int ctrl_id)
{	
	//no buildings of type complete? -> do nothing
	BuildingCount bc;
	GameClient::inst().GetLocalPlayer()->GetBuildingCount(bc);	
	if(bc.building_counts[bts[ctrl_id]] < 1)
		return;

	//military building open first of type if available
	if(ctrl_id < 4)
	{
		for(std::list<nobMilitary*>::const_iterator it=GameClient::inst().GetLocalPlayer()->GetMilitaryBuildings().begin(); it != GameClient::inst().GetLocalPlayer()->GetMilitaryBuildings().end(); it++)
		{
			if((*it)->GetBuildingType()==bts[ctrl_id]) // got first of type -> open building window (military)
			{
				gwv->MoveToMapObject((*it)->GetX(),(*it)->GetY());
				iwMilitaryBuilding* nextscrn=new iwMilitaryBuilding(gwv, gi, (*it));
				WindowManager::inst().Show(nextscrn);
				return;
			}
		}
		return;
	}
	//not warehouse, harbor (military excluded) -> so it is a nobusual!
	if(ctrl_id != 21 && ctrl_id != 31)
	{
		nobUsual* it=*GameClient::inst().GetLocalPlayer()->GetBuildings(bts[ctrl_id]).begin();
		gwv->MoveToMapObject(it->GetX(),it->GetY());
		iwBuilding* nextscrn=new iwBuilding(gwv, gi, (it));
		WindowManager::inst().Show(nextscrn);
		return;
	}
	else if(ctrl_id == 21)//warehouse?
	{
		//go through list until we get to a warehouse
		for(std::list<nobBaseWarehouse*>::const_iterator it=GameClient::inst().GetLocalPlayer()->GetStorehouses().begin(); it != GameClient::inst().GetLocalPlayer()->GetStorehouses().end(); it++)
		{
			if((*it)->GetBuildingType()==bts[ctrl_id])
			{
				gwv->MoveToMapObject((*it)->GetX(),(*it)->GetY());
				iwStorehouse* nextscrn=new iwStorehouse(gwv,gi,dynamic_cast<nobStorehouse*>((*it)));
				nextscrn->Move(x,y);
				WindowManager::inst().Show(nextscrn);
				return;
			}
		}
	}
	else if(ctrl_id==31)//harbor
	{
		//go through list until we get to a harbor
		for(std::list<nobBaseWarehouse*>::const_iterator it=GameClient::inst().GetLocalPlayer()->GetStorehouses().begin(); it != GameClient::inst().GetLocalPlayer()->GetStorehouses().end(); it++)
		{
			if((*it)->GetBuildingType()==bts[ctrl_id])
			{
				gwv->MoveToMapObject((*it)->GetX(),(*it)->GetY());
				iwHarborBuilding* nextscrn = new iwHarborBuilding(gwv,gi,dynamic_cast<nobHarborBuilding*>((*it)));
				nextscrn->Move(x,y);
				WindowManager::inst().Show(nextscrn);
				return;
			}
		}
	}

	
}
