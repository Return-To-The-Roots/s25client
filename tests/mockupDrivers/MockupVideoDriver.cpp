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

#include "MockupVideoDriver.h"
#include <boost/nowide/iostream.hpp>

MockupVideoDriver::MockupVideoDriver(VideoDriverLoaderInterface* CallBack) : VideoDriver(CallBack), tickCount_(1)
{
    modKeyState_.kt = KeyType::Invalid;
    modKeyState_.c = 0;
    modKeyState_.ctrl = false;
    modKeyState_.shift = false;
    modKeyState_.alt = false;
}

MockupVideoDriver::~MockupVideoDriver() = default;

const char* MockupVideoDriver::GetName() const
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

bool MockupVideoDriver::CreateScreen(const std::string&, const VideoMode& newSize, bool fullscreen)
{
    ResizeScreen(newSize, fullscreen);
    return true;
}

bool MockupVideoDriver::ResizeScreen(const VideoMode& newSize, bool fullscreen)
{
    SetNewSize(newSize, Extent(newSize.width, newSize.height));
    isFullscreen_ = fullscreen;
    return true;
}

bool MockupVideoDriver::MessageLoop()
{
    return true;
}

unsigned long MockupVideoDriver::GetTickCount() const
{
    return tickCount_;
}

static void* dummyLoader(const char*)
{
    return nullptr;
}

OpenGL_Loader_Proc MockupVideoDriver::GetLoaderFunction() const
{
    return dummyLoader;
}

void MockupVideoDriver::ListVideoModes(std::vector<VideoMode>& video_modes) const
{
    video_modes = video_modes_;
}

void MockupVideoDriver::SetMousePos(Position pos)
{
    mouse_xy.pos = pos;
}

KeyEvent MockupVideoDriver::GetModKeyState() const
{
    return modKeyState_;
}

void* MockupVideoDriver::GetMapPointer() const
{
    return nullptr;
}
