// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "nobStorehouse.h"
#include "EventManager.h"
#include "postSystem/PostMsgWithBuilding.h"

nobStorehouse::nobStorehouse(const MapPoint pos, const unsigned char player, const Nation nation)
    : nobBaseWarehouse(BuildingType::Storehouse, pos, player, nation)
{
    // Reset all goods and keep the 100 carriers placeholder. TODO: Revisit this default?
    inventory.clear();

    // Merge the current stock into the inventory snapshot
    AddToInventory();

    // Notify the player via post message
    SendPostMessage(player, std::make_unique<PostMsgWithBuilding>(
                              GetEvMgr().GetCurrentGF(), _("New storehouse finished"), PostCategory::Economy, *this));
}

nobStorehouse::nobStorehouse(SerializedGameData& sgd, const unsigned obj_id) : nobBaseWarehouse(sgd, obj_id) {}

void nobStorehouse::Draw(DrawPoint drawPt)
{
    // Draw the storehouse
    DrawBaseBuilding(drawPt);
}

void nobStorehouse::HandleEvent(const unsigned id)
{
    HandleBaseEvent(id);
}
