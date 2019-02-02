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
#ifndef VIDEODRIVERWRAPPER_H_INCLUDED
#define VIDEODRIVERWRAPPER_H_INCLUDED

#include "DriverWrapper.h"
#include "Point.h"
#include "driver/KeyEvent.h"
#include "driver/VideoMode.h"
#include "libutil/Singleton.h"
#include <memory>
#include <string>

class IVideoDriver;
class IRenderer;
class FrameCounter;
class FrameLimiter;

///////////////////////////////////////////////////////////////////////////////
// DriverWrapper
class VideoDriverWrapper : public Singleton<VideoDriverWrapper, SingletonPolicies::WithLongevity>
{
public:
    static constexpr unsigned Longevity = 30;

    VideoDriverWrapper();
    ~VideoDriverWrapper() override;

    /// Loads a new driver. Takes the existing one, if given
    /// Do not use this class unless this returned true!
    bool LoadDriver(IVideoDriver* existingDriver = nullptr);
    void UnloadDriver();
    IVideoDriver* GetDriver() const { return videodriver; }

    /// Erstellt das Fenster.
    bool CreateScreen(unsigned short screen_width, unsigned short screen_height, bool fullscreen);
    /// Verändert Auflösung, Fenster/Fullscreen
    bool ResizeScreen(unsigned short screen_width, unsigned short screen_height, bool fullscreen);
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

    IRenderer* GetRenderer() { return renderer_.get(); }

    /// Swapped den Buffer
    void SwapBuffers();
    /// Clears the screen (glClear)
    void ClearScreen();
    // liefert den Mausstatus (sollte nur beim Zeichnen der Maus verwendet werden, für alles andere die Mausmessages
    // benutzen!!!)
    int GetMouseX() const;
    int GetMouseY() const;
    Position GetMousePos() const;

    /// Listet verfügbare Videomodi auf
    void ListVideoModes(std::vector<VideoMode>& video_modes) const;

    /// Gibt Pointer auf ein Fenster zurück (device-dependent!), HWND unter Windows
    void* GetMapPointer() const;

    Extent GetScreenSize() const;
    bool IsFullscreen() const;

    bool IsLeftDown();
    bool IsRightDown();
    // setzt den Mausstatus
    void SetMousePos(int x, int y);
    void SetMousePos(const Position& newPos);
    /// Get state of the modifier keys
    KeyEvent GetModKeyState() const;

    // Nachrichtenschleife
    bool Run();

    unsigned GetTickCount();
    /// Set framerate target (FPS)
    /// negative for unlimited, 0 for hardware VSync
    void setTargetFramerate(int target);
    unsigned GetFPS() const;

    std::string GetName() const;
    bool IsLoaded() const { return videodriver != nullptr; }

    /// Calculate the size of the texture which is optimal for the driver and at least minSize
    Extent calcPreferredTextureSize(const Extent& minSize) const;

private:
    // Viewpoint und Co initialisieren
    bool Initialize();
    bool setHwVSync(bool enabled);

    // prüft ob eine Extension verfügbar ist
    bool hasExtension(const std::string& extension);

    // lädt eine Funktion aus den Extensions
    void* loadExtension(const std::string& extension);

    // Alle (im Programm benutzen) Extensions laden
    bool LoadAllExtensions();

private:
    DriverWrapper driver_wrapper;
    IVideoDriver* videodriver;
    std::unique_ptr<IRenderer> renderer_;
    std::unique_ptr<FrameCounter> frameCtr_;
    std::unique_ptr<FrameLimiter> frameLimiter_;
    bool loadedFromDll;
    /// (Some) OpenGL can be disabled for testing
    bool isOglEnabled_;

    std::vector<unsigned> texture_list;
    unsigned texture_current;
};

#define VIDEODRIVER VideoDriverWrapper::inst()

#endif
