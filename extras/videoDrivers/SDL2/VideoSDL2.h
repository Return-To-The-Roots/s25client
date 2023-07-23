// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "driver/VideoDriver.h"
#include <SDL.h>

class VideoDriverLoaderInterface;
struct VideoMode;

class VideoSDL2 final : public VideoDriver
{
    void CleanUp();

public:
    VideoSDL2(VideoDriverLoaderInterface* CallBack);

    ~VideoSDL2() override;

    /// Get the name of the driver
    const char* GetName() const override;

    bool Initialize() override;

    bool CreateScreen(const std::string& title, const VideoMode& size, bool fullscreen) override;
    bool ResizeScreen(const VideoMode& newSize, bool fullscreen) override;

    void DestroyScreen() override;

    /// Swap the OpenGL buffer
    bool SwapBuffers() override;

    bool MessageLoop() override;

    void ShowErrorMessage(const std::string& title, const std::string& message) override;

    /// Get a timestamp
    unsigned long GetTickCount() const override;

    OpenGL_Loader_Proc GetLoaderFunction() const override;

    /// Add supported video modes
    void ListVideoModes(std::vector<VideoMode>& video_modes) const override;

    /// Set mouse position
    void SetMousePos(Position pos) override;

    /// Get state of the modifier keys
    KeyEvent GetModKeyState() const override;

    /// Get (device-dependent!) window pointer, HWND in Windows
    void* GetMapPointer() const override;

protected:
    void onGuiScaleChanged() override;

private:
    void PrintError(const std::string& msg) const;
    void HandlePaste();
    void UpdateCurrentSizes();
    void MoveWindowToCenter();

    SDL_Window* window;
    SDL_GLContext context;
};
