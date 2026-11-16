// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "iwRoadWindow.h"

#include "GameInterface.h"
#include "Loader.h"
#include "controls/ctrlButton.h"
#include "drivers/VideoDriverWrapper.h"
#include "gameData/const_gui_ids.h"

iwRoadWindow::iwRoadWindow(GameInterface& gi, bool flagpossible, const Position& mousePos)
    : IngameWindow(CGI_ROADWINDOW, IngameWindow::posAtMouse, Extent(200, 100), _("Activity window"),
                   LOADER.getImageN("io", 1)),
      gi(gi), mousePosAtOpen_(mousePos)
{
    // Bau abbrechen
    ctrlButton* cancel = addImageButton(1, DrawPoint(10, 20), Extent(36, 36), TextureColor::Grey,
                                        LOADER.getImageN("io", 110), _("Interrupt road building"));
    ctrlButton* defaultBt = cancel;

    if(flagpossible)
    {
        // Flagge platzieren
        defaultBt = addImageButton(0, DrawPoint(10, 20), Extent(36, 36), TextureColor::Grey, LOADER.getImageN("io", 70),
                                   _("Erect flag"));
        // Abbrechen button daneben schieben
        cancel->setPos(DrawPoint(46, 20));
    }

    VIDEODRIVER.setMousePos(defaultBt->getDrawPos() + DrawPoint(defaultBt->getSize()) / 2);
}

void iwRoadWindow::msgButtonClick(const unsigned ctrl_id)
{
    switch(ctrl_id)
    {
        case 0: // Flagge & Weg bauen
        {
            gi.GI_BuildRoad();
        }
        break;
        case 1: // Bau abbrechen
        {
            gi.GI_CancelRoadBuilding();
        }
        break;
    }

    // Maus an vorherige Stelle setzen
    VIDEODRIVER.setMousePos(mousePosAtOpen_);

    // und fenster schließen
    close();
}
