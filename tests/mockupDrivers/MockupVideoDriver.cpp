// Copyright (C) 2005 - 2026 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "MockupVideoDriver.h"
#include <boost/nowide/iostream.hpp>
#include <iostream>

MockupVideoDriver::MockupVideoDriver(VideoDriverLoaderInterface* CallBack)
    : VideoDriver(CallBack), tickCount_(1), numTfinger_(0)
{
    modKeyState_.kt = KeyType::Invalid;
    modKeyState_.c = 0;
    modKeyState_.ctrl = false;
    modKeyState_.shift = false;
    modKeyState_.alt = false;
}

MockupVideoDriver::~MockupVideoDriver() = default;

const char* MockupVideoDriver::getName() const
{
    return "Mockup Video Driver";
}

bool MockupVideoDriver::Initialize()
{
    if(initialized)
        return true;
    initialized = true;
    return true;
}

bool MockupVideoDriver::CreateScreen(const std::string&, const VideoMode newSize, DisplayMode displayMode)
{
    ResizeScreen(newSize, displayMode);
    return true;
}

bool MockupVideoDriver::ResizeScreen(const VideoMode newSize, DisplayMode displayMode)
{
    setNewSize(newSize, Extent(newSize.width, newSize.height));
    displayMode_ = displayMode;
    return true;
}

bool MockupVideoDriver::MessageLoop()
{
    return true;
}

unsigned long MockupVideoDriver::getTickCount() const
{
    return tickCount_;
}

static void* dummyLoader(const char*)
{
    return nullptr;
}

OpenGL_Loader_Proc MockupVideoDriver::getLoaderFunction() const
{
    return dummyLoader;
}

std::vector<VideoMode> MockupVideoDriver::ListVideoModes() const
{
    return video_modes_;
}

void MockupVideoDriver::setMousePos(Position pos)
{
    mouse_xy.pos = pos;
}

KeyEvent MockupVideoDriver::getModKeyState() const
{
    return modKeyState_;
}

void* MockupVideoDriver::getMapPointer() const
{
    return nullptr;
}

void MockupVideoDriver::ShowErrorMessage(const std::string& title, const std::string& message)
{
    std::cerr << title << ": " << message << std::endl;
}

bool MockupVideoDriver::isTouchEvent() const
{
    return numTfinger_;
}
