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
#ifndef SDL_H_INCLUDED
#define SDL_H_INCLUDED

#pragma once

#include <VideoDriver.h>
class VideoDriverLoaderInterface;
struct VideoMode;
struct SDL_Surface;

/// Klasse für den SDL Videotreiber.
class VideoSDL : public VideoDriver
{
    public:
        VideoSDL(VideoDriverLoaderInterface* CallBack);

        ~VideoSDL() override;

        /// Funktion zum Auslesen des Treibernamens.
        const char* GetName() const override;

        /// Treiberinitialisierungsfunktion.
        bool Initialize() override;

        /// Treiberaufräumfunktion.
        void CleanUp() override;

        /// Erstellt das Fenster mit entsprechenden Werten.
        bool CreateScreen(unsigned short width, unsigned short height, const bool fullscreen) override;

        /// Erstellt oder verändert das Fenster mit entsprechenden Werten.
        bool ResizeScreen(unsigned short width, unsigned short height, const bool fullscreen) override;

        /// Schliesst das Fenster.
        void DestroyScreen() override;

        /// Wechselt die OpenGL-Puffer.
        bool SwapBuffers() override;

        /// Die Nachrichtenschleife.
        bool MessageLoop() override;

        /// Funktion zum Auslesen des TickCounts.
        unsigned long GetTickCount() const override;

        /// Funktion zum Holen einer Subfunktion.
        void* GetFunction(const char* function) const override;

        /// Listet verfügbare Videomodi auf
        void ListVideoModes(std::vector<VideoMode>& video_modes) const override;

        /// Funktion zum Setzen der Mauskoordinaten.
        void SetMousePos(int x, int y) override;

        /// Funktion zum Setzen der X-Koordinate der Maus.
        void SetMousePosX(int x) override;

        /// Funktion zum Setzen der Y-Koordinate der Maus.
        void SetMousePosY(int y) override;

        /// Get state of the modifier keys
        KeyEvent GetModKeyState() const override;

        /// Gibt Pointer auf ein Fenster zurück (device-dependent!), HWND unter Windows
        void* GetMapPointer() const override;

    private:
        SDL_Surface* screen; /// Das Fenster-SDL-Surface.
};

#endif // !SDL_H_INCLUDED
