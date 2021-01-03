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

#include "iwSurrender.h"
#include "Loader.h"
#include "network/GameClient.h"
#include "gameData/const_gui_ids.h"

iwSurrender::iwSurrender()
    : IngameWindow(CGI_ENDGAME, IngameWindow::posLastOrCenter, Extent(240, 100), _("Surrender game?"),
                   LOADER.GetImageN("resource", 41))
{
    // Ok
    AddImageButton(0, DrawPoint(85, 24), Extent(68, 57), TextureColor::Green2, LOADER.GetImageN("io", 32),
                   _("Surrender"));
    // Ok + Abbrennen
    AddImageButton(2, DrawPoint(16, 24), Extent(68, 57), TextureColor::Green2, LOADER.GetImageN("io", 23),
                   _("Destroy all buildings and surrender"));
    // Abbrechen
    AddImageButton(1, DrawPoint(158, 24), Extent(68, 57), TextureColor::Red1, LOADER.GetImageN("io", 40),
                   _("Don't surrender"));
}

void iwSurrender::Msg_ButtonClick(const unsigned ctrl_id)
{
    switch(ctrl_id)
    {
        case 0: // OK
        {
            GAMECLIENT.Surrender();
            Close();
        }
        break;
        case 1: // Abbrechen
        {
            Close();
        }
        break;
        case 2: // OK + Alles abbrennen
        {
            GAMECLIENT.DestroyAll();
            Close();
        }
        break;
    }
}
