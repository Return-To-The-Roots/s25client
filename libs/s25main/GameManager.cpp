// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "GameManager.h"
#include "GlobalVars.h"
#include "Loader.h"
#include "RTTR_Assert.h"
#include "RttrConfig.h"
#include "Settings.h"
#include "WindowManager.h"
#include "desktops/dskLobby.h"
#include "desktops/dskMainMenu.h"
#include "desktops/dskSplash.h"
#include "drivers/AudioDriverWrapper.h"
#include "drivers/VideoDriverWrapper.h"
#include "files.h"
#include "network/GameClient.h"
#include "network/GameServer.h"
#include "ogl/glArchivItem_Bitmap.h"
#include "liblobby/LobbyClient.h"
#include "libsiedler2/Archiv.h"
#include "s25util/Log.h"
#include "s25util/error.h"
#include <boost/pointer_cast.hpp>

GameManager::GameManager(Log& log, Settings& settings, VideoDriverWrapper& videoDriver, AudioDriverWrapper& audioDriver,
                         WindowManager& windowManager)
    : log_(log), settings_(settings), videoDriver_(videoDriver), audioDriver_(audioDriver),
      windowManager_(windowManager)
{
    ResetAverageGFPS();
}

/**
 *  Spiel starten
 */
bool GameManager::Start()
{
    // Einstellungen laden
    settings_.Load();

    /// Videotreiber laden
    if(!videoDriver_.LoadDriver(settings_.driver.video))
    {
        s25util::error(_("Video driver couldn't be loaded!"));
        return false;
    }

    // Fenster erstellen
    const auto screenSize =
      settings_.video.fullscreen ? settings_.video.fullscreenSize : settings_.video.windowedSize; //-V807
    if(!videoDriver_.CreateScreen(screenSize, settings_.video.fullscreen))
        return false;
    videoDriver_.setTargetFramerate(settings_.video.framerate);
    videoDriver_.SetMouseWarping(settings_.global.smartCursor);
    videoDriver_.setGuiScalePercent(settings_.video.guiScale);

    /// Audiodriver laden
    if(!audioDriver_.LoadDriver(settings_.driver.audio))
    {
        s25util::warning(_("Audio driver couldn't be loaded!"));
        // return false;
    }

    /// Lautstärken gleich mit setzen
    audioDriver_.SetMasterEffectVolume(settings_.sound.effectsVolume); //-V807
    audioDriver_.SetMusicVolume(settings_.sound.musicVolume);

    // Treibereinstellungen abspeichern
    settings_.Save();

    log_.write(_("\nStarting the game\n"));
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
    settings_.Save();

    // Fenster beenden
    videoDriver_.DestroyScreen();
}

/**
 *  Hauptschleife.
 */
bool GameManager::Run()
{
    // Nachrichtenschleife
    if(!videoDriver_.Run())
        GLOBALVARS.notdone = false;

    LOBBYCLIENT.Run();

    // Get this before the run so we know if we are currently skipping
    const unsigned targetSkipGF = GAMECLIENT.skiptogf;
    GAMECLIENT.Run();
    GAMESERVER.Run();

    if(targetSkipGF)
    {
        // if we skip drawing write a comment every 5k gf
        unsigned current_time = videoDriver_.GetTickCount();
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
                    log_.write(_("jumping to gf %i, now at gf %i, time for last 5k gf: %.3f s, avg gf time %.3f ms \n"))
                      % targetSkipGF % curGF % (timeDiff / 1000) % (timeDiff / numGFPassed);
                } else
                    log_.write(_("jumping to gf %i, now at gf %i \n")) % targetSkipGF % curGF;
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
                log_.write(_("jump to gf %i complete, time for last %i gf: %.3f s, avg gf time %.3f ms \n"))
                  % targetSkipGF % numGFPassed % (timeDiff / 1000) % (timeDiff / numGFPassed);
                lastSkipReport.reset();
            } else
            {
                log_.write(_("jump to gf %1% complete\n")) % targetSkipGF;
            }
        }
    } else
    {
        videoDriver_.ClearScreen();
        windowManager_.Draw();
        videoDriver_.SwapBuffers();
    }
    gfCounter_.update();

    // Fenstermanager aufräumen
    if(!GLOBALVARS.notdone)
        windowManager_.CleanUp();

    return GLOBALVARS.notdone;
}

bool GameManager::ShowSplashscreen()
{
    libsiedler2::Archiv arSplash;
    if(!LOADER.Load(arSplash, RTTRCONFIG.ExpandPath(s25::files::splash)))
        return false;
    auto image = boost::dynamic_pointer_cast<glArchivItem_Bitmap>(arSplash.release(0));
    if(!image)
    {
        s25util::error(_("Splash screen couldn't be loaded!"));
        return false;
    }
    windowManager_.Switch(std::make_unique<dskSplash>(std::move(image)));
    return true;
}

/**
 *  zeigt das Hauptmenü.
 */
bool GameManager::ShowMenu()
{
    GAMECLIENT.Stop();
    GAMESERVER.Stop();

    if(LOBBYCLIENT.IsLoggedIn())
        // Lobby zeigen
        windowManager_.Switch(std::make_unique<dskLobby>());
    else
        // Hauptmenü zeigen
        windowManager_.Switch(std::make_unique<dskMainMenu>());

    return true;
}

void GameManager::ResetAverageGFPS()
{
    gfCounter_ = FrameCounter(FrameCounter::clock::duration::max()); // Never update
}

static GameManager* globalGameManager = nullptr;

GameManager& getGlobalGameManager()
{
    return *globalGameManager;
}
void setGlobalGameManager(GameManager* gameManager)
{
    globalGameManager = gameManager;
}
