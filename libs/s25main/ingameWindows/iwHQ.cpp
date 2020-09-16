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

#include "iwHQ.h"
#include "Loader.h"
#include "buildings/nobBaseWarehouse.h"
#include "controls/ctrlGroup.h"
#include "network/GameClient.h"
#include "ogl/FontStyle.h"

iwHQ::iwHQ(GameWorldView& gwv, GameCommandFactory& gcFactory, nobBaseWarehouse* wh)
    : iwBaseWarehouse(gwv, gcFactory, wh)
{
    SetTitle(_("Headquarters"));

    // Soldaten Reservierungsseite
    ctrlGroup& reserve = AddPage();
    grpIdReserve = reserve.GetID();

    // "Reserve"-Überschrift
    reserve.AddText(0, DrawPoint(83, 87), _("Reserve"), 0xFFFFFF00, FontStyle::CENTER, NormalFont);

    // Y-Abstand zwischen den Zeilen
    const unsigned Y_DISTANCE = 30;

    for(unsigned i = 0; i < 5; ++i)
    {
        // Bildhintergrund
        reserve.AddImage(1 + i, DrawPoint(34, 124 + Y_DISTANCE * i), LOADER.GetMapImageN(2298));
        // Rang-Bild
        reserve.AddImage(6 + i, DrawPoint(34, 124 + Y_DISTANCE * i), LOADER.GetMapImageN(2321 + i));
        // Minus-Button
        reserve.AddImageButton(11 + i, DrawPoint(54, 112 + Y_DISTANCE * i), Extent(24, 24), TC_RED1,
                               LOADER.GetImageN("io", 139), _("Less"));
        // Plus-Button
        reserve.AddImageButton(16 + i, DrawPoint(118, 112 + Y_DISTANCE * i), Extent(24, 24), TC_GREEN2,
                               LOADER.GetImageN("io", 138), _("More"));
        // Anzahl-Text
        reserve.AddVarText(21 + i, DrawPoint(100, 117 + Y_DISTANCE * i), _("%u/%u"), 0xFFFFFF00, FontStyle::CENTER,
                           NormalFont, 2, wh->GetReserveAvailablePointer(i), wh->GetReserveClaimedVisualPointer(i));
    }
}

void iwHQ::Msg_Group_ButtonClick(const unsigned group_id, const unsigned ctrl_id)
{
    if(group_id == grpIdReserve)
    {
        // Minus-Button
        if(ctrl_id >= 11 && ctrl_id < 16)
        {
            // Netzwerk-Nachricht generieren
            GAMECLIENT.ChangeReserve(wh->GetPos(), ctrl_id - 11, wh->DecreaseReserveVisual(ctrl_id - 11));
        }
        // Plus-Button
        else if(ctrl_id >= 16 && ctrl_id < 21)
        {
            // Netzwerk-Nachricht generieren
            GAMECLIENT.ChangeReserve(wh->GetPos(), ctrl_id - 16, wh->IncreaseReserveVisual(ctrl_id - 16));
        }
    }

    // an Basis weiterleiten
    iwBaseWarehouse::Msg_Group_ButtonClick(group_id, ctrl_id);
}
