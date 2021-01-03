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

#include "dskIntro.h"

#include "Loader.h"
#include "WindowManager.h"

#include "dskMainMenu.h"

/** @class dskIntro
 *
 *  Klasse des Intro Desktops.
 */

dskIntro::dskIntro() : Desktop(LOADER.GetImageN("menu", 0))
{
    // "Zurück"
    AddTextButton(0, DrawPoint(300, 550), Extent(200, 22), TextureColor::Red1, _("Back"), NormalFont);

    AddImage(11, DrawPoint(20, 20), LOADER.GetImageN("logo", 0));
}

void dskIntro::Msg_ButtonClick(const unsigned ctrl_id)
{
    switch(ctrl_id)
    {
        case 0: // "Zurück"
        {
            WINDOWMANAGER.Switch(std::make_unique<dskMainMenu>());
        }
        break;
    }
}
