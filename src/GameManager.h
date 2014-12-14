// $Id: GameManager.h 9541 2014-12-14 12:02:52Z marcus $
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
#ifndef GAMEMANAGER_H_INCLUDED
#define GAMEMANAGER_H_INCLUDED

#pragma once

#include "Singleton.h"

// Die verschiedenen Cursor mit ihren Indizes in resource.idx
enum CursorType
{
    CURSOR_NONE,
    CURSOR_HAND,
    CURSOR_SCROLL     = 32,
    CURSOR_MOON       = 33,
    CURSOR_RM         = 34,
    CURSOR_RM_PRESSED = 35
};

/// "Die" GameManager-Klasse
class GameManager : public Singleton<GameManager>
{
    public:
        GameManager(void);

        bool Start();
        void Stop();
        bool Run();

        bool StartMenu();
        bool ShowMenu();

        /// Average FPS Zähler zurücksetzen.
        inline void ResetAverageFPS(void)
        {
            run_time = 0;
            frame_count = 0;
        }

        inline unsigned int GetRuntime(void)
        {
            return run_time;
        }

        inline unsigned int GetFrameCount(void)
        {
            return frame_count;
        }

        inline unsigned int GetAverageFPS(void)
        {
            if(run_time == 0)
                return 0;
            return (frame_count / run_time);
        }

        inline unsigned int GetFPS(void)
        {
            return framerate;
        }

        void SetCursor(CursorType cursor = CURSOR_HAND, bool once = false);

    private:

        void DrawCursor();

    private:
        unsigned int frames;
        unsigned int frame_count;
        unsigned int framerate;
        unsigned int frame_time;
        unsigned int run_time;
        unsigned int last_time;
		unsigned int skipgf_last_time;
		unsigned int skipgf_last_report_gf;
        CursorType cursor;
        CursorType cursor_next;
};

#define GAMEMANAGER GameManager::inst()

#endif // GAMEMANAGER_H_INCLUDED
