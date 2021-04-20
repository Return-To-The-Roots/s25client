// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "FrameCounter.h"
#include <boost/optional.hpp>

class Log;
class Settings;
class VideoDriverWrapper;
class AudioDriverWrapper;
class WindowManager;

/// "Die" GameManager-Klasse
class GameManager
{
public:
    GameManager(Log& log, Settings& settings, VideoDriverWrapper& videoDriver, AudioDriverWrapper& audioDriver,
                WindowManager& windowManager);

    bool Start();
    void Stop();
    bool Run();

    bool ShowMenu();

    /// Average FPS Zähler zurücksetzen.
    void ResetAverageGFPS();
    FrameCounter::clock::duration GetRuntime() { return gfCounter_.getCurIntervalLength(); }
    unsigned GetNumFrames() { return gfCounter_.getCurNumFrames(); }
    unsigned GetAverageGFPS() { return gfCounter_.getCurFrameRate(); }

private:
    bool ShowSplashscreen();

    Log& log_;
    Settings& settings_;
    VideoDriverWrapper& videoDriver_;
    AudioDriverWrapper& audioDriver_;
    WindowManager& windowManager_;
    FrameCounter gfCounter_;

    struct SkipReport
    {
        unsigned time, gf;
    };
    boost::optional<SkipReport> lastSkipReport;
};

GameManager& getGlobalGameManager();
void setGlobalGameManager(GameManager* gameManager);

#define GAMEMANAGER getGlobalGameManager()
