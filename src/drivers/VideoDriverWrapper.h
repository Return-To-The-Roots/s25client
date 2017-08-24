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

#include "DriverWrapper.h"
#include "Point.h"
#include "driver/src/VideoInterface.h"
#include "libutil/src/Singleton.h"
#include <boost/array.hpp>
#include <string>

///////////////////////////////////////////////////////////////////////////////
// DriverWrapper
class VideoDriverWrapper : public Singleton<VideoDriverWrapper, SingletonPolicies::WithLongevity>
{
public:
    BOOST_STATIC_CONSTEXPR unsigned Longevity = 30;

    VideoDriverWrapper();
    ~VideoDriverWrapper() override;

    /// Loads a new driver. Takes the existing one, if given
    bool LoadDriver(IVideoDriver* existingDriver = NULL);
    IVideoDriver* GetDriver() const { return videodriver; }

    /// Erstellt das Fenster.
    bool CreateScreen(const unsigned short screen_width, const unsigned short screen_height, const bool fullscreen);
    /// Verändert Auflösung, Fenster/Fullscreen
    bool ResizeScreen(const unsigned short screen_width, const unsigned short screen_height, const bool fullscreen);
    /// Viewport (neu) setzen
    void RenewViewport();
    /// zerstört das Fenster.
    bool DestroyScreen();
    /// räumt die Texturen auf
    void CleanUp();
    /// erstellt eine Textur
    unsigned GenerateTexture();
    void BindTexture(unsigned t);
    void DeleteTexture(unsigned t);

    /// Swapped den Buffer
    bool SwapBuffers();
    /// Clears the screen (glClear)
    void ClearScreen();
    // liefert den Mausstatus (sollte nur beim Zeichnen der Maus verwendet werden, für alles andere die Mausmessages
    // benutzen!!!)
    int GetMouseX() const;
    int GetMouseY() const;
    Point<int> GetMousePos() const;

    /// Listet verfügbare Videomodi auf
    void ListVideoModes(std::vector<VideoMode>& video_modes) const;

    /// Gibt Pointer auf ein Fenster zurück (device-dependent!), HWND unter Windows
    void* GetMapPointer() const;

    unsigned short GetScreenWidth() const;
    unsigned short GetScreenHeight() const;
    Extent GetScreenSize() const;
    bool IsFullscreen() const { return videodriver->IsFullscreen(); }

    bool IsLeftDown();
    bool IsRightDown();
    // setzt den Mausstatus
    void SetMousePos(const int x, const int y);
    void SetMousePos(const Point<int>& newPos);
    /// Get state of the modifier keys
    KeyEvent GetModKeyState() const;

    // Nachrichtenschleife
    bool Run();

    unsigned GetTickCount();

    std::string GetName() const { return (videodriver) ? videodriver->GetName() : ""; }
    bool IsLoaded() const { return videodriver != NULL; }

    /// Return the next highest power of 2 for the given value
    static unsigned nextPowerOfTwo(unsigned k);
    /// Calculate the size of the texture which is optimal for the driver and at least minSize
    Extent calcPreferredTextureSize(const Extent& minSize) const;

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
    /// (Some) OpenGL can be disabled for testing
    bool isOglEnabled_;

    boost::array<unsigned, 100000> texture_list;
    unsigned texture_pos;
    unsigned texture_current;
};

#define VIDEODRIVER VideoDriverWrapper::inst()

#endif
