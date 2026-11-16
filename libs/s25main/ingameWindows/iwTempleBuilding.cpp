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
    getCtrl<Window>(1)->setPos(DrawPoint(117, 160));
    getCtrl<ctrlImage>(3)->setImage(wineaddon::getTempleProductionModeTex(ProductionMode::Default));
    AddImageButton(8, DrawPoint(130, 176), Extent(34, 32), TextureColor::Grey,
                   wineaddon::getTempleProductionModeTex(static_cast<nobTemple*>(building)->getProductionMode()),
                   _("Output mineral"));
}

void iwTempleBuilding::msgButtonClick(const unsigned ctrl_id)
{
    if(ctrl_id == 8)
    {
        const auto nextProductionMode = static_cast<nobTemple*>(building)->getNextProductionMode();
        if(gcFactory.SetTempleProductionMode(building->getPos(), nextProductionMode))
        {
            getCtrl<ctrlImageButton>(8)->setImage(wineaddon::getTempleProductionModeTex(nextProductionMode));
            static_cast<nobTemple*>(building)->setProductionMode(nextProductionMode);
        }
    } else
        iwBuilding::msgButtonClick(ctrl_id);
}