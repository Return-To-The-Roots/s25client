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

#include "defines.h" // IWYU pragma: keep
#include "iwBuildings.h"
#include "Loader.h"
#include "GamePlayer.h"
#include "WindowManager.h"
#include "buildings/nobUsual.h"
#include "buildings/nobMilitary.h"
#include "buildings/nobBaseWarehouse.h"
#include "buildings/nobStorehouse.h"
#include "buildings/nobHarborBuilding.h"
#include "iwMilitaryBuilding.h"
#include "iwBuilding.h"
#include "iwStorehouse.h"
#include "iwHarborBuilding.h"
#include "iwHelp.h"
#include "world/GameWorldView.h"
#include "world/GameWorldViewer.h"
#include "ogl/glArchivItem_Font.h"
#include "gameTypes/BuildingCount.h"
#include "gameData/const_gui_ids.h"
#include "files.h"
#include <cstdio>

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
const DrawPoint iconPadding(30, 40);
// Abstand der einzelnen Symbole untereinander
const DrawPoint iconSpacing(40, 48);
// Abstand der Schriften unter den Icons
const unsigned short font_distance_y = 20;


iwBuildings::iwBuildings(GameWorldView& gwv, GameCommandFactory& gcFactory):
    IngameWindow(CGI_BUILDINGS, 0xFFFE, 0xFFFE, 185, 480, _("Buildings"), LOADER.GetImageN("resource", 41)),
    gwv(gwv), gcFactory(gcFactory)
{
    const Nation playerNation = gwv.GetViewer().GetPlayer().nation;
    // Symbole für die einzelnen Gebäude erstellen
    for(unsigned short y = 0; y < BUILDINGS_COUNT / 4 + (BUILDINGS_COUNT % 4 > 0 ? 1 : 0); ++y)
    {
        for(unsigned short x = 0; x < ((y == BUILDINGS_COUNT / 4) ? BUILDINGS_COUNT % 4 : 4); ++x)
        {
			if(bts[y*4+x] != BLD_CHARBURNER)
            {
                AddImageButton(y * 4 + x, iconPadding.x - 16 + iconSpacing.x * x, iconPadding.y - 16 + iconSpacing.y * y, 32, 32, TC_GREY, LOADER.GetImageN(NATION_ICON_IDS[playerNation], bts[y * 4 + x]), _(BUILDING_NAMES[bts[y * 4 + x]]));
            }
			else
				AddImageButton(y * 4 + x, iconPadding.x - 16 + iconSpacing.x * x, iconPadding.y - 16  + iconSpacing.y * y,32,32,TC_GREY,LOADER.GetImageN("charburner", playerNation * 8 + 8) , _(BUILDING_NAMES[bts[y * 4 + x]]));
        }
    }

    // Hilfe-Button
    AddImageButton(32, width_ - 14 - 30, height_ - 20 - 32, 30, 32, TC_GREY, LOADER.GetImageN("io", 21), _("Help"));

}

/// Anzahlen der Gebäude zeichnen
void iwBuildings::Msg_PaintAfter()
{
    // Anzahlen herausfinden
    BuildingCount bc = gwv.GetViewer().GetPlayer().GetBuildingCount();

    // Anzahlen unter die Gebäude schreiben
    DrawPoint rowPos = GetDrawPos() + iconPadding + DrawPoint(0, font_distance_y);
    for(unsigned short y = 0; y < BUILDINGS_COUNT / 4 + (BUILDINGS_COUNT % 4 > 0 ? 1 : 0); ++y)
    {
        DrawPoint curPos = rowPos;
        for(unsigned short x = 0; x < ((y == BUILDINGS_COUNT / 4) ? BUILDINGS_COUNT % 4 : 4); ++x)
        {
            char txt[64];
            sprintf(txt, "%u/%u", bc.buildings[bts[y * 4 + x]], bc.buildingSites[bts[y * 4 + x]]);
            NormalFont->Draw(curPos, txt, glArchivItem_Font::DF_CENTER, COLOR_YELLOW);
            curPos.x += iconSpacing.x;
        }
        rowPos.y += iconSpacing.y;
    }
}

void iwBuildings::Msg_ButtonClick(const unsigned int ctrl_id)
{	
    if (ctrl_id == 32) // Help button
        return; // TODO should show help text

	//no buildings of type complete? -> do nothing
    const GamePlayer& localPlayer = gwv.GetViewer().GetPlayer();
	BuildingCount bc = localPlayer.GetBuildingCount();//-V807
	if(bc.buildings[bts[ctrl_id]] < 1)
		return;

	//military building open first of type if available
	if(ctrl_id < 4)
	{
		for(std::list<nobMilitary*>::const_iterator it=localPlayer.GetMilitaryBuildings().begin(); it != localPlayer.GetMilitaryBuildings().end(); ++it)
		{
			if((*it)->GetBuildingType()==bts[ctrl_id]) // got first of type -> open building window (military)
			{
				gwv.MoveToMapPt((*it)->GetPos());
				iwMilitaryBuilding* nextscrn=new iwMilitaryBuilding(gwv, gcFactory, *it);
				WINDOWMANAGER.Show(nextscrn);
				return;
			}
		}
		return;
	}
	//not warehouse, harbor (military excluded) -> so it is a nobusual!
	if(ctrl_id != 21 && ctrl_id != 31)
	{
		nobUsual* it=*localPlayer.GetBuildings(bts[ctrl_id]).begin();
		gwv.MoveToMapPt(it->GetPos());
		iwBuilding* nextscrn=new iwBuilding(gwv, gcFactory, it);
		WINDOWMANAGER.Show(nextscrn);
		return;
	}
	else if(ctrl_id == 21)//warehouse?
	{
		//go through list until we get to a warehouse
		for(std::list<nobBaseWarehouse*>::const_iterator it=localPlayer.GetStorehouses().begin(); it != localPlayer.GetStorehouses().end(); ++it)
		{
			if((*it)->GetBuildingType()==bts[ctrl_id])
			{
				gwv.MoveToMapPt((*it)->GetPos());
				iwStorehouse* nextscrn=new iwStorehouse(gwv, gcFactory, dynamic_cast<nobStorehouse*>(*it));
				nextscrn->Move(x_,y_);
				WINDOWMANAGER.Show(nextscrn);
				return;
			}
		}
	}
	else if(ctrl_id==31)//harbor
	{
		//go through list until we get to a harbor
		for(std::list<nobBaseWarehouse*>::const_iterator it=localPlayer.GetStorehouses().begin(); it != localPlayer.GetStorehouses().end(); ++it)
		{
			if((*it)->GetBuildingType()==bts[ctrl_id])
			{
				gwv.MoveToMapPt((*it)->GetPos());
				iwHarborBuilding* nextscrn = new iwHarborBuilding(gwv, gcFactory, dynamic_cast<nobHarborBuilding*>(*it));
				nextscrn->Move(x_,y_);
				WINDOWMANAGER.Show(nextscrn);
				return;
			}
		}
	}

	
}
