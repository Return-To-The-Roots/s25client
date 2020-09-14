// Copyright (c) 2005 - 2018 Settlers Freaks (sf-team at siedler25.org)
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

private:
    void PrintError(const std::string& msg) const;
    void HandlePaste();
    void UpdateCurrentSizes();
    void MoveWindowToCenter();

    SDL_Window* window;
    SDL_GLContext context;
};
