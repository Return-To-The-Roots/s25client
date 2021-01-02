// Copyright (c) 2005 - 2020 Settlers Freaks (sf-team at siedler25.org)
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

void nobStorehouse::Serialize_nobStorehouse(SerializedGameData& sgd) const
{
    Serialize_nobBaseWarehouse(sgd);
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
