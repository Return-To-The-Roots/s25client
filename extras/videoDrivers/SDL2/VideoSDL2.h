// Copyright (C) 2005 - 2026 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "driver/VideoDriver.h"
#include <SDL.h>

class VideoDriverLoaderInterface;
struct VideoMode;

class VideoSDL2 final : public VideoDriver
{
    void cleanUp();

public:
    VideoSDL2(VideoDriverLoaderInterface* CallBack);

    ~VideoSDL2() override;

    /// Get the name of the driver
    const char* getName() const override;

    bool initialize() override;

    bool createScreen(const std::string& title, VideoMode size, DisplayMode displayMode) override;
    bool resizeScreen(VideoMode newSize, DisplayMode displayMode) override;

    void destroyScreen() override;

    /// Swap the OpenGL buffer
    bool swapBuffers() override;

    bool messageLoop() override;

    void showErrorMessage(const std::string& title, const std::string& message) override;

    /// Get a timestamp
    unsigned long getTickCount() const override;

    OpenGL_Loader_Proc getLoaderFunction() const override;

    /// Get supported video modes
    std::vector<VideoMode> listVideoModes() const override;

    /// Set mouse position
    void setMousePos(Position pos) override;

    /// Get state of the modifier keys
    KeyEvent getModKeyState() const override;

    /// Get (device-dependent!) window pointer, HWND in Windows
    void* getMapPointer() const override;

    static void printError();
    static void printError(const std::string& msg);

private:
    void handlePaste();
    void updateCurrentSizes();
    void moveWindowToCenter();
    void updateCurrentDisplayMode();

    SDL_Window* window;
    SDL_GLContext context;
};
