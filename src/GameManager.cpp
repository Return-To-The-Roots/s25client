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
#include "helpers/win32_nanosleep.h" // IWYU pragma: keep
#include "network/GameClient.h"
#include "network/GameServer.h"
#include "ogl/glArchivItem_Bitmap.h"
#include "gameData/GameConsts.h"
#include "liblobby/LobbyClient.h"
#include "libutil/Log.h"
#include "libutil/colors.h"
#include "libutil/error.h"
#include <boost/math/special_functions/round.hpp>
#include <cstdio>
#include <ctime>

GameManager::GameManager()
    : frames(0), frame_count(0), framerate(0), frame_time(0), run_time(0), last_time(0), skipgf_last_time(0), skipgf_last_report_gf(0),
      cursor_(CURSOR_HAND), cursor_next(CURSOR_HAND)
{}

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

    const bool skipping = GAMECLIENT.skiptogf && GAMECLIENT.skiptogf > GAMECLIENT.GetGFNumber();

    // only draw if we dont skip ahead right now
    if(!skipping)
    {
        const unsigned long vsync_wanted =
          ((GAMECLIENT.GetState() != GameClient::CS_GAME) || GAMECLIENT.IsPaused()) ? 60 : SETTINGS.video.vsync;

        // SW-VSync (mit 4% Toleranz)
        if(vsync_wanted > 1)
        {
            static unsigned long vsync = vsync_wanted;

            // immer 10% dazu/weg bis man über der Framerate liegt
            if(vsync < 200 && 1000 * framerate < (unsigned)(960 * vsync))
                vsync = (1100 * vsync) / 1000;
            else if(vsync > vsync_wanted)
                vsync = (900 * vsync) / 1000;
            else
                vsync = vsync_wanted;

            unsigned long goal_ticks = 960 * 1000 * 1000 / vsync;
#ifdef _WIN32
            if(goal_ticks < 13 * 1000 * 1000) // timer resolutions < 13ms do not work for windows correctly
                goal_ticks = 0;
#endif // !_WIN32

            if(goal_ticks > 0 && (current_time - last_time) * 1000 * 1000 < goal_ticks && (current_time >= last_time))
            {
                struct timespec req;
                req.tv_sec = 0;
                req.tv_nsec = goal_ticks - (current_time - last_time) * 1000 * 1000;

                while(nanosleep(&req, &req) == -1)
                    continue;

                current_time = VIDEODRIVER.GetTickCount();
            }
        }

        VIDEODRIVER.ClearScreen();
        WINDOWMANAGER.Draw();

        DrawCursor();
    } else if(GAMECLIENT.GetGFNumber() % 5000 == 0)
    {
        // if we skip drawing write a comment every 5k gf
        if(GAMECLIENT.GetGFNumber() > skipgf_last_report_gf)
        {
            if(skipgf_last_time)
                LOG.write(_("jumping to gf %i, now at gf %i, time for last 5k gf: %.3f s, avg gf time %.3f ms \n")) % GAMECLIENT.skiptogf
                  % GAMECLIENT.GetGFNumber() % (static_cast<double>(VIDEODRIVER.GetTickCount() - skipgf_last_time) / 1000)
                  % (static_cast<double>(VIDEODRIVER.GetTickCount() - skipgf_last_time) / 5000);
            else
                LOG.write(_("jumping to gf %i, now at gf %i \n")) % GAMECLIENT.skiptogf % GAMECLIENT.GetGFNumber();
            skipgf_last_time = VIDEODRIVER.GetTickCount();
            skipgf_last_report_gf = GAMECLIENT.GetGFNumber();
        }
    }
    // jump complete!
    if(GAMECLIENT.skiptogf && GAMECLIENT.skiptogf == GAMECLIENT.GetGFNumber())
    {
        if(skipgf_last_time)
        {
            if((GAMECLIENT.skiptogf - 1) % 5000 > 0)
                LOG.write(_("jump to gf %i complete, time for last %i gf: %.3f s, avg gf time %.3f ms \n")) % GAMECLIENT.skiptogf
                  % ((GAMECLIENT.skiptogf - 1) % 5000 + 1) % (static_cast<double>(VIDEODRIVER.GetTickCount() - skipgf_last_time) / 1000)
                  % (static_cast<double>(VIDEODRIVER.GetTickCount() - skipgf_last_time) / ((GAMECLIENT.skiptogf - 1) % 5000));
            else
                LOG.write(_("jump to gf %i complete \n")) % GAMECLIENT.skiptogf;
        }
        skipgf_last_time = 0;
        skipgf_last_report_gf = 0;
    }
    last_time = current_time;

    // Framerate berechnen
    if(current_time - frame_time >= 1000)
    {
        unsigned msSinceLastFrameCalculation = current_time - frame_time;

        run_time += msSinceLastFrameCalculation / 1000;

        // Gesamtzahl der gezeichneten Frames erhöhen
        frame_count += frames;

        // normale Framerate berechnen
        framerate = static_cast<unsigned>(boost::math::iround(frames * 1000. / msSinceLastFrameCalculation));
        frames = 0;

        frame_time = current_time;
        WINDOWMANAGER.UpdateFps(framerate);
    }

    // und zeichnen
    // only draw if we dont skip ahead right now
    if(!skipping)
    {
        // Zeichenpuffer wechseln
        VIDEODRIVER.SwapBuffers();
    }
    ++frames;

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
