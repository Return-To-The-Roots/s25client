// Copyright (c) 2005 - 2015 Settlers Freaks (sf-team at siedler25.org)
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

///////////////////////////////////////////////////////////////////////////////
// Header
#include "defines.h" // IWYU pragma: keep
#include "GameManager.h"

#include "GlobalVars.h"
#include "Settings.h"

#include "SoundManager.h"
#include "WindowManager.h"
#include "drivers/VideoDriverWrapper.h"
#include "drivers/AudioDriverWrapper.h"

#include "LobbyClient.h"
#include "GameServer.h"
#include "GameClient.h"

#include "desktops/dskSplash.h"
#include "desktops/dskMainMenu.h"
#include "desktops/dskLobby.h"
#include "ingameWindows/iwMusicPlayer.h"
#include "ogl/glArchivItem_Font.h"
#include "Loader.h"
#include "MusicPlayer.h"
#include "gameData/GameConsts.h"
#include "helpers/win32_nanosleep.h" // IWYU pragma: keep
#include "helpers/converters.h"
#include "Log.h"
#include "libutil/src/error.h"
#include "libutil/src/colors.h"

#include <ctime>

// Include last!
#include "DebugNew.h" // IWYU pragma: keep

GameManager::GameManager() : frames(0), frame_count(0), framerate(0), frame_time(0),
    run_time(0), last_time(0), skipgf_last_time(0), skipgf_last_report_gf(0),
    cursor_(CURSOR_HAND), cursor_next(CURSOR_HAND)
{
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  Spiel starten
 *
 *  @author FloSoft
 */
bool GameManager::Start()
{
    // Einstellungen laden
    if(!SETTINGS.Load())
        return false;

    /// Videotreiber laden
    if(!VIDEODRIVER.LoadDriver())
    {
        LOG.lprintf("Video driver couldn't be loaded!\n");
        return false;
    }

    // Im Vollbildmodus überprüfen, ob Video-Mode überhaupt existiert
    if(SETTINGS.video.fullscreen) //-V807
    {
        std::vector<VideoMode> available_video_modes;
        VIDEODRIVER.ListVideoModes(available_video_modes);

        bool found = false;
        for(size_t i = 0; i < available_video_modes.size(); ++i)
        {
            if(available_video_modes[i].width == SETTINGS.video.fullscreen_width &&
                    available_video_modes[i].height == SETTINGS.video.fullscreen_height)
                found = true;
        }

        if(!found && !available_video_modes.empty())
        {
            // Nicht gefunden, erste gültige Auflösung nehmen
            SETTINGS.video.fullscreen_width = available_video_modes[0].width;
            SETTINGS.video.fullscreen_height = available_video_modes[0].height;
        }
    }

    // Fenster erstellen
    if(!VIDEODRIVER.CreateScreen(SETTINGS.video.fullscreen ? SETTINGS.video.fullscreen_width : SETTINGS.video.windowed_width,
            SETTINGS.video.fullscreen ? SETTINGS.video.fullscreen_height : SETTINGS.video.windowed_height,
            SETTINGS.video.fullscreen))
        return false;

    /// Audiodriver laden
    if(!AUDIODRIVER.LoadDriver())
    {
        LOG.lprintf("Audio driver couldn't be loaded!\n");
        //return false;
    }

    /// Lautstärken gleich mit setzen
    AUDIODRIVER.SetMasterEffectVolume(SETTINGS.sound.effekte_volume); //-V807
    AUDIODRIVER.SetMasterMusicVolume(SETTINGS.sound.musik_volume);

    // Treibereinstellungen abspeichern
    SETTINGS.Save();

    LOG.lprintf("\nStarte das Spiel\n");
    if(!StartMenu())
        return false;

    std::string playlist = iwMusicPlayer::GetFullPlaylistPath(SETTINGS.sound.playlist);
    if(MUSICPLAYER.Load(playlist))
        MUSICPLAYER.Play();

    return true;
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  Spiel beenden.
 *
 *  @author FloSoft
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

///////////////////////////////////////////////////////////////////////////////
/**
 *  Hauptschleife.
 *
 *  @author FloSoft
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

    const bool skipping = GAMECLIENT.skiptogf > GAMECLIENT.GetGFNumber();

    //only draw if we dont skip ahead right now
    if(!skipping)
    {
        const unsigned long vsync_wanted = ((GAMECLIENT.GetState() != GameClient::CS_GAME) || GAMECLIENT.IsPaused()) ? 60 : SETTINGS.video.vsync;

        // SW-VSync (mit 4% Toleranz)
        if(vsync_wanted > 1)
        {
    	    static unsigned long vsync = vsync_wanted;
    	
            // immer 10% dazu/weg bis man über der Framerate liegt
            if(vsync < 200 && 1000 * framerate < (unsigned int)(960 * vsync) )
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
                req.tv_sec  = 0;
                req.tv_nsec = goal_ticks - (current_time - last_time) * 1000 * 1000 ;

                while(nanosleep(&req, &req) == -1)
                    continue;

                current_time = VIDEODRIVER.GetTickCount();
            }
        }
		WINDOWMANAGER.Draw();
        if(GAMECLIENT.GetState() == GameClient::CS_GAME)
        {
            const int startSpeed = SPEED_GF_LENGTHS[GAMECLIENT.GetGGS().game_speed];
            const int speedStep = startSpeed / 10 - static_cast<int>(GAMECLIENT.GetGFLength()) / 10;
            if(speedStep != 0)
            {
                glArchivItem_Bitmap* runnerImg = LOADER.GetImageN("io", 164);
                const short x = VIDEODRIVER.GetScreenWidth() - 55;
                const short y = 35;
                runnerImg->Draw(x, y, 0, 0, 0, 0);
                if(speedStep != 1)
                {
                    std::string multiplier = helpers::toString(std::abs(speedStep));
                    NormalFont->Draw(x - runnerImg->getNx() + 19, y - runnerImg->getNy() + 6, multiplier, glArchivItem_Font::DF_LEFT, speedStep > 0 ? COLOR_YELLOW : COLOR_RED);
                }
            }
        }

		DrawCursor();
    } else if(GAMECLIENT.GetGFNumber() % 5000 == 0)
	{
        //if we skip drawing write a comment every 5k gf
        if(GAMECLIENT.GetGFNumber() > skipgf_last_report_gf)
		{
			if(skipgf_last_time)
				LOG.lprintf("jumping to gf %i, now at gf %i, time for last 5k gf: %.3f s, avg gf time %.3f ms \n",GAMECLIENT.skiptogf, GAMECLIENT.GetGFNumber(),double (VIDEODRIVER.GetTickCount()-skipgf_last_time)/1000,double (VIDEODRIVER.GetTickCount()-skipgf_last_time)/5000);
			else
				LOG.lprintf("jumping to gf %i, now at gf %i \n",GAMECLIENT.skiptogf, GAMECLIENT.GetGFNumber());
			skipgf_last_time=VIDEODRIVER.GetTickCount();
			skipgf_last_report_gf=GAMECLIENT.GetGFNumber();
		}
	}
	//jump complete!
	if(GAMECLIENT.skiptogf && GAMECLIENT.skiptogf == GAMECLIENT.GetGFNumber())
	{
		if(skipgf_last_time)
		{
			if((GAMECLIENT.skiptogf-1)%5000>0)
				LOG.lprintf("jump to gf %i complete, time for last %i gf: %.3f s, avg gf time %.3f ms \n",GAMECLIENT.skiptogf, (GAMECLIENT.skiptogf-1)%5000+1,double (VIDEODRIVER.GetTickCount()-skipgf_last_time)/1000,double (VIDEODRIVER.GetTickCount()-skipgf_last_time)/((GAMECLIENT.skiptogf-1)%5000));
			else
				LOG.lprintf("jump to gf %i complete \n",GAMECLIENT.skiptogf);
		}
		skipgf_last_time=0;
		skipgf_last_report_gf=0;
	}
    last_time = current_time;
    

    // Framerate berechnen
    if(current_time - frame_time >= 1000)
    {
        // weitere Sekunde vergangen
        ++run_time;

        // Gesamtzahl der gezeichneten Frames erhöhen
        frame_count += frames;

        // normale Framerate berechnen
        framerate = frames;
        frames = 0;

        frame_time = current_time;
    }

    // und zeichnen
	//only draw if we dont skip ahead right now
	if(!skipping)
	{
		char frame_str[64];
		sprintf(frame_str, "%u fps", framerate);

		SmallFont->Draw( VIDEODRIVER.GetScreenWidth(), 0, frame_str, glArchivItem_Font::DF_RIGHT, COLOR_YELLOW);

		// Zeichenpuffer wechseln
		VIDEODRIVER.SwapBuffers();
	}
    ++frames;

    // Fenstermanager aufräumen
    if(!GLOBALVARS.notdone)
        WINDOWMANAGER.CleanUp();

    return GLOBALVARS.notdone;
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  Startet und lädt das Menü.
 *
 *  @author FloSoft
 */
bool GameManager::StartMenu()
{
    // generelle Daten laden
    if(!LOADER.LoadFilesAtStart())
    {
        error("Einige Dateien konnten nicht geladen werden.\n"
              "Stellen Sie sicher, dass die Siedler 2 Gold-Edition im gleichen \n"
              "Verzeichnis wie Return to the Roots installiert ist.");

        error("Some files failed to load.\n"
              "Please ensure that the Settlers 2 Gold-Edition is installed \n"
              "in the same directory as Return to the Roots.");

        return false;
    }

    // Splash-Screen anzeigen
    WINDOWMANAGER.Switch(new dskSplash);

    return true;
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  zeigt das Hauptmenü.
 *
 *  @author FloSoft
 */
bool GameManager::ShowMenu()
{
    GAMECLIENT.Stop();
    GAMESERVER.Stop();
    SOUNDMANAGER.StopAll();

    GAMECLIENT.SetInterface(NULL);

    if(LOBBYCLIENT.LoggedIn())
        // Lobby zeigen
        WINDOWMANAGER.Switch(new dskLobby);
    else
        // Hauptmenü zeigen
        WINDOWMANAGER.Switch(new dskMainMenu);

    return true;
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  Set the cursor type
 *
 *  @author Divan
 */
void GameManager::SetCursor(CursorType cursor, bool once)
{
    cursor_next = cursor;
    if(!once) this->cursor_ = cursor;
    return;
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  Draw the cursor
 *
 *  @author Divan
 */
void GameManager::DrawCursor()
{
    // Mauszeiger zeichnen
    switch(cursor_next)
    {
        case CURSOR_HAND:
        {
            if(VIDEODRIVER.IsLeftDown())
                LOADER.GetImageN("resource", 31)->Draw(VIDEODRIVER.GetMouseX(), VIDEODRIVER.GetMouseY(), 0, 0, 0, 0, 0, 0);
            else
                LOADER.GetImageN("resource", 30)->Draw(VIDEODRIVER.GetMouseX(), VIDEODRIVER.GetMouseY(), 0, 0, 0, 0, 0, 0);
        } break;
        case CURSOR_SCROLL:
        case CURSOR_MOON:
        case CURSOR_RM:
        case CURSOR_RM_PRESSED:
        {
            LOADER.GetImageN("resource", cursor_next)->Draw(VIDEODRIVER.GetMouseX(), VIDEODRIVER.GetMouseY(), 0, 0, 0, 0, 0, 0);
        } break;
        case CURSOR_NONE:
        default:
        {}
    }

    cursor_next = cursor_;
    return;
}
