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

#include "defines.h" // IWYU pragma: keep
#include "iwDemolishBuilding.h"
#include "GameClient.h"
#include "Loader.h"
#include "buildings/noBaseBuilding.h"
#include "ogl/glArchivItem_Font.h"
#include "world/GameWorldView.h"
#include "world/GameWorldViewer.h"

iwDemolishBuilding::iwDemolishBuilding(GameWorldView& gwv, const noBaseBuilding* building, const bool flag)
    : IngameWindow(building->CreateGUIID(), IngameWindow::posAtMouse, Extent(200, 200), _("Demolish?"), LOADER.GetImageN("resource", 41)),
      gwv(gwv), building(building), flag(flag)
{
    // Ja
    AddImageButton(0, DrawPoint(14, 140), Extent(66, 40), TC_RED1, LOADER.GetImageN("io", 32), _("Yes")); //-V525
    // Nein
    AddImageButton(1, DrawPoint(82, 140), Extent(66, 40), TC_GREY, LOADER.GetImageN("io", 40), _("No"));
    // Gehe zum Standort
    AddImageButton(2, DrawPoint(150, 140), Extent(36, 40), TC_GREY, LOADER.GetImageN("io", 107), _("Go to place"));
    // Gebäudebild
    AddImage(3, DrawPoint(104, 109), building->GetBuildingImage());
    // Gebäudename
    AddText(4, DrawPoint(100, 125), _(BUILDING_NAMES[building->GetBuildingType()]), 0xFFFFFF00, glArchivItem_Font::DF_CENTER, NormalFont);
}

void iwDemolishBuilding::Msg_ButtonClick(const unsigned ctrl_id)
{
    switch(ctrl_id)
    {
        case 0:
        {
            if(flag)
            {
                // Flagge (mitsamt Gebäude) wegreißen
                GAMECLIENT.DestroyFlag(gwv.GetViewer().GetNeighbour(building->GetPos(), Direction::SOUTHEAST));
            } else
            {
                GAMECLIENT.DestroyBuilding(building->GetPos());
            }

            Close();
        }
        break;
        case 1:
        {
            // Einfach schließen
            Close();
        }
        break;
        case 2:
        {
            // Zum Ort gehen
            gwv.MoveToMapPt(building->GetPos());
        }
        break;
    }
}

void iwDemolishBuilding::Msg_PaintBefore()
{
    IngameWindow::Msg_PaintBefore();
    // Schatten des Gebäudes (muss hier gezeichnet werden wegen schwarz und halbdurchsichtig)
    glArchivItem_Bitmap* bitmap = building->GetBuildingImageShadow();

    if(bitmap)
        bitmap->DrawFull(GetDrawPos() + DrawPoint(104, 109), COLOR_SHADOW);
}
