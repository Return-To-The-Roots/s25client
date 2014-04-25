// $Id: iwBuildingSite.cpp 9357 2014-04-25 15:35:25Z FloSoft $
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
#include "iwBuildingSite.h"

#include "VideoDriverWrapper.h"

#include "Loader.h"
#include "GameClient.h"
#include "WindowManager.h"
#include "noBuildingSite.h"

#include "iwDemolishBuilding.h"
#include "iwMsgbox.h"
#include "iwHelp.h"

///////////////////////////////////////////////////////////////////////////////
// Makros / Defines
#if defined _WIN32 && defined _DEBUG && defined _MSC_VER
#define new new(_NORMAL_BLOCK, THIS_FILE, __LINE__)
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

///////////////////////////////////////////////////////////////////////////////
/**
 *  Konstruktor von @p iwBuildingSite.
 *
 *  @author OLiver
 */
iwBuildingSite::iwBuildingSite(GameWorldViewer* const gwv, const noBuildingSite* const buildingsite)
    : IngameWindow(buildingsite->CreateGUIID(), 0xFFFE, 0xFFFE, 226, 194, _(BUILDING_NAMES[buildingsite->GetBuildingType()]), LOADER.GetImageN("resource", 41)),
      gwv(gwv), buildingsite(buildingsite)
{
    // Bild des Gebäudes
    AddImage(0, 113, 130, buildingsite->GetBuildingImage());
    // Gebäudename
    AddText(1, 113, 44, _("Order of building site"), COLOR_YELLOW, glArchivItem_Font::DF_CENTER, NormalFont);

    // Hilfe
    AddImageButton( 2,  16, 147, 30, 32, TC_GREY, LOADER.GetImageN("io",  21));
    // Gebäude abbrennen
    AddImageButton( 3,  50, 147, 34, 32, TC_GREY, LOADER.GetImageN("io",  23));

    // "Gehe Zu Ort"
    AddImageButton( 4, 179, 147, 30, 32, TC_GREY, LOADER.GetImageN("io", 107), _("Go to place"));
}

///////////////////////////////////////////////////////////////////////////////
/**
 *
 *
 *  @author OLiver
 */
void iwBuildingSite::Msg_ButtonClick(const unsigned int ctrl_id)
{
    switch(ctrl_id)
    {
        case 2: // Hilfe
        {
            WindowManager::inst().Show(new iwHelp(GUI_ID(CGI_HELPBUILDING + buildingsite->GetBuildingType()),
                                                  _(BUILDING_NAMES[buildingsite->GetBuildingType()]),
                                                  _(BUILDING_HELP_STRINGS[buildingsite->GetBuildingType()]) ) );
        } break;
        case 3: // Gebäude abbrennen
        {
            // Abreißen?
            Close();
            WindowManager::inst().Show(new iwDemolishBuilding(gwv, buildingsite));
        } break;
        case 4: // "Gehe Zu Ort"
        {
            gwv->MoveToMapObject(buildingsite->GetX(), buildingsite->GetY());
        } break;
    }
}

void iwBuildingSite::Msg_PaintBefore()
{
    // Schatten des Gebäudes (muss hier gezeichnet werden wegen schwarz und halbdurchsichtig)
    glArchivItem_Bitmap* bitmap = buildingsite->GetBuildingImageShadow();

    if(bitmap)
        bitmap->Draw(GetX() + 113, GetY() + 130, 0, 0, 0, 0, 0, 0, COLOR_SHADOW);
}

void iwBuildingSite::Msg_PaintAfter()
{
    // Baukosten zeichnen
    for(unsigned char i = 0; i < 2; ++i)
    {
        unsigned int wares_count = 0;
        unsigned int wares_delivered = 0;
        unsigned int wares_used = 0;

        if(i == 0)
        {
            wares_count = BUILDING_COSTS[buildingsite->GetNation()][buildingsite->GetBuildingType()].boards;
            wares_used = buildingsite->getUsedBoards();
            wares_delivered = buildingsite->getBoards() + wares_used;
        }
        else
        {
            wares_count = BUILDING_COSTS[buildingsite->GetNation()][buildingsite->GetBuildingType()].stones;
            wares_used = buildingsite->getUsedStones();
            wares_delivered = buildingsite->getStones() + wares_used;
        }

        if(wares_count == 0)
            break;

        // "Schwarzer Rahmen"
        DrawRectangle(GetX() + width / 2 - 24 * wares_count / 2, GetY() + 60 + i * 29, 24 * wares_count, 24, 0x80000000);

        // Die Waren
        for(unsigned char z = 0; z < wares_count; ++z)
        {
            glArchivItem_Bitmap* bitmap = LOADER.GetMapImageN(2250 + (i == 0 ? GD_BOARDS : GD_STONES));
            bitmap->Draw(GetX() + width / 2 - 24 * wares_count / 2 + 24 * z + 12, GetY() + 72 + i * 28, 0, 0, 0, 0, 0, 0, (z < wares_delivered ? 0xFFFFFFFF : 0xFF404040) );

            // Hammer wenn Ware verbaut
            if(z < wares_used)
                LOADER.GetMapImageN(2250 + GD_HAMMER)->Draw(GetX() + width / 2 - 24 * wares_count / 2 + 24 * z + 12, GetY() + 72 + i * 28);
        }
    }
}
