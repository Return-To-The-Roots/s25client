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
#include "dskSplash.h"

#include "GameManager.h"
#include "GlobalVars.h"
#include "Loader.h"
#include "MusicPlayer.h"
#include "Settings.h"
#include "WindowManager.h"
#include "controls/ctrlTimer.h"
#include "dskMainMenu.h"
#include "ingameWindows/iwMusicPlayer.h"
#include "ogl/glArchivItem_Bitmap.h"
#include "libutil/error.h"

dskSplash::dskSplash(glArchivItem_Bitmap* splashImg) : Desktop(splashImg), isLoading(false), isLoaded(false)
{
    background->setInterpolateTexture(false);
    GAMEMANAGER.SetCursor(CURSOR_NONE);
}

dskSplash::~dskSplash()
{
    // We took ownership!
    delete background;
    GAMEMANAGER.SetCursor();
}

void dskSplash::SetActive(bool activate)
{
    Desktop::SetActive(activate);
    if(activate && !GetCtrl<ctrlTimer>(0))
        AddTimer(0, 1);
}

void dskSplash::Msg_Timer(const unsigned ctrl_id)
{
    GetCtrl<ctrlTimer>(ctrl_id)->Stop();
    if(ctrl_id == 0)
        AddTimer(1, 1);
    else if(ctrl_id == 1 && !isLoaded && !isLoading)
    {
        isLoading = true;
        LoadFiles();
    } else if(isLoaded)
        WINDOWMANAGER.Switch(new dskMainMenu);
}

bool dskSplash::Msg_LeftDown(const MouseCoords& /*mc*/)
{
    if(isLoaded)
        WINDOWMANAGER.Switch(new dskMainMenu);

    return true;
}

void dskSplash::LoadFiles()
{
    LOADER.ClearOverrideFolders();
    LOADER.AddOverrideFolder("<RTTR_RTTR>/LSTS");
    LOADER.AddOverrideFolder("<RTTR_USERDATA>/LSTS");
    if(LOADER.LoadFilesAtStart())
    {
        isLoaded = true;
        AddTimer(2, 5000);
        SetFpsDisplay(true);
        MUSICPLAYER.Load(iwMusicPlayer::GetFullPlaylistPath(SETTINGS.sound.playlist));
        if(SETTINGS.sound.musik)
            MUSICPLAYER.Play();

    } else
    {
        s25util::error(_("Some files failed to load.\n"
                         "Please ensure that the Settlers 2 Gold-Edition is installed \n"
                         "in the same directory as Return to the Roots."));
        GLOBALVARS.notdone = false;
    }
}
