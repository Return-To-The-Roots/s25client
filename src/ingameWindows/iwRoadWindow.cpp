// Copyright (c) 2005 - 2015 Settlers Freaks (sf-team at siedler25.org)
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

///////////////////////////////////////////////////////////////////////////////
// Header
#include "defines.h"
#include "iwRoadWindow.h"

#include "Loader.h"
#include "drivers/VideoDriverWrapper.h"
#include "controls/controls.h"

#include "desktops/dskGameInterface.h"

///////////////////////////////////////////////////////////////////////////////
// Makros / Defines
#if defined _WIN32 && defined _DEBUG && defined _MSC_VER
#define new new(_NORMAL_BLOCK, THIS_FILE, __LINE__)
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

///////////////////////////////////////////////////////////////////////////////
/**
 *  Konstruktor von @p iwRoadWindow.
 *
 *  @author OLiver
 */
iwRoadWindow::iwRoadWindow(dskGameInterface* const GameInterface, bool flagpossible, int mouse_x, int mouse_y)
    : IngameWindow(CGI_ROADWINDOW, mouse_x, mouse_y, 200, 100, _("Activity window"), LOADER.GetImageN("io", 1)),
      GameInterface(GameInterface), mousePosAtOpen_(mouse_x, mouse_y)
{
    // Bau abbrechen
    ctrlButton* cancel = AddImageButton(1, 10, 20, 36, 36, TC_GREY, LOADER.GetImageN("io", 110), _("Interrupt road building"));

    if(flagpossible)
    {
        // Flagge platzieren
        AddImageButton(0, 10, 20, 36, 36, TC_GREY, LOADER.GetImageN("io", 70), _("Erect flag"));
        // Abbrechenbutton daneben schieben
        cancel->Move(46, 20);
    }

    if(x + GetWidth() > VIDEODRIVER.GetScreenWidth())
        x = mouse_x - GetWidth() - 40;
    if(y + GetIwHeight() > VIDEODRIVER.GetScreenHeight())
        y = mouse_y - GetIwHeight() - 40;

    VIDEODRIVER.SetMousePos(GetX() + 20, GetY() + 45);
}

iwRoadWindow::~iwRoadWindow()
{
    GameInterface->RoadWindowClosed();
}

void iwRoadWindow::Msg_ButtonClick(const unsigned int ctrl_id)
{
    switch(ctrl_id)
    {
        case 0: // Flagge & Weg bauen
        {
            GameInterface->CommandBuildRoad();
        } break;
        case 1: // Bau abbrechen
        {
            GameInterface->ActivateRoadMode(RM_DISABLED);
        } break;
    }

    // Maus an vorherige Stelle setzen
    VIDEODRIVER.SetMousePos(mousePosAtOpen_.x, mousePosAtOpen_.y);

    // und fenster schließen
    Close();
}
