//
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
#include "iwMilitaryBuilding.h"

#include "Loader.h"
#include "GameClient.h"
#include "gameData/MilitaryConsts.h"
#include "WindowManager.h"
#include "iwDemolishBuilding.h"
#include "iwMsgbox.h"
#include "iwHelp.h"
#include "buildings/nobMilitary.h"
#include "world/GameWorldView.h"
#include "figures/nofPassiveSoldier.h"
#include "ogl/glArchivItem_Bitmap.h"
#include "ogl/glArchivItem_Font.h"
#include "controls/ctrlButton.h"
#include <set>

// Include last!
#include "DebugNew.h" // IWYU pragma: keep

iwMilitaryBuilding::iwMilitaryBuilding(GameWorldView& gwv, nobMilitary* const building)
    : IngameWindow(building->CreateGUIID(), (unsigned short) - 2, (unsigned short) - 2, 226, 194, _(BUILDING_NAMES[building->GetBuildingType()]), LOADER.GetImageN("resource", 41)),
    gwv(gwv), building(building)
{
    // Schwert
    AddImage(0, 28, 39, LOADER.GetMapImageN(2298));
    AddImage(1, 28, 39, LOADER.GetMapImageN(2250 + GD_SWORD));

    // Schild
    AddImage(2, 196, 39, LOADER.GetMapImageN(2298));
    AddImage(3, 196, 39, LOADER.GetMapImageN(2250 + GD_SHIELDROMANS));

    // Hilfe
    AddImageButton(4,  16, 147, 30, 32, TC_GREY, LOADER.GetImageN("io",  21));
    // Abreißen
    AddImageButton(5,  50, 147, 34, 32, TC_GREY, LOADER.GetImageN("io",  23));
    // Gold an/aus (227,226)
    AddImageButton(6,  90, 147, 32, 32, TC_GREY, LOADER.GetImageN("io", ((building->IsGoldDisabledVirtual()) ? 226 : 227)));
    // "Gehe Zu Ort"
    AddImageButton(7, 179, 147, 30, 32, TC_GREY, LOADER.GetImageN("io", 107), _("Go to place"));

    // Gebäudebild
    AddImage(8, 117, 114, LOADER.GetNationImage(building->GetNation(), 250 + 5 * building->GetBuildingType()));
	// "Go to next" (building of same type)
    AddImageButton( 9, 179, 115, 30, 32, TC_GREY, LOADER.GetImageN("io_new", 11), _("Go to next military building"));
	//addon military control active? -> show button
	if(GAMECLIENT.GetGGS().isEnabled(AddonId::MILITARY_CONTROL))
		AddImageButton( 10, 124, 147, 30, 32, TC_GREY, LOADER.GetImageN("io_new", 12), _("Send max rank soldiers to a warehouse"));
}

void iwMilitaryBuilding::Msg_PaintAfter()
{
    // Schatten des Gebäudes (muss hier gezeichnet werden wegen schwarz und halbdurchsichtig)
    LOADER.GetNationImage(building->GetNation(), 250 + 5 * building->GetBuildingType() + 1)->Draw(GetX() + 117, GetY() + 114, 0, 0, 0, 0, 0, 0, COLOR_SHADOW);

    // Schwarzer Untergrund für Goldanzeige
    DrawRectangle(GetX() + width_ / 2 - 22 * GOLD_COUNT[building->nation][building->size] / 2, GetY() + 60, 22 * GOLD_COUNT[building->nation][building->size], 24, 0x96000000);
    // Gold
    for(unsigned short i = 0; i < GOLD_COUNT[building->nation][building->size]; ++i)
        LOADER.GetMapImageN(2278)->Draw(GetX() + width_ / 2 - 22 * GOLD_COUNT[building->nation][building->size] / 2 + 12 + i * 22, GetY() + 72, 0, 0, 0, 0, 0, 0, ( i >= building->coins ? 0xFFA0A0A0 : 0xFFFFFFFF) );

    // Schwarzer Untergrund für Soldatenanzeige
    DrawRectangle(GetX() + width_ / 2 - 22 * TROOPS_COUNT[building->nation][building->size] / 2, GetY() + 98 , 22 * TROOPS_COUNT[building->nation][building->size], 24, 0x96000000);

    // Sammeln aus der Rausgeh-Liste und denen, die wirklich noch drinne sind
    std::multiset<const nofSoldier*, ComparatorSoldiersByRank<true> > soldiers;
    for(SortedTroops::iterator it = building->troops.begin(); it != building->troops.end(); ++it)
        soldiers.insert(*it);

    for(std::list<noFigure*>::iterator it = building->leave_house.begin(); it != building->leave_house.end(); ++it)
    {
        if((*it)->GetGOT() == GOT_NOF_ATTACKER ||
                (*it)->GetGOT() == GOT_NOF_AGGRESSIVEDEFENDER ||
                (*it)->GetGOT() == GOT_NOF_DEFENDER ||
                (*it)->GetGOT() == GOT_NOF_PASSIVESOLDIER)
        {
            soldiers.insert(static_cast<nofSoldier*>(*it));
        }
    }

    // Soldaten zeichnen
    unsigned short i = 0;
    for(std::multiset<const nofSoldier*, ComparatorSoldiersByRank<true> >::const_iterator it = soldiers.begin(); it != soldiers.end(); ++it, ++i)
        LOADER.GetMapImageN(2321 + (*it)->GetRank())->Draw(GetX() + width_ / 2 - 22 * TROOPS_COUNT[building->nation][building->size] / 2 + 12 + i * 22, GetY() + 110, 0, 0, 0, 0, 0, 0);

    // Draw health above soldiers
    if (GAMECLIENT.GetGGS().isEnabled(AddonId::MILITARY_HITPOINTS)) { 
        unsigned short leftXCoordinate = GetX() + width_ / 2 - 22 * TROOPS_COUNT[building->nation][building->size] / 2;

        // black background for hitpoints
        DrawRectangle(leftXCoordinate, GetY() + 84, 22 * TROOPS_COUNT[building->nation][building->size], 14, 0x96000000);

        i = 0;
        for (std::multiset<const nofSoldier*, ComparatorSoldiersByRank<true> >::const_iterator it = soldiers.begin(); it != soldiers.end(); ++it, ++i) {
            int hitpoints = static_cast<int>((*it)->GetHitpoints());
            int maxHitpoints = static_cast<int>(HITPOINTS[building->nation][(*it)->GetRank()]);
            unsigned int hitpointsColour;
            if (hitpoints <= maxHitpoints / 2)
                hitpointsColour = COLOR_RED;
            else
            {
                if (hitpoints == maxHitpoints)
                    hitpointsColour = COLOR_GREEN;
                else
                    hitpointsColour = COLOR_ORANGE;
            }  
            std::stringstream hitpointsText;
            hitpointsText << hitpoints;
            unsigned short x = leftXCoordinate + 12 + i * 22;
            NormalFont->Draw(x, GetY() + 86, hitpointsText.str(), glArchivItem_Font::DF_CENTER, hitpointsColour);
        }
    }
}


void iwMilitaryBuilding::Msg_ButtonClick(const unsigned int ctrl_id)
{
    switch(ctrl_id)
    {
        case 4: // Hilfe
        {
            WINDOWMANAGER.Show(new iwHelp(GUI_ID(CGI_HELPBUILDING + building->GetBuildingType()), _(BUILDING_NAMES[building->GetBuildingType()]),
                                                  _(BUILDING_HELP_STRINGS[building->GetBuildingType()])));
        } break;
        case 5: // Gebäude abbrennen
        {
            // Darf das Gebäude abgerissen werden?
            if(!building->IsDemolitionAllowed())
            {
                // Messagebox anzeigen
                DemolitionNotAllowed();
            }
            else
            {
                // Abreißen?
                Close();
                WINDOWMANAGER.Show(new iwDemolishBuilding(gwv, building));
            }
        } break;
        case 6: // Gold einstellen/erlauben
        {
            if(!GAMECLIENT.IsReplayModeOn() && !GAMECLIENT.IsPaused())
            {
                // NC senden
                if(GAMECLIENT.SetCoinsAllowed(building->GetPos(), building->IsGoldDisabledVirtual()))
                {
                    // visuell anzeigen
                    building->ToggleCoinsVirtual();
                    // anderes Bild auf dem Button
                    if(building->IsGoldDisabledVirtual())
                        GetCtrl<ctrlImageButton>(6)->SetImage(LOADER.GetImageN("io", 226));
                    else
                        GetCtrl<ctrlImageButton>(6)->SetImage(LOADER.GetImageN("io", 227));
                }
            }
        } break;
        case 7: // "Gehe Zu Ort"
        {
            gwv.MoveToMapPt(building->GetPos());
        } break;
		case 9: //go to next of same type
		{
            const std::list<nobMilitary*>& militaryBuildings = GAMECLIENT.GetPlayer(building->GetPlayer()).GetMilitaryBuildings();
			//go through list once we get to current building -> open window for the next one and go to next location
			for(std::list<nobMilitary*>::const_iterator it=militaryBuildings.begin(); it != militaryBuildings.end(); ++it)
			{
				if((*it)->GetX()==building->GetX() && (*it)->GetY()==building->GetY()) //got to current building in the list?
				{
					//close old window, open new window (todo: only open if it isnt already open), move to location of next building
					Close();
					++it;
					if(it == militaryBuildings.end()) //was last entry in list -> goto first												{
						it=militaryBuildings.begin();
					gwv.MoveToMapPt((*it)->GetPos());
					iwMilitaryBuilding* nextscrn=new iwMilitaryBuilding(gwv, *it);
					nextscrn->Move(x_,y_);
					WINDOWMANAGER.Show(nextscrn);
					break;
				}
			}
		} break;
		case 10: //send home button (addon)
		{
			GAMECLIENT.SendSoldiersHome(building->GetPos());
		}
		break;
    }
}

void iwMilitaryBuilding::DemolitionNotAllowed()
{
    // Meldung auswählen, je nach Einstellung
    std::string msg;
    switch(GAMECLIENT.GetGGS().getSelection(AddonId::DEMOLITION_PROHIBITION))
    {
        default: RTTR_Assert(false); break;
        case 1: msg = _("Demolition ist not allowed because the building is under attack!"); break;
        case 2: msg = _("Demolition ist not allowed because the building is located in border area!"); break;
    }

    WINDOWMANAGER.Show(new iwMsgbox(_("Demolition not possible"), msg, NULL, MSB_OK, MSB_EXCLAMATIONRED));
}

