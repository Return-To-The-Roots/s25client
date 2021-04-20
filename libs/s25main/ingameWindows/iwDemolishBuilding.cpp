// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "iwDemolishBuilding.h"
#include "Loader.h"
#include "buildings/noBaseBuilding.h"
#include "network/GameClient.h"
#include "ogl/FontStyle.h"
#include "world/GameWorldView.h"
#include "world/GameWorldViewer.h"
#include "world/MapBase.h"
#include "gameData/BuildingConsts.h"
#include "gameData/const_gui_ids.h"

iwDemolishBuilding::iwDemolishBuilding(GameWorldView& gwv, const noBaseBuilding* building, const bool flag)
    : IngameWindow(CGI_BUILDING + MapBase::CreateGUIID(building->GetPos()), IngameWindow::posAtMouse, Extent(200, 200),
                   _("Demolish?"), LOADER.GetImageN("resource", 41)),
      gwv(gwv), building(building), flag(flag)
{
    // Ja
    AddImageButton(0, DrawPoint(14, 140), Extent(66, 40), TextureColor::Red1, LOADER.GetImageN("io", 32),
                   _("Yes")); //-V525
    // Nein
    AddImageButton(1, DrawPoint(82, 140), Extent(66, 40), TextureColor::Grey, LOADER.GetImageN("io", 40), _("No"));
    // Gehe zum Standort
    AddImageButton(2, DrawPoint(150, 140), Extent(36, 40), TextureColor::Grey, LOADER.GetImageN("io", 107),
                   _("Go to place"));
    // Gebäudebild
    AddImage(3, DrawPoint(104, 109), &building->GetBuildingImage());
    // Gebäudename
    AddText(4, DrawPoint(100, 125), _(BUILDING_NAMES[building->GetBuildingType()]), 0xFFFFFF00, FontStyle::CENTER,
            NormalFont);
}

void iwDemolishBuilding::Msg_ButtonClick(const unsigned ctrl_id)
{
    switch(ctrl_id)
    {
        case 0:
        {
            bool success = false;
            if(flag)
            {
                // Flagge (mitsamt Gebäude) wegreißen
                success =
                  GAMECLIENT.DestroyFlag(gwv.GetViewer().GetNeighbour(building->GetPos(), Direction::SouthEast));
            } else
            {
                success = GAMECLIENT.DestroyBuilding(building->GetPos());
            }

            if(success)
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
