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
