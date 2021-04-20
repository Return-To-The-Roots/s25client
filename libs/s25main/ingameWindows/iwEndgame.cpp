// Copyright (c) 2005 - 2017 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "iwEndgame.h"
#include "GameManager.h"
#include "Loader.h"
#include "WindowManager.h"
#include "iwSave.h"
#include "gameData/const_gui_ids.h"

iwEndgame::iwEndgame()
    : IngameWindow(CGI_ENDGAME, IngameWindow::posLastOrCenter, Extent(240, 100), _("End game?"),
                   LOADER.GetImageN("resource", 41))
{
    // Ok
    AddImageButton(0, DrawPoint(16, 24), Extent(71, 57), TextureColor::Green2, LOADER.GetImageN("io", 32)); //-V525
    // Abbrechen
    AddImageButton(1, DrawPoint(88, 24), Extent(71, 57), TextureColor::Red1, LOADER.GetImageN("io", 40));
    // Ok + Speichern
    AddImageButton(2, DrawPoint(160, 24), Extent(65, 57), TextureColor::Grey, LOADER.GetImageN("io", 47));
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
