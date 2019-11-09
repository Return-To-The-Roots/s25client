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
#include "RTTR_Assert.h"
#include "RttrConfig.h"
#include "Settings.h"
#include "SoundManager.h"
#include "WindowManager.h"
#include "desktops/dskLobby.h"
#include "desktops/dskMainMenu.h"
#include "desktops/dskSplash.h"
#include "drivers/AudioDriverWrapper.h"
#include "drivers/VideoDriverWrapper.h"
#include "network/GameClient.h"
#include "network/GameServer.h"
#include "ogl/glArchivItem_Bitmap.h"
#include "liblobby/LobbyClient.h"
#include "s25util//dynamicUniqueCast.h"
#include "s25util/Log.h"
#include "s25util/error.h"

GameManager::GameManager() : cursor_(CURSOR_HAND)
{
    ResetAverageGFPS();
}

/**
 *  Spiel starten
 */
bool GameManager::Start()
{
    // Einstellungen laden
    SETTINGS.Load();

    /// Videotreiber laden
    if(!VIDEODRIVER.LoadDriver(SETTINGS.driver.video))
    {
        s25util::error(_("Video driver couldn't be loaded!\n"));
        return false;
    }

    // Fenster erstellen
    const auto screenSize = SETTINGS.video.fullscreen ? SETTINGS.video.fullscreenSize : SETTINGS.video.windowedSize; //-V807
    if(!VIDEODRIVER.CreateScreen(screenSize, SETTINGS.video.fullscreen))
        return false;
    VIDEODRIVER.setTargetFramerate(SETTINGS.video.vsync);
    VIDEODRIVER.SetMouseWarping(SETTINGS.global.smartCursor);

    /// Audiodriver laden
    if(!AUDIODRIVER.LoadDriver(SETTINGS.driver.audio))
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
    return ShowSplashscreen();
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

    // Get this before the run so we know if we are currently skipping
    const unsigned targetSkipGF = GAMECLIENT.skiptogf;
    GAMECLIENT.Run();
    GAMESERVER.Run();

    if(targetSkipGF)
    {
        // if we skip drawing write a comment every 5k gf
        unsigned current_time = VIDEODRIVER.GetTickCount();
        const unsigned curGF = GAMECLIENT.GetGFNumber();
        if(targetSkipGF > curGF)
        {
            if(curGF % 5000 == 0)
            {
                if(lastSkipReport)
                {
                    // Elapsed time in ms
                    const auto timeDiff = static_cast<double>(current_time - lastSkipReport->time);
                    const unsigned numGFPassed = curGF - lastSkipReport->gf;
                    LOG.write(_("jumping to gf %i, now at gf %i, time for last 5k gf: %.3f s, avg gf time %.3f ms \n")) % targetSkipGF
                      % curGF % (timeDiff / 1000) % (timeDiff / numGFPassed);
                } else
                    LOG.write(_("jumping to gf %i, now at gf %i \n")) % targetSkipGF % curGF;
                lastSkipReport = SkipReport{current_time, curGF};
            } else if(!lastSkipReport)
                lastSkipReport = SkipReport{current_time, curGF};
        } else
        {
            // Jump just completed
            RTTR_Assert(!GAMECLIENT.skiptogf);
            if(lastSkipReport)
            {
                const auto timeDiff = static_cast<double>(current_time - lastSkipReport->time);
                const unsigned numGFPassed = curGF - lastSkipReport->gf;
                LOG.write(_("jump to gf %i complete, time for last %i gf: %.3f s, avg gf time %.3f ms \n")) % targetSkipGF % numGFPassed
                  % (timeDiff / 1000) % (timeDiff / numGFPassed);
                lastSkipReport.reset();
            } else
            {
                LOG.write(_("jump to gf %1% complete\n")) % targetSkipGF;
            }
        }
    } else
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
    auto image = libutil::dynamicUniqueCast<glArchivItem_Bitmap>(arSplash.release(0));
    if(!image)
        return false;
    WINDOWMANAGER.Switch(std::make_unique<dskSplash>(std::move(image)));
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
        WINDOWMANAGER.Switch(std::make_unique<dskLobby>());
    else
        // Hauptmenü zeigen
        WINDOWMANAGER.Switch(std::make_unique<dskMainMenu>());

    return true;
}

void GameManager::ResetAverageGFPS()
{
    gfCounter_ = FrameCounter(FrameCounter::clock::duration::max()); // Never update
}

/**
 *  Set the cursor type
 */
void GameManager::SetCursor(CursorType cursor)
{
    cursor_ = cursor;
}

/**
 *  Draw the cursor
 */
void GameManager::DrawCursor()
{
    unsigned resId;
    switch(cursor_)
    {
        case CURSOR_HAND: resId = VIDEODRIVER.IsLeftDown() ? 31 : 30; break;
        case CURSOR_RM: resId = VIDEODRIVER.IsLeftDown() ? 35 : 34; break;
        default: resId = cursor_;
    }
    if(resId)
        LOADER.GetImageN("resource", resId)->DrawFull(VIDEODRIVER.GetMousePos());
}
