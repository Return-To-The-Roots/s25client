// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "dskDirectIP.h"
#include "Loader.h"
#include "Settings.h"
#include "WindowManager.h"
#include "dskMultiPlayer.h"
#include "ingameWindows/iwDirectIPConnect.h"
#include "ingameWindows/iwDirectIPCreate.h"
#include "ingameWindows/iwMsgbox.h"

dskDirectIP::dskDirectIP()
{
    RTTR_Assert(dskMenuBase::ID_FIRST_FREE <= 3);

    AddTextButton(3, DrawPoint(115, 180), Extent(220, 22), TextureColor::Green2, _("Create Game"), NormalFont);
    AddTextButton(4, DrawPoint(115, 210), Extent(220, 22), TextureColor::Green2, _("Join Game"), NormalFont);

    // "Zurück"
    AddTextButton(5, DrawPoint(115, 250), Extent(220, 22), TextureColor::Red1, _("Back"), NormalFont);

    AddImage(11, DrawPoint(20, 20), LOADER.GetImageN("logo", 0));
}

void dskDirectIP::Msg_ButtonClick(const unsigned ctrl_id)
{
    switch(ctrl_id)
    {
        case 3: // "Erstellen"
        {
            // Hosten geht nur ohne aktiven Proxy
            if(SETTINGS.proxy.type != ProxyType::None)
                WINDOWMANAGER.Show(std::make_unique<iwMsgbox>(
                  _("Sorry!"),
                  _("You can't create a game while a proxy server is active\nDisable the use of a proxy server first!"),
                  this, MsgboxButton::Ok, MsgboxIcon::ExclamationGreen, 1));
            else
                WINDOWMANAGER.ReplaceWindow(std::make_unique<iwDirectIPCreate>(ServerType::Direct));
        }
        break;
        case 4: // "Verbinden"
        {
            WINDOWMANAGER.ReplaceWindow(std::make_unique<iwDirectIPConnect>(ServerType::Direct));
        }
        break;
        case 5: // "Zurück"
        {
            WINDOWMANAGER.Switch(std::make_unique<dskMultiPlayer>());
        }
        break;
    }
}
