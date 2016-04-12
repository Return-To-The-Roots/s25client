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
#include "defines.h" // IWYU pragma: keep
#include <build_version.h>
#include "dskDirectIP.h"

#include "WindowManager.h"
#include "Loader.h"
#include "Settings.h"

#include "dskMultiPlayer.h"
#include "ogl/glArchivItem_Font.h"
#include "ingameWindows/iwMsgbox.h"
#include "ingameWindows/iwDirectIPCreate.h"
#include "ingameWindows/iwDirectIPConnect.h"
#include "libutil/src/colors.h"

// Include last!
#include "DebugNew.h" // IWYU pragma: keep

dskDirectIP::dskDirectIP() : Desktop(LOADER.GetImageN("menu", 0))
{
    // Version
    AddVarText(0, 0, 600, _("Return To The Roots - v%s-%s"), COLOR_YELLOW, 0 | glArchivItem_Font::DF_BOTTOM, NormalFont, 2, GetWindowVersion(), GetWindowRevisionShort());

    // URL
    AddText(1, 400, 600, _("http://www.siedler25.org"), COLOR_GREEN, glArchivItem_Font::DF_CENTER | glArchivItem_Font::DF_BOTTOM, NormalFont);

    // Copyright
    AddVarText(2, 800, 600, _("© 2005 - %s Settlers Freaks"), COLOR_YELLOW, glArchivItem_Font::DF_RIGHT | glArchivItem_Font::DF_BOTTOM, NormalFont, 1, GetCurrentYear());

    AddTextButton(3, 115, 180, 220, 22, TC_GREEN2, _("Create Game"), NormalFont);
    AddTextButton(4, 115, 210, 220, 22, TC_GREEN2, _("Join Game"), NormalFont);

    // "Zurück"
    AddTextButton(5, 115, 250, 220, 22, TC_RED1, _("Back"), NormalFont);

    AddImage(11, 20, 20, LOADER.GetImageN("logo", 0));
}


void dskDirectIP::Msg_ButtonClick(const unsigned int ctrl_id)
{
    switch(ctrl_id)
    {
        case 3: // "Erstellen"
        {
            // Hosten geht nur ohne aktiven Proxy
            if(SETTINGS.proxy.typ != 0)
                WINDOWMANAGER.Show(new iwMsgbox(_("Sorry!"), _("You can't create a game while a proxy server is active\nDisable the use of a proxy server first!"), this, MSB_OK, MSB_EXCLAMATIONGREEN, 1));
            else
                WINDOWMANAGER.Show(new iwDirectIPCreate(ServerType::DIRECT));
        } break;
        case 4: // "Verbinden"
        {
            WINDOWMANAGER.Show(new iwDirectIPConnect(ServerType::DIRECT));
        } break;
        case 5: // "Zurück"
        {
            WINDOWMANAGER.Switch(new dskMultiPlayer);
        } break;
    }
}
