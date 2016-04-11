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
#ifndef VIDEODRIVERWRAPPER_H_INCLUDED
#define VIDEODRIVERWRAPPER_H_INCLUDED


#include "Singleton.h"
#include "DriverWrapper.h"
#include "driver/src/VideoInterface.h"
#include <string>
#include <boost/array.hpp>

///////////////////////////////////////////////////////////////////////////////
// DriverWrapper
class VideoDriverWrapper : public Singleton<VideoDriverWrapper, SingletonPolicies::WithLongevity>
{
    public:

        BOOST_STATIC_CONSTEXPR unsigned Longevity = 30;

        VideoDriverWrapper();
        ~VideoDriverWrapper() override;

        /// Läd den Treiber
        bool LoadDriver();

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
        void BindTexture(unsigned int t);
        void DeleteTexture(unsigned int t);

        // Swapped den Buffer
        bool SwapBuffers();
        // liefert den Mausstatus (sollte nur beim Zeichnen der Maus verwendet werden, für alles andere die Mausmessages
        // benutzen!!!)
        int GetMouseX();
        int GetMouseY();

        /// Listet verfügbare Videomodi auf
        void ListVideoModes(std::vector<VideoMode>& video_modes) const;

        /// Gibt Pointer auf ein Fenster zurück (device-dependent!), HWND unter Windows
        void* GetMapPointer() const;

        unsigned short GetScreenWidth()  const { const unsigned short w = videodriver->GetScreenWidth(); return (w < 800 ? 800 : w); }
        unsigned short GetScreenHeight() const { const unsigned short h = videodriver->GetScreenHeight(); return (h < 600 ? 600 : h); }
        bool IsFullscreen() const { return videodriver->IsFullscreen(); }

        bool IsLeftDown();
        bool IsRightDown();
        // setzt den Mausstatus
        void SetMousePos(const int x, const int y);
        /// Get state of the modifier keys
        KeyEvent GetModKeyState() const;

        // Nachrichtenschleife
        bool Run();

        unsigned int GetTickCount();

        std::string GetName() const { return (videodriver) ? videodriver->GetName() : ""; }
        bool IsLoaded() const { return videodriver != NULL; }

    private:
        // Viewpoint und Co initialisieren
        bool Initialize();

        // prüft ob eine Extension verfügbar ist
        bool hasExtension(const std::string& extension);

        // lädt eine Funktion aus den Extensions
        void* loadExtension(const std::string& extension);

        // Alle (im Programm benutzen) Extensions laden
        bool LoadAllExtensions();

    private:

        DriverWrapper driver_wrapper;
        IVideoDriver* videodriver;

        boost::array<unsigned int, 100000> texture_list;
        unsigned int texture_pos;
        unsigned int texture_current;
};

#define VIDEODRIVER VideoDriverWrapper::inst()

#endif

