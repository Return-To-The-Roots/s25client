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
#include "GameManager.h"
#include "GlobalVars.h"
#include "Loader.h"
#include "RttrConfig.h"
#include "Settings.h"
#include "SoundManager.h"
#include "WindowManager.h"
#include "desktops/dskLobby.h"
#include "desktops/dskMainMenu.h"
#include "desktops/dskSplash.h"
#include "drivers/AudioDriverWrapper.h"
#include "drivers/VideoDriverWrapper.h"
#include "helpers/converters.h"
#include "network/GameClient.h"
#include "network/GameServer.h"
#include "ogl/glArchivItem_Bitmap.h"
#include "gameData/GameConsts.h"
#include "liblobby/LobbyClient.h"
#include "libutil/Log.h"
#include "libutil/colors.h"
#include "libutil/error.h"
#include <cstdio>

GameManager::GameManager() : skipgf_last_time(0), skipgf_last_report_gf(0), cursor_(CURSOR_HAND), cursor_next(CURSOR_HAND)
{
    ResetAverageGFPS();
}

/**
 *  Spiel starten
 */
bool GameManager::Start()
{
    // Einstellungen laden
    if(!SETTINGS.Load())
        return false;

    /// Videotreiber laden
    if(!VIDEODRIVER.LoadDriver())
    {
        s25util::error(_("Video driver couldn't be loaded!\n"));
        return false;
    }

    // Fenster erstellen
    Extent screenSize = SETTINGS.video.fullscreen ? SETTINGS.video.fullscreenSize : SETTINGS.video.windowedSize; //-V807
    if(!VIDEODRIVER.CreateScreen(screenSize.x, screenSize.y, SETTINGS.video.fullscreen))
        return false;

    /// Audiodriver laden
    if(!AUDIODRIVER.LoadDriver())
    {
        s25util::warning(_("Audio driver couldn't be loaded!\n"));
        // return false;
    }

    /// Lautstärken gleich mit setzen
    AUDIODRIVER.SetMasterEffectVolume(SETTINGS.sound.effekte_volume); //-V807
    AUDIODRIVER.SetMusicVolume(SETTINGS.sound.musik_volume);

    // Treibereinstellungen abspeichern
    SETTINGS.Save();

    LOG.write(_("\nStarting the game\n"));
    if(!ShowSplashscreen())
        return false;

    return true;
}

/**
 *  Spiel beenden.
 */
void GameManager::Stop()
{
    GAMECLIENT.Stop();
    GAMESERVER.Stop();
    LOBBYCLIENT.Stop();
    // Global Einstellungen speichern
    SETTINGS.Save();

    // Fenster beenden
    VIDEODRIVER.DestroyScreen();
}

/**
 *  Hauptschleife.
 */
bool GameManager::Run()
{
    // Nachrichtenschleife
    if(!VIDEODRIVER.Run())
        GLOBALVARS.notdone = false;

    LOBBYCLIENT.Run();

    GAMECLIENT.Run();
    GAMESERVER.Run();

    unsigned current_time = VIDEODRIVER.GetTickCount();
    const unsigned targetSkipGF = GAMECLIENT.skiptogf;
    const unsigned curGF = targetSkipGF ? GAMECLIENT.GetGFNumber() : 0;
    const bool skipping = targetSkipGF && targetSkipGF > curGF;

    // if we skip drawing write a comment every 5k gf
    if(skipping && curGF % 5000 == 0)
    {
        if(curGF > skipgf_last_report_gf)
        {
            // Elapsed time in ms
            double timeDiff = static_cast<double>(current_time - skipgf_last_time);
            LOG.write(_("jumping to gf %i, now at gf %i, time for last 5k gf: %.3f s, avg gf time %.3f ms \n")) % targetSkipGF % curGF
              % (timeDiff / 1000) % (timeDiff / (curGF - skipgf_last_time));
        } else
            LOG.write(_("jumping to gf %i, now at gf %i \n")) % targetSkipGF % curGF;
        skipgf_last_time = current_time;
        skipgf_last_report_gf = curGF;
    }
    // jump complete!
    if(targetSkipGF && targetSkipGF == curGF)
    {
        if(curGF > skipgf_last_report_gf)
        {
            double timeDiff = static_cast<double>(current_time - skipgf_last_time);
            unsigned numGFPassed = curGF - skipgf_last_report_gf;
            LOG.write(_("jump to gf %i complete, time for last %i gf: %.3f s, avg gf time %.3f ms \n")) % targetSkipGF % numGFPassed
              % (timeDiff / 1000) % (timeDiff / numGFPassed);
        }
        skipgf_last_time = 0;
        skipgf_last_report_gf = 0;
    }

    // only draw if we dont skip ahead right now
    if(!skipping)
    {
        VIDEODRIVER.ClearScreen();
        WINDOWMANAGER.Draw();
        DrawCursor();
        VIDEODRIVER.SwapBuffers();
    }
    gfCounter_.update();

    // Fenstermanager aufräumen
    if(!GLOBALVARS.notdone)
        WINDOWMANAGER.CleanUp();

    return GLOBALVARS.notdone;
}

bool GameManager::ShowSplashscreen()
{
    libsiedler2::Archiv arSplash;
    if(!LOADER.LoadFile(arSplash, RTTRCONFIG.ExpandPath("<RTTR_RTTR>/splash.bmp")))
        return false;
    glArchivItem_Bitmap* image = dynamic_cast<glArchivItem_Bitmap*>(arSplash[0]);
    if(!image)
        return false;
    arSplash.release(0);
    WINDOWMANAGER.Switch(new dskSplash(image));
    return true;
}

/**
 *  zeigt das Hauptmenü.
 */
bool GameManager::ShowMenu()
{
    GAMECLIENT.Stop();
    GAMESERVER.Stop();
    SOUNDMANAGER.StopAll();

    if(LOBBYCLIENT.IsLoggedIn())
        // Lobby zeigen
        WINDOWMANAGER.Switch(new dskLobby);
    else
        // Hauptmenü zeigen
        WINDOWMANAGER.Switch(new dskMainMenu);

    return true;
}

void GameManager::ResetAverageGFPS()
{
    gfCounter_ = FrameCounter(boost::chrono::hours::max()); // Never update
}

/**
 *  Set the cursor type
 */
void GameManager::SetCursor(CursorType cursor, bool once)
{
    cursor_next = cursor;
    if(!once)
        this->cursor_ = cursor;
}

/**
 *  Draw the cursor
 */
void GameManager::DrawCursor()
{
    unsigned resId;
    switch(cursor_next)
    {
        case CURSOR_HAND: resId = VIDEODRIVER.IsLeftDown() ? 31 : 30; break;
        case CURSOR_RM: resId = VIDEODRIVER.IsLeftDown() ? 35 : 34; break;
        default: resId = cursor_next;
    }
    if(resId)
        LOADER.GetImageN("resource", resId)->DrawFull(VIDEODRIVER.GetMousePos());

    cursor_next = cursor_;
}
