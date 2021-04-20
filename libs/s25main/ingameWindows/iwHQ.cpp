// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

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
        reserve.AddImageButton(11 + i, DrawPoint(54, 112 + Y_DISTANCE * i), Extent(24, 24), TextureColor::Red1,
                               LOADER.GetImageN("io", 139), _("Less"));
        // Plus-Button
        reserve.AddImageButton(16 + i, DrawPoint(118, 112 + Y_DISTANCE * i), Extent(24, 24), TextureColor::Green2,
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
        RTTR_Assert(ctrl_id >= 11 && ctrl_id < 21);
        unsigned rank = 0, newReserve = 0, oldReserve = 0;

        // Minus-Button
        if(ctrl_id >= 11 && ctrl_id < 16)
        {
            rank = ctrl_id - 11;
            oldReserve = *(wh->GetReserveClaimedVisualPointer(rank));
            newReserve = oldReserve > 0 ? oldReserve - 1 : oldReserve;
        }
        // Plus-Button
        else if(ctrl_id >= 16 && ctrl_id < 21)
        {
            rank = ctrl_id - 16;
            oldReserve = *(wh->GetReserveClaimedVisualPointer(rank));
            newReserve = oldReserve + 1;
        }

        // Netzwerk-Nachricht generieren
        if(newReserve != oldReserve && GAMECLIENT.ChangeReserve(wh->GetPos(), rank, newReserve))
            wh->SetReserveVisual(rank, newReserve);
    }

    // an Basis weiterleiten
    iwBaseWarehouse::Msg_Group_ButtonClick(group_id, ctrl_id);
}
