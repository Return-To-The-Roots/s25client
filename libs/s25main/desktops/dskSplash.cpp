// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "dskSplash.h"
#include "GlobalVars.h"
#include "Loader.h"
#include "MusicPlayer.h"
#include "Playlist.h"
#include "RttrConfig.h"
#include "Settings.h"
#include "WindowManager.h"
#include "controls/ctrlTimer.h"
#include "dskMainMenu.h"
#include "helpers/format.hpp"
#include "ingameWindows/iwMusicPlayer.h"
#include "mygettext/mygettext.h"
#include "ogl/glArchivItem_Bitmap.h"
#include "gameData/ApplicationLoader.h"
#include "s25util/Log.h"
#include "s25util/error.h"
#include <drivers/VideoDriverWrapper.h>

using namespace std::chrono_literals;

dskSplash::dskSplash(std::unique_ptr<glArchivItem_Bitmap> splashImg)
    : Desktop(splashImg.get()), splashImg(std::move(splashImg)), isLoading(false), isLoaded(false)
{
    background->setInterpolateTexture(false);
    WINDOWMANAGER.SetCursor(Cursor::None);
}

dskSplash::~dskSplash()
{
    WINDOWMANAGER.SetCursor();
}

void dskSplash::SetActive(bool activate)
{
    Desktop::SetActive(activate);
    if(activate && !GetCtrl<ctrlTimer>(0))
        AddTimer(0, 1ms);
}

void dskSplash::Msg_Timer(const unsigned ctrl_id)
{
    GetCtrl<ctrlTimer>(ctrl_id)->Stop();
    if(ctrl_id == 0)
        AddTimer(1, 1ms);
    else if(ctrl_id == 1 && !isLoaded && !isLoading)
    {
        isLoading = true;
        LoadFiles();
    } else if(isLoaded)
        WINDOWMANAGER.Switch(std::make_unique<dskMainMenu>());
}

bool dskSplash::Msg_LeftDown(const MouseCoords& /*mc*/)
{
    if(isLoaded)
        WINDOWMANAGER.Switch(std::make_unique<dskMainMenu>());

    return true;
}

void dskSplash::LoadFiles()
{
    ApplicationLoader loader(RTTRCONFIG, LOADER, LOG, SETTINGS.sound.playlist);
    if(loader.load())
    {
        isLoaded = true;
        AddTimer(2, 5s);
        SetFpsDisplay(true);
        if(loader.getPlaylist())
            MUSICPLAYER.SetPlaylist(std::move(*loader.getPlaylist()));
        if(SETTINGS.sound.musicEnabled)
            MUSICPLAYER.Play();

    } else
    {
        s25util::error(_("Some essential game files failed to load.\n"
                         "Please ensure that the Settlers 2 Gold-Edition is installed \n"
                         "in the same directory as Return to the Roots."));
        VIDEODRIVER.ShowErrorMessage("Please install Settlers II game files into",
                                     RTTRCONFIG.ExpandPath("<RTTR_GAME>").string().c_str());
        GLOBALVARS.notdone = false;
    }
}
