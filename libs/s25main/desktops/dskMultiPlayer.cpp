// Copyright (c) 2005 - 2017 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

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
    AddTextButton(3, DrawPoint(115, 180), Extent(220, 22), TextureColor::Green2, _("Internet Lobby"), NormalFont);
    // "Netzwerk / LAN"
    AddTextButton(4, DrawPoint(115, 210), Extent(220, 22), TextureColor::Green2, _("Network/LAN"), NormalFont);
    // "Direkte IP"
    AddTextButton(5, DrawPoint(115, 250), Extent(220, 22), TextureColor::Green2, _("Direct IP"), NormalFont);
    // "Zurück"
    AddTextButton(6, DrawPoint(115, 290), Extent(220, 22), TextureColor::Red1, _("Back"), NormalFont);

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
