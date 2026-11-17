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
    setTitle(_("Headquarters"));

    // Soldaten Reservierungsseite
    ctrlGroup& reserve = addPage();
    grpIdReserve = reserve.getID();

    // "Reserve"-Überschrift
    reserve.addText(0, DrawPoint(83, 87), _("Reserve"), 0xFFFFFF00, FontStyle::CENTER, NormalFont);

    // Y-Abstand zwischen den Zeilen
    const unsigned Y_DISTANCE = 30;

    for(unsigned i = 0; i < 5; ++i)
    {
        // Bildhintergrund
        reserve.addImage(1 + i, DrawPoint(34, 124 + Y_DISTANCE * i), LOADER.getMapTexture(2298));
        // Rang-Bild
        reserve.addImage(6 + i, DrawPoint(34, 124 + Y_DISTANCE * i), LOADER.getMapTexture(2321 + i));
        // Minus-Button
        reserve.addImageButton(11 + i, DrawPoint(54, 112 + Y_DISTANCE * i), Extent(24, 24), TextureColor::Red1,
                               LOADER.getImageN("io", 139), _("Less"));
        // Plus-Button
        reserve.addImageButton(16 + i, DrawPoint(118, 112 + Y_DISTANCE * i), Extent(24, 24), TextureColor::Green2,
                               LOADER.getImageN("io", 138), _("More"));
        // Anzahl-Text
        reserve.addVarText(21 + i, DrawPoint(100, 117 + Y_DISTANCE * i), _("%u/%u"), 0xFFFFFF00, FontStyle::CENTER,
                           NormalFont, 2, wh->getReserveAvailablePointer(i), wh->getReserveClaimedVisualPointer(i));
    }
}

void iwHQ::msgGroupButtonClick(const unsigned group_id, const unsigned ctrl_id)
{
    if(group_id == grpIdReserve)
    {
        RTTR_Assert(ctrl_id >= 11 && ctrl_id < 21);
        unsigned rank = 0, newReserve = 0, oldReserve = 0;

        // Minus-Button
        if(ctrl_id >= 11 && ctrl_id < 16)
        {
            rank = ctrl_id - 11;
            oldReserve = *(wh->getReserveClaimedVisualPointer(rank));
            newReserve = oldReserve > 0 ? oldReserve - 1 : oldReserve;
        }
        // Plus-Button
        else if(ctrl_id >= 16 && ctrl_id < 21)
        {
            rank = ctrl_id - 16;
            oldReserve = *(wh->getReserveClaimedVisualPointer(rank));
            newReserve = oldReserve + 1;
        }

        // Netzwerk-Nachricht generieren
        if(newReserve != oldReserve && GAMECLIENT.ChangeReserve(wh->getPos(), rank, newReserve))
            wh->setReserveVisual(rank, newReserve);
    }

    // an Basis weiterleiten
    iwBaseWarehouse::msgGroupButtonClick(group_id, ctrl_id);
}
