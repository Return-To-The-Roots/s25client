// $Id: VideoDriverWrapper.h 9357 2014-04-25 15:35:25Z FloSoft $
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
#ifndef VIDEODRIVERWRAPPER_H_INCLUDED
#define VIDEODRIVERWRAPPER_H_INCLUDED


#include "Singleton.h"
#include "ExtensionList.h"
#include "DriverWrapper.h"
#include "../driver/src/VideoDriver.h"

class VideoDriver;

///////////////////////////////////////////////////////////////////////////////
// DriverWrapper
class VideoDriverWrapper : public Singleton<VideoDriverWrapper>
{
    public:
        /// Konstruktor von @p DriverWrapper
        VideoDriverWrapper();
        /// Destruktor von @p DriverWrapper
        ~VideoDriverWrapper();

        /// Läd den Treiber
        bool LoadDriver(void);

        /// Erstellt das Fenster.
        bool CreateScreen(const unsigned short screen_width, const unsigned short screen_height, const bool fullscreen);
        /// Verändert Auflösung, Fenster/Fullscreen
        bool ResizeScreen(const unsigned short screen_width, const unsigned short screen_height, const bool fullscreen);
        // Viewport (neu) setzen
        void RenewViewport(bool onlyRenew = false);
        // zerstört das Fenster.
        bool DestroyScreen();
        // räumt die Texturen auf
        void CleanUp();
        // erstellt eine Textur
        unsigned int GenerateTexture();
        inline void BindTexture(unsigned int t) {if (t != texture_current) {texture_current = t; glBindTexture(GL_TEXTURE_2D, t);}}
        inline void DeleteTexture(unsigned int t) {if (t == texture_current) {texture_current = 0;} glDeleteTextures(1, &t);}

        // Swapped den Buffer
        bool SwapBuffers();
        // liefert den Mausstatus (sollte nur beim Zeichnen der Maus verwendet werden, für alles andere die Mausmessages
        // benutzen!!!)
        int GetMouseX();
        int GetMouseY();

        /// Listet verfügbare Videomodi auf
        void ListVideoModes(std::vector<VideoDriver::VideoMode>& video_modes) const;

        /// Gibt Pointer auf ein Fenster zurück (device-dependent!), HWND unter Windows
        void* GetWindowPointer() const;

        unsigned short GetScreenWidth()  const
        { const unsigned short w = videodriver->GetScreenWidth(); return (w < 800 ? 800 : w); }
        unsigned short GetScreenHeight() const
        { const unsigned short h = videodriver->GetScreenHeight(); return (h < 600 ? 600 : h); }
        bool IsFullscreen() const
        { return videodriver->IsFullscreen(); }

        bool IsLeftDown();
        bool IsRightDown();
        // setzt den Mausstatus
        void SetMousePos(const int x, const int y);
        /// Get state of the modifier keys
        KeyEvent GetModKeyState(void) const { if(videodriver) return videodriver->GetModKeyState(); const KeyEvent ke = {KT_INVALID, 0, false, false, false}; return ke;  }

        // Nachrichtenschleife
        bool Run();

        unsigned int GetTickCount();

        const char* GetName(void) const { if(videodriver) return videodriver->GetName();    return NULL; }

    private:
        // Viewpoint und Co initialisieren
        bool Initialize();

        // prüft ob eine Extension verfügbar ist
        bool hasExtension(const char* extension);

        // lädt eine Funktion aus den Extensions
        void* loadExtension(const char* extension);

        // Alle (im Programm benutzen) Extensions laden
        bool LoadAllExtensions();

    private:

        DriverWrapper driver_wrapper;
        VideoDriver* videodriver;

        unsigned int texture_list[100000];
        unsigned int texture_pos;
        unsigned int texture_current;
};


#endif

