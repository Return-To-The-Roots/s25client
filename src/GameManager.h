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
#ifndef GAMEMANAGER_H_INCLUDED
#define GAMEMANAGER_H_INCLUDED

#pragma once

#include "FrameCounter.h"
#include "libutil/Singleton.h"

// Die verschiedenen Cursor mit ihren Indizes in resource.idx
enum CursorType
{
    CURSOR_NONE,
    CURSOR_HAND,
    CURSOR_SCROLL = 32,
    CURSOR_MOON = 33,
    CURSOR_RM = 34
};

/// "Die" GameManager-Klasse
class GameManager : public Singleton<GameManager, SingletonPolicies::WithLongevity>
{
public:
    BOOST_STATIC_CONSTEXPR unsigned Longevity = 15;

    GameManager();

    bool Start();
    void Stop();
    bool Run();

    bool ShowMenu();

    /// Average FPS Zähler zurücksetzen.
    void ResetAverageGFPS();
    FrameCounter::clock::duration GetRuntime() { return gfCounter_.getCurIntervalLength(); }
    unsigned GetNumFrames() { return gfCounter_.getCurNumFrames(); }
    unsigned GetAverageGFPS() { return gfCounter_.getCurFrameRate(); }

    void SetCursor(CursorType cursor = CURSOR_HAND, bool once = false);

private:
    bool ShowSplashscreen();
    void DrawCursor();

    FrameCounter gfCounter_;

    unsigned skipgf_last_time;
    unsigned skipgf_last_report_gf;
    CursorType cursor_;
    CursorType cursor_next;
};

#define GAMEMANAGER GameManager::inst()

#endif // GAMEMANAGER_H_INCLUDED
