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

#include "dskMultiPlayer.h"

#include "Loader.h"
#include "WindowManager.h"

#include "desktops/dskDirectIP.h"
#include "desktops/dskLAN.h"
#include "desktops/dskMainMenu.h"
#include "ingameWindows/iwLobbyConnect.h"

/** @class dskMultiPlayer
 *
 *  Klasse des Multispieler Desktops.
 */

dskMultiPlayer::dskMultiPlayer()
{
    RTTR_Assert(dskMenuBase::ID_FIRST_FREE <= 3);

    // "Internet - Lobby"
    AddTextButton(3, DrawPoint(115, 180), Extent(220, 22), TC_GREEN2, _("Internet Lobby"), NormalFont);
    // "Netzwerk / LAN"
    AddTextButton(4, DrawPoint(115, 210), Extent(220, 22), TC_GREEN2, _("Network/LAN"), NormalFont);
    // "Direkte IP"
    AddTextButton(5, DrawPoint(115, 250), Extent(220, 22), TC_GREEN2, _("Direct IP"), NormalFont);
    // "Zurück"
    AddTextButton(6, DrawPoint(115, 290), Extent(220, 22), TC_RED1, _("Back"), NormalFont);

    AddImage(11, DrawPoint(20, 20), LOADER.GetImageN("logo", 0));
}

void dskMultiPlayer::Msg_ButtonClick(const unsigned ctrl_id)
{
    switch(ctrl_id)
    {
        case 3: // Lobby
            WINDOWMANAGER.Show(std::make_unique<iwLobbyConnect>(), true);
            break;
        case 4: // Local Area Network
            WINDOWMANAGER.Switch(std::make_unique<dskLAN>());
            break;
        case 5: // Direct IP
            WINDOWMANAGER.Switch(std::make_unique<dskDirectIP>());
            break;
        case 6: // Zurück
            WINDOWMANAGER.Switch(std::make_unique<dskMainMenu>());
            break;
    }
}
