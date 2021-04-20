// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

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
