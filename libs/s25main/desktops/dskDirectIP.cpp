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

namespace {
enum
{
    ID_btCreateGame = dskMenuBase::ID_FIRST_FREE,
    ID_btJoinGame,
    ID_btBack,
    ID_imgLogo
};
}

dskDirectIP::dskDirectIP()
{
    AddTextButton(ID_btCreateGame, DrawPoint(115, 180), Extent(220, 22), TextureColor::Green2, _("Create Game"),
                  NormalFont);
    AddTextButton(ID_btJoinGame, DrawPoint(115, 210), Extent(220, 22), TextureColor::Green2, _("Join Game"),
                  NormalFont);

    AddTextButton(ID_btBack, DrawPoint(115, 250), Extent(220, 22), TextureColor::Red1, _("Back"), NormalFont);

    AddImage(ID_imgLogo, DrawPoint(20, 20), LOADER.GetImageN("logo", 0));
}

void dskDirectIP::Msg_ButtonClick(const unsigned ctrl_id)
{
    switch(ctrl_id)
    {
        case ID_btCreateGame:
            // Hosten geht nur ohne aktiven Proxy
            if(SETTINGS.proxy.type != ProxyType::None)
            {
                WINDOWMANAGER.Show(std::make_unique<iwMsgbox>(
                  _("Sorry!"),
                  _("You can't create a game while a proxy server is active\nDisable the use of a proxy server first!"),
                  this, MsgboxButton::Ok, MsgboxIcon::ExclamationGreen, 1));
            } else
                WINDOWMANAGER.ReplaceWindow(std::make_unique<iwDirectIPCreate>(ServerType::Direct));
            break;
        case ID_btJoinGame: WINDOWMANAGER.ReplaceWindow(std::make_unique<iwDirectIPConnect>(ServerType::Direct)); break;
        case ID_btBack: WINDOWMANAGER.Switch(std::make_unique<dskMultiPlayer>()); break;
    }
}
