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
    : iwBuilding(gwv, gcFactory, building, Extent(226, 223))
{
    GetCtrl<Window>(1)->SetPos(DrawPoint(117, 160));
    GetCtrl<ctrlImage>(3)->SetImage(wineaddon::GetTempleProductionModeTex(ProductionMode::Default));
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