// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "FrameCounter.h"
#include <optional>

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

    bool start();
    void stop();
    bool run();

    bool showMenu();

    /// Average FPS Zähler zurücksetzen.
    void resetAverageGFPS();
    FrameCounter::clock::duration getRuntime() { return gfCounter_.getCurIntervalLength(); }
    unsigned getNumFrames() { return gfCounter_.getCurNumFrames(); }
    unsigned getAverageGFPS() { return gfCounter_.getCurFrameRate(); }

private:
    bool showSplashscreen();

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
    std::optional<SkipReport> lastSkipReport;
};

GameManager& getGlobalGameManager();
void setGlobalGameManager(GameManager* gameManager);

#define GAMEMANAGER getGlobalGameManager()
