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

#include "iwRoadWindow.h"

#include "GameInterface.h"
#include "Loader.h"
#include "controls/ctrlButton.h"
#include "drivers/VideoDriverWrapper.h"
#include "gameData/const_gui_ids.h"

iwRoadWindow::iwRoadWindow(GameInterface& gi, bool flagpossible, const Position& mousePos)
    : IngameWindow(CGI_ROADWINDOW, IngameWindow::posAtMouse, Extent(200, 100), _("Activity window"),
                   LOADER.GetImageN("io", 1)),
      gi(gi), mousePosAtOpen_(mousePos)
{
    // Bau abbrechen
    ctrlButton* cancel = AddImageButton(1, DrawPoint(10, 20), Extent(36, 36), TextureColor::Grey,
                                        LOADER.GetImageN("io", 110), _("Interrupt road building"));
    ctrlButton* defaultBt = cancel;

    if(flagpossible)
    {
        // Flagge platzieren
        defaultBt = AddImageButton(0, DrawPoint(10, 20), Extent(36, 36), TextureColor::Grey, LOADER.GetImageN("io", 70),
                                   _("Erect flag"));
        // Abbrechen button daneben schieben
        cancel->SetPos(DrawPoint(46, 20));
    }

    VIDEODRIVER.SetMousePos(defaultBt->GetDrawPos() + DrawPoint(defaultBt->GetSize()) / 2);
}

iwRoadWindow::~iwRoadWindow()
{
    gi.GI_WindowClosed(this);
}

void iwRoadWindow::Msg_ButtonClick(const unsigned ctrl_id)
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
    VIDEODRIVER.SetMousePos(mousePosAtOpen_);

    // und fenster schließen
    Close();
}
