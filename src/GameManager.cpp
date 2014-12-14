// $Id: GameManager.cpp 9543 2014-12-14 12:04:38Z marcus $
//
// Copyright (c) 2005 - 2011 Settlers Freaks (sf-team at siedler25.org)
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
#include "main.h"
#include "GameManager.h"

#include "GlobalVars.h"
#include "Settings.h"

#include "SoundManager.h"
#include "WindowManager.h"
#include "VideoDriverWrapper.h"
#include "AudioDriverWrapper.h"

#include "LobbyClient.h"
#include "GameServer.h"
#include "GameClient.h"

#include "dskSplash.h"
#include "dskMainMenu.h"
#include "dskLobby.h"
#include "iwMusicPlayer.h"

#include "MusicPlayer.h"

///////////////////////////////////////////////////////////////////////////////
// Makros / Defines
#if defined _WIN32 && defined _DEBUG && defined _MSC_VER
#define new new(_NORMAL_BLOCK, THIS_FILE, __LINE__)
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

///////////////////////////////////////////////////////////////////////////////
/**
 *  Konstruktor von @pGameManager.
 *
 *  @author OLiver
 */
GameManager::GameManager(void) : frames(0), frame_count(0), framerate(0), frame_time(0), run_time(0), last_time(0), cursor(CURSOR_HAND), cursor_next(CURSOR_HAND)
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
    if(!VideoDriverWrapper::inst().LoadDriver())
    {
        LOG.lprintf("Video driver couldn't be loaded!\n");
        return false;
    }

    // Im Vollbildmodus überprüfen, ob Video-Mode überhaupt existiert
    if(SETTINGS.video.fullscreen)
    {
        std::vector<VideoDriver::VideoMode> available_video_modes;
        VideoDriverWrapper::inst().ListVideoModes(available_video_modes);

        bool found = false;
        for(size_t i = 0; i < available_video_modes.size(); ++i)
        {
            if(available_video_modes[i].width == SETTINGS.video.fullscreen_width &&
                    available_video_modes[i].height == SETTINGS.video.fullscreen_height)
                found = true;
        }

        if(!found && available_video_modes.size())
        {
            // Nicht gefunden, erste gültige Auflösung nehmen
            SETTINGS.video.fullscreen_width = available_video_modes[0].width;
            SETTINGS.video.fullscreen_height = available_video_modes[0].height;
        }
    }

    // Fenster erstellen
    if(!VideoDriverWrapper::inst().CreateScreen(SETTINGS.video.fullscreen ? SETTINGS.video.fullscreen_width : SETTINGS.video.windowed_width,
            SETTINGS.video.fullscreen ? SETTINGS.video.fullscreen_height : SETTINGS.video.windowed_height,
            SETTINGS.video.fullscreen))
        return false;

    /// Audiodriver laden
    if(!AudioDriverWrapper::inst().LoadDriver())
    {
        LOG.lprintf("Audio driver couldn't be loaded!\n");
        //return false;
    }

    /// Lautstärken gleich mit setzen
    AudioDriverWrapper::inst().SetMasterEffectVolume(SETTINGS.sound.effekte_volume);
    AudioDriverWrapper::inst().SetMasterMusicVolume(SETTINGS.sound.musik_volume);

    // Treibereinstellungen abspeichern
    SETTINGS.Save();

    LOG.lprintf("\nStarte das Spiel\n");
    if(!StartMenu())
        return false;

    std::string playlist = iwMusicPlayer::GetFullPlaylistPath(SETTINGS.sound.playlist);
    if(MusicPlayer::inst().Load(playlist))
        MusicPlayer::inst().Play();

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
    // Global Einstellungen speichern
    SETTINGS.Save();

    // Fenster beenden
    VideoDriverWrapper::inst().DestroyScreen();
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
    if(!VideoDriverWrapper::inst().Run())
        GLOBALVARS.notdone = false;

    LOBBYCLIENT.Run();

    GAMECLIENT.Run();
    GAMESERVER.Run();

    unsigned int current_time = VideoDriverWrapper::inst().GetTickCount();

    // SW-VSync (mit 4% Toleranz)
    if(SETTINGS.video.vsync > 1)
    {
        static unsigned long vsync = SETTINGS.video.vsync;

        // immer 10% dazu/weg bis man über der Framerate liegt
        if(vsync < 200 && 1000 * framerate < (unsigned int)(960 * vsync) )
            vsync = (1100 * vsync) / 1000;
        else if(vsync > SETTINGS.video.vsync)
            vsync = (900 * vsync) / 1000;
        else
            vsync = SETTINGS.video.vsync;

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

            current_time = VideoDriverWrapper::inst().GetTickCount();
        }
    }
	//only draw if we dont skip ahead right now
	if(!GAMECLIENT.skiptogf || GAMECLIENT.skiptogf < GAMECLIENT.GetGFNumber())
	{
		WindowManager::inst().Draw();
		if ((GAMECLIENT.GetState() == GameClient::CS_GAME) && (GAMECLIENT.GetGFLength() < 30))
		{
			LOADER.GetImageN("io", 164)->Draw(VideoDriverWrapper::inst().GetScreenWidth() - 55, 35, 0, 0, 0, 0);
		}

		DrawCursor();
	}

	//if we skip drawing write a comment every 5k gf
	if(GAMECLIENT.skiptogf && GAMECLIENT.skiptogf > GAMECLIENT.GetGFNumber() && GAMECLIENT.GetGFNumber()%5000==0)
	{
		if(GAMECLIENT.GetGFNumber() > skipgf_last_report_gf)
		{
			if(skipgf_last_time)
				LOG.lprintf("jumping to gf %i, now at gf %i, time for last 5k gf: %.3f s, avg gf time %.3f ms \n",GAMECLIENT.skiptogf, GAMECLIENT.GetGFNumber(),double (VideoDriverWrapper::inst().GetTickCount()-skipgf_last_time)/1000,double (VideoDriverWrapper::inst().GetTickCount()-skipgf_last_time)/5000);
			else
				LOG.lprintf("jumping to gf %i, now at gf %i \n",GAMECLIENT.skiptogf, GAMECLIENT.GetGFNumber());
			skipgf_last_time=VideoDriverWrapper::inst().GetTickCount();
			skipgf_last_report_gf=GAMECLIENT.GetGFNumber();
		}
	}
	//jump complete!
	if(GAMECLIENT.skiptogf && GAMECLIENT.skiptogf == GAMECLIENT.GetGFNumber())
	{
		if(skipgf_last_time)
		{
			if((GAMECLIENT.skiptogf-1)%5000>0)
				LOG.lprintf("jump to gf %i complete, time for last %i gf: %.3f s, avg gf time %.3f ms \n",GAMECLIENT.skiptogf, (GAMECLIENT.skiptogf-1)%5000+1,double (VideoDriverWrapper::inst().GetTickCount()-skipgf_last_time)/1000,double (VideoDriverWrapper::inst().GetTickCount()-skipgf_last_time)/((GAMECLIENT.skiptogf-1)%5000));
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
	if(!GAMECLIENT.skiptogf || GAMECLIENT.skiptogf < GAMECLIENT.GetGFNumber())
	{
		char frame_str[64];
		sprintf(frame_str, "%d fps", framerate);

		SmallFont->Draw( VideoDriverWrapper::inst().GetScreenWidth(), 0, frame_str, glArchivItem_Font::DF_RIGHT, COLOR_YELLOW);

		// Zeichenpuffer wechseln
		VideoDriverWrapper::inst().SwapBuffers();
	}
    ++frames;

    // Fenstermanager aufräumen
    if(GLOBALVARS.notdone == false)
        WindowManager::inst().CleanUp();

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
    WindowManager::inst().Switch(new dskSplash);

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

    GameClient::inst().SetInterface(NULL);

    // Wir sind nicht mehr im Spiel
    GLOBALVARS.ingame = false;

    if(LOBBYCLIENT.LoggedIn())
        // Lobby zeigen
        WindowManager::inst().Switch(new dskLobby);
    else
        // Hauptmenü zeigen
        WindowManager::inst().Switch(new dskMainMenu);

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
    if(!once) this->cursor = cursor;
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
            if(VideoDriverWrapper::inst().IsLeftDown())
                LOADER.GetImageN("resource", 31)->Draw(VideoDriverWrapper::inst().GetMouseX(), VideoDriverWrapper::inst().GetMouseY(), 0, 0, 0, 0, 0, 0);
            else
                LOADER.GetImageN("resource", 30)->Draw(VideoDriverWrapper::inst().GetMouseX(), VideoDriverWrapper::inst().GetMouseY(), 0, 0, 0, 0, 0, 0);
        } break;
        case CURSOR_SCROLL:
        case CURSOR_MOON:
        case CURSOR_RM:
        case CURSOR_RM_PRESSED:
        {
            LOADER.GetImageN("resource", cursor_next)->Draw(VideoDriverWrapper::inst().GetMouseX(), VideoDriverWrapper::inst().GetMouseY(), 0, 0, 0, 0, 0, 0);
        } break;
        case CURSOR_NONE:
        default:
        {}
    }

    cursor_next = cursor;
    return;
}
