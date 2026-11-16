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
    WINDOWMANAGER.setCursor(Cursor::None);
}

dskSplash::~dskSplash()
{
    WINDOWMANAGER.setCursor();
}

void dskSplash::setActive(bool activate)
{
    Desktop::setActive(activate);
    if(activate && !getCtrl<ctrlTimer>(0))
        addTimer(0, 1ms);
}

void dskSplash::msgTimer(const unsigned ctrl_id)
{
    getCtrl<ctrlTimer>(ctrl_id)->stop();
    if(ctrl_id == 0)
        addTimer(1, 1ms);
    else if(ctrl_id == 1 && !isLoaded && !isLoading)
    {
        isLoading = true;
        loadFiles();
    } else if(isLoaded)
        WINDOWMANAGER.Switch(std::make_unique<dskMainMenu>());
}

bool dskSplash::msgLeftDown(const MouseCoords& /*mc*/)
{
    if(isLoaded)
        WINDOWMANAGER.Switch(std::make_unique<dskMainMenu>());

    return true;
}

void dskSplash::loadFiles()
{
    ApplicationLoader loader(RTTRCONFIG, LOADER, LOG, SETTINGS.sound.playlist);
    if(loader.load())
    {
        isLoaded = true;
        addTimer(2, 5s);
        setFpsDisplay(true);
        if(loader.getPlaylist())
            MUSICPLAYER.setPlaylist(std::move(*loader.getPlaylist()));
        if(SETTINGS.sound.musicEnabled)
            MUSICPLAYER.play();

    } else
    {
        const auto fmt = boost::format(_("Some essential game files failed to load.\n"
                                         "Please ensure that the Settlers 2 Gold-Edition is installed in\n"
                                         "%1%"))
                         % RTTRCONFIG.ExpandPath("<RTTR_GAME>").string();

        s25util::error(fmt.str());
        VIDEODRIVER.showErrorMessage(_("Missing game files"), fmt.str());

        GLOBALVARS.notdone = false;
    }
}
