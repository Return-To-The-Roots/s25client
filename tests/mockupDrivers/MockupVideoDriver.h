// Copyright (C) 2005 - 2026 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "driver/VideoDriver.h"

class MockupVideoDriver : public VideoDriver
{
public:
    MockupVideoDriver(VideoDriverLoaderInterface* CallBack);
    ~MockupVideoDriver() override;
    const char* getName() const override;
    bool Initialize() override;
    bool CreateScreen(const std::string& title, VideoMode newSize, DisplayMode displayMode) override;
    bool ResizeScreen(VideoMode newSize, DisplayMode displayMode) override;
    void DestroyScreen() override {}
    bool SwapBuffers() override { return true; }
    bool MessageLoop() override;
    unsigned long getTickCount() const override;
    OpenGL_Loader_Proc getLoaderFunction() const override;
    std::vector<VideoMode> ListVideoModes() const override;
    void setMousePos(Position pos) override;
    KeyEvent getModKeyState() const override;
    void* getMapPointer() const override;
    bool isOpenGL() const override { return false; }
    void ShowErrorMessage(const std::string& title, const std::string& message) override;
    using VideoDriver::FindClosestVideoMode;
    using VideoDriver::setNewSize;
    bool isTouchEvent() const override;

    KeyEvent modKeyState_;
    unsigned long tickCount_;
    unsigned numTfinger_;
    std::vector<VideoMode> video_modes_;
};
