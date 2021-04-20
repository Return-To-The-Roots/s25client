// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "dskMainMenu.h"
#include "CollisionDetection.h"
#include "GlobalVars.h"
#include "Loader.h"
#include "Settings.h"
#include "WindowManager.h"
#include "controls/ctrlButton.h"
#include "controls/ctrlTimer.h"
#include "desktops/dskCredits.h"
#include "desktops/dskIntro.h"
#include "desktops/dskMultiPlayer.h"
#include "desktops/dskOptions.h"
#include "desktops/dskSinglePlayer.h"
#include "desktops/dskTest.h"
#include "ingameWindows/iwMsgbox.h"
#include "ingameWindows/iwTextfile.h"

enum
{
    ID_btSingleplayer = dskMenuBase::ID_FIRST_FREE,
    ID_btMultiplayer,
    ID_btOptions,
    ID_btIntro,
    ID_btReadme,
    ID_btCredits,
    ID_btQuit,
    ID_logo,
    ID_tmrDebugData
};

dskMainMenu::dskMainMenu()
{
    RTTR_Assert(dskMenuBase::ID_FIRST_FREE <= 3);

    // "Einzelspieler"
    AddTextButton(ID_btSingleplayer, DrawPoint(115, 180), Extent(220, 22), TextureColor::Green2, _("Singleplayer"),
                  NormalFont);
    // "Mehrspieler"
    AddTextButton(ID_btMultiplayer, DrawPoint(115, 210), Extent(220, 22), TextureColor::Green2, _("Multiplayer"),
                  NormalFont);
    // "Optionen"
    AddTextButton(ID_btOptions, DrawPoint(115, 250), Extent(220, 22), TextureColor::Green2, _("Options"), NormalFont);
    // "Intro"
    AddTextButton(ID_btIntro, DrawPoint(115, 280), Extent(220, 22), TextureColor::Green2, _("Intro"), NormalFont)
      ->SetEnabled(false);
    // "ReadMe"
    AddTextButton(ID_btReadme, DrawPoint(115, 310), Extent(220, 22), TextureColor::Green2, _("Readme"), NormalFont);
    // "Credits"
    AddTextButton(ID_btCredits, DrawPoint(115, 340), Extent(220, 22), TextureColor::Green2, _("Credits"), NormalFont);
    // "Programm verlassen"
    AddTextButton(ID_btQuit, DrawPoint(115, 390), Extent(220, 22), TextureColor::Red1, _("Quit program"), NormalFont);

    AddImage(ID_logo, DrawPoint(20, 20), LOADER.GetImageN("logo", 0));

    using namespace std::chrono_literals;
    if(SETTINGS.global.submit_debug_data == 0)
        AddTimer(ID_tmrDebugData, 250ms);

    /*AddText(20, DrawPoint(50, 450), _("Font Test"), COLOR_YELLOW, FontStyle::LEFT, SmallFont);
    AddText(21, DrawPoint(50, 470), _("Font Test"), COLOR_YELLOW, FontStyle::LEFT, NormalFont);
    AddText(22, DrawPoint(50, 490), _("Font Test"), COLOR_YELLOW, FontStyle::LEFT, LargeFont);*/
    //  !\"#$%&'()*+,-./0123456789:;<=>?@abcdefghijklmnopqrstuvwxyz\\_ABCDEFGHIJKLMNOPQRSTUVWXYZÇüéâäàåçêëèïîì©ÄÅôöòûùÖÜáíóúñ
}

void dskMainMenu::Msg_Timer(const unsigned ctrl_id)
{
    GetCtrl<ctrlTimer>(ctrl_id)->Stop();
    WINDOWMANAGER.Show(std::make_unique<iwMsgbox>(
      _("Submit debug data?"),
      _("RttR now supports sending debug data. Would you like to help us improving this game by sending debug data?"),
      this, MsgboxButton::YesNo, MsgboxIcon::QuestionRed, 100));
}

void dskMainMenu::Msg_MsgBoxResult(const unsigned msgbox_id, const MsgboxResult mbr)
{
    if(msgbox_id == 100)
    {
        if(mbr == MsgboxResult::Yes)
            SETTINGS.global.submit_debug_data = 1;
        else
            SETTINGS.global.submit_debug_data = 2;

        SETTINGS.Save();
    }
}

bool dskMainMenu::Msg_LeftUp(const MouseCoords& mc)
{
    auto* txtVersion = GetCtrl<Window>(dskMenuBase::ID_txtVersion);
    if(mc.dbl_click && IsPointInRect(mc.GetPos(), txtVersion->GetBoundaryRect()))
    {
        WINDOWMANAGER.Switch(std::make_unique<dskTest>());
        return true;
    }
    return false;
}

void dskMainMenu::Msg_ButtonClick(const unsigned ctrl_id)
{
    switch(ctrl_id)
    {
        case ID_btSingleplayer: // "Single Player"
            WINDOWMANAGER.Switch(std::make_unique<dskSinglePlayer>());
            break;
        case ID_btMultiplayer: // "Multiplayer"
            WINDOWMANAGER.Switch(std::make_unique<dskMultiPlayer>());
            break;
        case ID_btOptions: // "Options"
            WINDOWMANAGER.Switch(std::make_unique<dskOptions>());
            break;
        case ID_btIntro: // "Intro"
            WINDOWMANAGER.Switch(std::make_unique<dskIntro>());
            break;
        case ID_btCredits: // "Credits"
            WINDOWMANAGER.Switch(std::make_unique<dskCredits>());
            break;
        case ID_btQuit: // "Quit"
            GLOBALVARS.notdone = false;
            break;
        case ID_btReadme: // "Readme"
            WINDOWMANAGER.ToggleWindow(std::make_unique<iwTextfile>("readme.txt", _("Readme!")));
            break;
    }
}
