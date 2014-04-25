// $Id: SDL.h 9357 2014-04-25 15:35:25Z FloSoft $
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
#ifndef SDL_H_INCLUDED
#define SDL_H_INCLUDED

#pragma once

#include <VideoDriver.h>

/// Klasse für den SDL Videotreiber.
class VideoSDL : public VideoDriver
{
    public:
        /// Konstruktor von @p VideoSDL.
        VideoSDL(VideoDriverLoaderInterface* CallBack);

        /// Destruktor von @p VideoSDL.
        ~VideoSDL(void);

        /// Funktion zum Auslesen des Treibernamens.
        const char* GetName(void) const;

        /// Treiberinitialisierungsfunktion.
        bool Initialize(void);

        /// Treiberaufräumfunktion.
        void CleanUp(void);

        /// Erstellt das Fenster mit entsprechenden Werten.
        bool CreateScreen(unsigned short width, unsigned short height, const bool fullscreen);

        /// Erstellt oder verändert das Fenster mit entsprechenden Werten.
        bool ResizeScreen(unsigned short width, unsigned short height, const bool fullscreen);

        /// Schliesst das Fenster.
        void DestroyScreen(void);

        /// Wechselt die OpenGL-Puffer.
        bool SwapBuffers(void);

        /// Die Nachrichtenschleife.
        bool MessageLoop(void);

        /// Funktion zum Auslesen des TickCounts.
        unsigned long GetTickCount(void) const;

        /// Funktion zum Holen einer Subfunktion.
        void* GetFunction(const char* function) const;

        /// Listet verfügbare Videomodi auf
        void ListVideoModes(std::vector<VideoMode>& video_modes) const;

        /// Funktion zum Setzen der Mauskoordinaten.
        void SetMousePos(int x, int y);

        /// Funktion zum Setzen der X-Koordinate der Maus.
        void SetMousePosX(int x);

        /// Funktion zum Setzen der Y-Koordinate der Maus.
        void SetMousePosY(int y);

        /// Get state of the modifier keys
        KeyEvent GetModKeyState(void) const;

        /// Gibt Pointer auf ein Fenster zurück (device-dependent!), HWND unter Windows
        void* GetWindowPointer() const;

    private:
        SDL_Surface* screen; ///< Das Fenster-SDL-Surface.
};

#endif // !SDL_H_INCLUDED
