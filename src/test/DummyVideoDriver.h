// Copyright (c) 2016 Settlers Freaks (sf-team at siedler25.org)
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

#ifndef DummyVideoDriver_h__
#define DummyVideoDriver_h__

#include "driver/src/VideoDriver.h"
#include "Point.h"

/// Dummy driver that does nothing except mocking an actual driver
class DummyVideoDriver: public VideoDriver
{
public:
    DummyVideoDriver(): VideoDriver(NULL){}
    ~DummyVideoDriver() override {}
    const char* GetName() const override { return "Dummy"; }
    bool Initialize() override { initialized = true; return true; }
    void CleanUp() override {}
    bool CreateScreen(unsigned short width, unsigned short height, const bool fullscreen) override { return ResizeScreen(width, height, fullscreen); }
    bool ResizeScreen(unsigned short width, unsigned short height, const bool fullscreen) override { screenWidth = width; screenHeight = height; isFullscreen_ = fullscreen; return true; }
    void DestroyScreen() override {}
    bool SwapBuffers() override { return true; }
    bool MessageLoop() override { return true; }
    unsigned long GetTickCount() const override { return 0; }
    void* GetFunction(const char* /*function*/) const override { return NULL; }
    void ListVideoModes(std::vector<VideoMode>& video_modes) const override { video_modes = std::vector<VideoMode>(1, VideoMode(800, 600)); }
    void SetMousePos(int x, int y) override { mouse_xy.x = x; mouse_xy.y = y; }
    KeyEvent GetModKeyState() const override { return KeyEvent(); }
    void* GetMapPointer() const override { return NULL; }
};

#endif // DummyVideoDriver_h__
