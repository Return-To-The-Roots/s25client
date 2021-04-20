// Copyright (c) 2005 - 2020 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "nobStorehouse.h"
#include "EventManager.h"
#include "postSystem/PostMsgWithBuilding.h"

nobStorehouse::nobStorehouse(const MapPoint pos, const unsigned char player, const Nation nation)
    : nobBaseWarehouse(BuildingType::Storehouse, pos, player, nation)
{
    // Alle Waren 0, außer 100 Träger. TODO: Really?
    inventory.clear();

    // Aktuellen Warenbestand zur aktuellen Inventur dazu addieren
    AddToInventory();

    // Post versenden
    SendPostMessage(player, std::make_unique<PostMsgWithBuilding>(
                              GetEvMgr().GetCurrentGF(), _("New storehouse finished"), PostCategory::Economy, *this));
}

nobStorehouse::nobStorehouse(SerializedGameData& sgd, const unsigned obj_id) : nobBaseWarehouse(sgd, obj_id) {}

void nobStorehouse::Draw(DrawPoint drawPt)
{
    // Gebäude an sich zeichnen
    DrawBaseBuilding(drawPt);
}

void nobStorehouse::HandleEvent(const unsigned id)
{
    HandleBaseEvent(id);
}
