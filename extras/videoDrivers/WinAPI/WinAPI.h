// Copyright (C) 2005 - 2026 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "driver/VideoDriver.h"
#ifndef WIN32_LEAN_AND_MEAN
#    define WIN32_LEAN_AND_MEAN
#endif
#include "Point.h"
#include "Rect.h"
#include <windows.h>
#include <string>
#include <utility>

class VideoWinAPI final : public VideoDriver
{
    void CleanUp();

public:
    VideoWinAPI(VideoDriverLoaderInterface* CallBack);
    ~VideoWinAPI();

    /// Return name of driver
    const char* GetName() const override;

    /// Initialize the driver, return true on success
    bool Initialize() override;

    /// Create window/rendering context with given title, size and display mode, return true on success
    bool CreateScreen(const std::string& title, const VideoMode newSize, DisplayMode displayMode) override;

    /// Change window/resolution, return true on success
    bool ResizeScreen(VideoMode newSize, DisplayMode displayMode) override;

    /// Close window
    void DestroyScreen() override;

    /// Swap OpenGL buffers
    bool SwapBuffers() override;

    /// Process messages/events, return false when the application should quit
    bool MessageLoop() override;

    /// Popup Window
    void ShowErrorMessage(const std::string& title, const std::string& message) override;

    /// Return the current tick count (time since epoch in ms)
    unsigned long GetTickCount() const override;

    /// Get a pointer to an OpenGL function loader
    OpenGL_Loader_Proc GetLoaderFunction() const override;

    /// Return supported resolutions
    std::vector<VideoMode> ListVideoModes() const override;

    /// Set position of the mouse cursor in screen coordinates
    void SetMousePos(Position pos) override;

    /// Get state of the modifier keys
    KeyEvent GetModKeyState() const override;

    /// Get pointer to window (device-dependent!)
    void* GetMapPointer() const override;

private:
    /// Get style and extended style flags
    std::pair<DWORD, DWORD> GetStyleFlags(DisplayMode mode) const;
    /// Calculate the rect for the window of the requested size at the display mode
    /// Returns the final rect (position&size) for the window and the possibly adjusted size of the render region
    std::pair<Rect, VideoMode> CalculateWindowRect(DisplayMode mode, VideoMode requestedSize) const;
    bool RegisterAndCreateWindow(const std::string& title, VideoMode wndSize, DisplayMode displayMode);
    bool InitOGL();
    static bool MakeFullscreen(VideoMode resolution);
    VideoMode getDesktopSize(VideoMode fallback) const;

    /// Callback for pressed character keys
    void OnWMChar(unsigned c, bool disablepaste = false, LPARAM lParam = 0);
    void OnWMKeyDown(unsigned c, LPARAM lParam = 0);

    /// Handle paste from clipboard (CTRL+V)
    void OnWMPaste();

    /// Window callback function
    static LRESULT CALLBACK WindowProc(HWND window, UINT msg, WPARAM wParam, LPARAM lParam);

private:
    int mouse_z;     /// Scrolling position for mouse wheel.
    HWND screen;     /// window handle
    HDC screen_dc;   /// Draw context of the window.
    HGLRC screen_rc; /// OpenGL-context of the window.
    bool isDisplayModeChangeable, isMinimized;
    std::wstring windowClassName;
};
