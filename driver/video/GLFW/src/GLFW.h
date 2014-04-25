// $Id: GLFW.h 9357 2014-04-25 15:35:25Z FloSoft $
//
// Copyright (c) 2005-2009 Settlers Freaks (sf-team at siedler25.org)
//
// This file is part of Siedler II.5 RTTR.
//
// Siedler II.5 RTTR is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 2 of the License, or
// (at your option) any later version.
//
// Siedler II.5 RTTR is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with Siedler II.5 RTTR. If not, see <http://www.gnu.org/licenses/>.
#ifndef GLFW_H_INCLUDED
#define GLFW_H_INCLUDED

#pragma once

#include <VideoDriver.h>

/// Klasse für den GL-Framework Videotreiber.
class VideoGLFW : public VideoDriver
{
    public:
        /// Konstruktor von @p VideoGLFW.
        VideoGLFW(VideoDriverLoaderInterface* CallBack);

        /// Destruktor von @p VideoGLFW.
        ~VideoGLFW(void);

        /// Treiberinitialisierungsfunktion.
        bool Initialize(void);

        /// Treiberaufräumfunktion.
        void CleanUp(void);

        /// Erstellt das Fenster mit entsprechenden Werten.
        bool CreateScreen(unsigned short width, unsigned short height, bool fullscreen);

        /// Erstellt oder verändert das Fenster mit entsprechenden Werten.
        bool ResizeScreen(unsigned short* width, unsigned short* height, bool fullscreen);

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

        /// Funktion zum Setzen der Mauskoordinaten.
        void SetMousePos(int x, int y);

        /// Funktion zum Setzen der X-Koordinate der Maus.
        void SetMousePosX(int x);

        /// Funktion zum Setzen der Y-Koordinate der Maus.
        void SetMousePosY(int y);

        /// Get state of the modifier keys
        KeyEvent GetModKeyState(void) const;

        /// Gibt Pointer auf ein Fenster zur?ck (device-dependent!), HWND unter Windows
        void* GetWindowPointer() const;

    private:
        /// Callbackfunktion des GL-Frameworks um Mausbewegungen abzufangen.
        static void GLFWCALL OnMouseMove(int x, int y);

        /// Callbackfunktion des GL-Frameworks um Mausklicks abzufangen.
        static void GLFWCALL OnMouseButton(int button, int action);

        /// Tastenwandlungsfunktion.
        unsigned char TranslateKey(unsigned char key);

        /// Callbackfunktion des GL-Frameworks um Tastatureingaben abzufangen.
        static void GLFWCALL OnKeyAction(int key, int action);

    private:
        bool mouse_l; ///< Status der Linken Maustaste.
        bool mouse_r; ///< Status der Rechten Maustaste.
        void* libGL;  ///< Handle auf die libGL.
};

#endif // !GLFW_H_INCLUDED
