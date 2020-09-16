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

#include "iwBuildingSite.h"
#include "Loader.h"
#include "WindowManager.h"
#include "buildings/noBuildingSite.h"
#include "iwDemolishBuilding.h"
#include "iwHelp.h"
#include "ogl/FontStyle.h"
#include "ogl/glArchivItem_Bitmap.h"
#include "world/GameWorldView.h"
#include "world/MapBase.h"
#include "gameData/BuildingConsts.h"
#include "gameData/const_gui_ids.h"

iwBuildingSite::iwBuildingSite(GameWorldView& gwv, const noBuildingSite* const buildingsite)
    : IngameWindow(CGI_BUILDING + MapBase::CreateGUIID(buildingsite->GetPos()), IngameWindow::posAtMouse,
                   Extent(226, 194), _(BUILDING_NAMES[buildingsite->GetBuildingType()]),
                   LOADER.GetImageN("resource", 41)),
      gwv(gwv), buildingsite(buildingsite)
{
    // Bild des Gebäudes
    AddImage(0, DrawPoint(113, 130), buildingsite->GetBuildingImage());
    // Gebäudename
    AddText(1, DrawPoint(113, 44), _("Order of building site"), COLOR_YELLOW, FontStyle::CENTER, NormalFont);

    // Hilfe
    AddImageButton(2, DrawPoint(16, 147), Extent(30, 32), TC_GREY, LOADER.GetImageN("io", 225), _("Help"));
    // Gebäude abbrennen
    AddImageButton(3, DrawPoint(50, 147), Extent(34, 32), TC_GREY, LOADER.GetImageN("io", 23), _("Demolish house"));

    // "Gehe Zu Ort"
    AddImageButton(4, DrawPoint(179, 147), Extent(30, 32), TC_GREY, LOADER.GetImageN("io", 107), _("Go to place"));
}

void iwBuildingSite::Msg_ButtonClick(const unsigned ctrl_id)
{
    switch(ctrl_id)
    {
        case 2: // Hilfe
        {
            WINDOWMANAGER.ReplaceWindow(
              std::make_unique<iwHelp>(_(BUILDING_HELP_STRINGS[buildingsite->GetBuildingType()])));
        }
        break;
        case 3: // Gebäude abbrennen
        {
            // Abreißen?
            Close();
            WINDOWMANAGER.Show(std::make_unique<iwDemolishBuilding>(gwv, buildingsite));
        }
        break;
        case 4: // "Gehe Zu Ort"
        {
            gwv.MoveToMapPt(buildingsite->GetPos());
        }
        break;
    }
}

void iwBuildingSite::Msg_PaintAfter()
{
    IngameWindow::Msg_PaintAfter();
    // Baukosten zeichnen
    DrawPoint curPos = GetDrawPos() + DrawPoint(GetSize().x / 2, 60);
    for(unsigned char i = 0; i < 2; ++i)
    {
        unsigned wares_count = 0;
        unsigned wares_delivered = 0;
        unsigned wares_used = 0;

        if(i == 0)
        {
            wares_count = BUILDING_COSTS[buildingsite->GetNation()][buildingsite->GetBuildingType()].boards;
            wares_used = buildingsite->getUsedBoards();
            wares_delivered = buildingsite->getBoards() + wares_used;
        } else
        {
            wares_count = BUILDING_COSTS[buildingsite->GetNation()][buildingsite->GetBuildingType()].stones;
            wares_used = buildingsite->getUsedStones();
            wares_delivered = buildingsite->getStones() + wares_used;
        }

        if(wares_count == 0)
            break;

        // "Schwarzer Rahmen"
        DrawPoint waresPos = curPos - DrawPoint(24 * wares_count / 2, 0);
        DrawRectangle(Rect(waresPos, Extent(24 * wares_count, 24)), 0x80000000);
        waresPos += DrawPoint(12, 12);

        // Die Waren
        for(unsigned char z = 0; z < wares_count; ++z)
        {
            glArchivItem_Bitmap* bitmap = LOADER.GetMapImageN(WARES_TEX_MAP_OFFSET + (i == 0 ? GD_BOARDS : GD_STONES));
            bitmap->DrawFull(waresPos, (z < wares_delivered ? 0xFFFFFFFF : 0xFF404040));

            // Hammer wenn Ware verbaut
            if(z < wares_used)
                LOADER.GetMapImageN(WARES_TEX_MAP_OFFSET + GD_HAMMER)->DrawFull(waresPos);
            waresPos.x += 24;
        }
        curPos.y += 29;
    }
}
