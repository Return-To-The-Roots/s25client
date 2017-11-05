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

#include "rttrDefines.h" // IWYU pragma: keep
#include "dskDirectIP.h"

#include "Loader.h"
#include "Settings.h"
#include "WindowManager.h"

#include "dskMultiPlayer.h"
#include "ingameWindows/iwDirectIPConnect.h"
#include "ingameWindows/iwDirectIPCreate.h"
#include "ingameWindows/iwMsgbox.h"
#include "libutil/colors.h"

dskDirectIP::dskDirectIP()
{
    RTTR_Assert(dskMenuBase::ID_FIRST_FREE <= 3);

    AddTextButton(3, DrawPoint(115, 180), Extent(220, 22), TC_GREEN2, _("Create Game"), NormalFont);
    AddTextButton(4, DrawPoint(115, 210), Extent(220, 22), TC_GREEN2, _("Join Game"), NormalFont);

    // "Zurück"
    AddTextButton(5, DrawPoint(115, 250), Extent(220, 22), TC_RED1, _("Back"), NormalFont);

    AddImage(11, DrawPoint(20, 20), LOADER.GetImageN("logo", 0));
}

void dskDirectIP::Msg_ButtonClick(const unsigned ctrl_id)
{
    switch(ctrl_id)
    {
        case 3: // "Erstellen"
        {
            // Hosten geht nur ohne aktiven Proxy
            if(SETTINGS.proxy.typ != 0)
                WINDOWMANAGER.Show(new iwMsgbox(
                  _("Sorry!"), _("You can't create a game while a proxy server is active\nDisable the use of a proxy server first!"), this,
                  MSB_OK, MSB_EXCLAMATIONGREEN, 1));
            else
                WINDOWMANAGER.Show(new iwDirectIPCreate(ServerType::DIRECT));
        }
        break;
        case 4: // "Verbinden"
        {
            WINDOWMANAGER.Show(new iwDirectIPConnect(ServerType::DIRECT));
        }
        break;
        case 5: // "Zurück"
        {
            WINDOWMANAGER.Switch(new dskMultiPlayer);
        }
        break;
    }
}
