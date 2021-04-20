// Copyright (c) 2005 - 2017 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "driver/VideoDriver.h"
#ifndef WIN32_LEAN_AND_MEAN
#    define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <string>
#include <utility>

/// Klasse für den WinAPI Videotreiber.
class VideoWinAPI final : public VideoDriver
{
    /// Treiberaufräumfunktion.
    void CleanUp();

public:
    VideoWinAPI(VideoDriverLoaderInterface* CallBack);

    ~VideoWinAPI();

    /// Funktion zum Auslesen des Treibernamens.
    const char* GetName() const override;

    /// Treiberinitialisierungsfunktion.
    bool Initialize() override;

    /// Erstellt das Fenster mit entsprechenden Werten.
    bool CreateScreen(const std::string& title, const VideoMode& newSize, bool fullscreen) override;

    /// Erstellt oder verändert das Fenster mit entsprechenden Werten.
    bool ResizeScreen(const VideoMode& newSize, bool fullscreen) override;

    /// Schliesst das Fenster.
    void DestroyScreen() override;

    /// Wechselt die OpenGL-Puffer.
    bool SwapBuffers() override;

    /// Die Nachrichtenschleife.
    bool MessageLoop() override;

    /// Funktion zum Auslesen des TickCounts.
    unsigned long GetTickCount() const override;

    /// Funktion zum Holen einer Subfunktion.
    OpenGL_Loader_Proc GetLoaderFunction() const override;

    /// Listet verfügbare Videomodi auf
    void ListVideoModes(std::vector<VideoMode>& video_modes) const override;

    /// Funktion zum Setzen der Mauskoordinaten.
    void SetMousePos(Position pos) override;

    /// Get state of the modifier keys
    KeyEvent GetModKeyState() const override;

    /// Gibt Pointer auf ein Fenster zurück (device-dependent!), HWND unter Windows
    void* GetMapPointer() const override;

private:
    std::pair<DWORD, DWORD> GetStyleFlags(bool fullscreen) const;
    /// Calculate the rect for the window and adjusts the (usable) size if required
    RECT CalculateWindowRect(bool fullscreen, VideoMode& size) const;
    bool RegisterAndCreateWindow(const std::string& title, const VideoMode& wndSize, bool fullscreen);
    bool InitOGL();
    static bool MakeFullscreen(const VideoMode& resolution);

    /// Funktion zum Senden einer gedrückten Taste.
    void OnWMChar(unsigned c, bool disablepaste = false, LPARAM lParam = 0);
    void OnWMKeyDown(unsigned c, LPARAM lParam = 0);

    /// Funktion zum Pasten von Text aus dem Clipboard.
    void OnWMPaste();

    /// Callbackfunktion der WinAPI.
    static LRESULT CALLBACK WindowProc(HWND window, UINT msg, WPARAM wParam, LPARAM lParam);

private:
    bool mouse_l;    /// Status der Linken Maustaste.
    bool mouse_r;    /// Status der Rechten Maustaste.
    int mouse_z;     /// Scrolling position for mousewheel.
    HWND screen;     /// Fensterhandle.
    HDC screen_dc;   /// Zeichenkontext des Fensters.
    HGLRC screen_rc; /// OpenGL-Kontext des Fensters.
    bool isWindowResizable, isMinimized;
    std::wstring windowClassName;
};
