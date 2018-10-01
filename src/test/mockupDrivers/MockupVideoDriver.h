// Copyright (c) 2017 - 2017 Settlers Freaks (sf-team at siedler25.org)
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

#ifndef MockupVideoDriver_h__
#define MockupVideoDriver_h__

#include "driver/VideoDriver.h"

class MockupVideoDriver : public VideoDriver
{
public:
    MockupVideoDriver(VideoDriverLoaderInterface* CallBack);
    const char* GetName() const override;
    bool Initialize() override;
    void CleanUp() override;
    bool CreateScreen(const std::string& title, const VideoMode& newSize, bool fullscreen) override;
    bool ResizeScreen(const VideoMode& newSize, bool fullscreen) override;
    void DestroyScreen() override {}
    bool SwapBuffers() override { return true; }
    bool MessageLoop() override;
    unsigned long GetTickCount() const override;
    void* GetFunction(const char* function) const override;
    void ListVideoModes(std::vector<VideoMode>& video_modes) const override;
    void SetMousePos(int x, int y) override;
    KeyEvent GetModKeyState() const override;
    void* GetMapPointer() const override;
    bool IsOpenGL() override { return false; }

    KeyEvent modKeyState_;
    unsigned long tickCount_;
};

#endif // MockupVideoDriver_h__
