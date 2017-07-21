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

#include "defines.h" // IWYU pragma: keep
#include "MockupVideoDriver.h"

MockupVideoDriver::MockupVideoDriver(VideoDriverLoaderInterface* CallBack):VideoDriver(CallBack), tickCount_(1)
{
    modKeyState_.kt = KT_INVALID;
    modKeyState_.c = 0;
    modKeyState_.ctrl = false;
    modKeyState_.shift = false;
    modKeyState_.alt = false;
}

const char* MockupVideoDriver::GetName() const
{
    return "Mockup Video Driver";
}

bool MockupVideoDriver::Initialize()
{
    return true;
}

bool MockupVideoDriver::CreateScreen(unsigned short width, unsigned short height, const bool fullscreen)
{
    return ResizeScreen(width, height, fullscreen);
}

bool MockupVideoDriver::ResizeScreen(unsigned short width, unsigned short height, const bool fullscreen)
{
    screenWidth = width;
    screenHeight = height;
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

void* MockupVideoDriver::GetFunction(const char* function) const
{
    return NULL;
}

void MockupVideoDriver::ListVideoModes(std::vector<VideoMode>& video_modes) const
{}

void MockupVideoDriver::SetMousePos(int x, int y)
{
    mouse_xy.x = x;
    mouse_xy.y = y;
}

KeyEvent MockupVideoDriver::GetModKeyState() const
{
    return modKeyState_;
}

void* MockupVideoDriver::GetMapPointer() const
{
    return NULL;
}
