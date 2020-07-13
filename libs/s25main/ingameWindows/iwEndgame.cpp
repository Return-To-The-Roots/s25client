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

#include "iwEndgame.h"
#include "GameManager.h"
#include "Loader.h"
#include "WindowManager.h"
#include "iwSave.h"
#include "gameData/const_gui_ids.h"

iwEndgame::iwEndgame()
    : IngameWindow(CGI_ENDGAME, IngameWindow::posLastOrCenter, Extent(240, 100), _("End game?"), LOADER.GetImageN("resource", 41))
{
    // Ok
    AddImageButton(0, DrawPoint(16, 24), Extent(71, 57), TC_GREEN2, LOADER.GetImageN("io", 32)); //-V525
    // Abbrechen
    AddImageButton(1, DrawPoint(88, 24), Extent(71, 57), TC_RED1, LOADER.GetImageN("io", 40));
    // Ok + Speichern
    AddImageButton(2, DrawPoint(160, 24), Extent(65, 57), TC_GREY, LOADER.GetImageN("io", 47));
}

void iwEndgame::Msg_ButtonClick(const unsigned ctrl_id)
{
    switch(ctrl_id)
    {
        case 0: // OK
        {
            GAMEMANAGER.ShowMenu();
        }
        break;
        case 1: // Abbrechen
        {
            Close();
        }
        break;
        case 2: // OK + Speichern
        {
            WINDOWMANAGER.ToggleWindow(std::make_unique<iwSave>());
        }
        break;
    }
}
