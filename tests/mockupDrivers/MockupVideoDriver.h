// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "driver/VideoDriver.h"

class MockupVideoDriver : public VideoDriver
{
public:
    MockupVideoDriver(VideoDriverLoaderInterface* CallBack);
    ~MockupVideoDriver() override;
    const char* GetName() const override;
    bool Initialize() override;
    bool CreateScreen(const std::string& title, const VideoMode& newSize, DisplayMode displayMode) override;
    bool ResizeScreen(const VideoMode& newSize, DisplayMode displayMode) override;
    void DestroyScreen() override {}
    bool SwapBuffers() override { return true; }
    bool MessageLoop() override;
    unsigned long GetTickCount() const override;
    OpenGL_Loader_Proc GetLoaderFunction() const override;
    void ListVideoModes(std::vector<VideoMode>& video_modes) const override;
    void SetMousePos(Position pos) override;
    KeyEvent GetModKeyState() const override;
    void* GetMapPointer() const override;
    bool IsOpenGL() const override { return false; }
    void ShowErrorMessage(const std::string& title, const std::string& message) override;
    using VideoDriver::FindClosestVideoMode;

    KeyEvent modKeyState_;
    unsigned long tickCount_;
    std::vector<VideoMode> video_modes_;
};
