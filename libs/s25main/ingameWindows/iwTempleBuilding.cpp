// Copyright (C) 2024 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "iwTempleBuilding.h"
#include "WineLoader.h"
#include "buildings/nobTemple.h"
#include "controls/ctrlImage.h"
#include "controls/ctrlImageButton.h"
#include "factories/GameCommandFactory.h"
#include "world/GameWorldView.h"

iwTempleBuilding::iwTempleBuilding(GameWorldView& gwv, GameCommandFactory& gcFactory, nobUsual* const building)
    : iwBuilding(gwv, gcFactory, building)
{
    Resize(Extent(226, 223));

    GetCtrl<ctrlImage>(3)->SetImage(
      wineaddon::GetTempleProductionModeTex(static_cast<nobTemple*>(building)->GetProductionMode()));

    GetCtrl<Window>(1)->SetPos(DrawPoint(117, 160));
    GetCtrl<Window>(4)->SetPos(DrawPoint(16, 176));
    GetCtrl<Window>(5)->SetPos(DrawPoint(50, 176));
    GetCtrl<Window>(6)->SetPos(DrawPoint(90, 176));
    GetCtrl<Window>(7)->SetPos(DrawPoint(179, 176));
    GetCtrl<Window>(12)->SetPos(DrawPoint(179, 144));

    AddImageButton(8, DrawPoint(130, 176), Extent(34, 32), TextureColor::Grey,
                   wineaddon::GetTempleProductionModeTex(static_cast<nobTemple*>(building)->GetProductionMode()));
}

void iwTempleBuilding::Msg_ButtonClick(const unsigned ctrl_id)
{
    if(ctrl_id == 8)
    {
        const auto nextProductionMode = static_cast<nobTemple*>(building)->getNextProductionMode();
        if(gcFactory.SetTempleProductionMode(building->GetPos(), nextProductionMode))
        {
            GetCtrl<ctrlImageButton>(8)->SetImage(wineaddon::GetTempleProductionModeTex(nextProductionMode));
            static_cast<nobTemple*>(building)->SetProductionMode(nextProductionMode);
        }
    } else
        iwBuilding::Msg_ButtonClick(ctrl_id);
}
