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

#include "rttrDefines.h" // IWYU pragma: keep
#include "MockupVideoDriver.h"
#include <boost/nowide/iostream.hpp>
#include <SDL.h>

MockupVideoDriver::MockupVideoDriver(VideoDriverLoaderInterface* CallBack) : VideoDriver(CallBack), tickCount_(1)
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
    if(initialized)
        return true;
    if(SDL_InitSubSystem(SDL_INIT_VIDEO) < 0)
    {
        fprintf(stderr, "%s\n", SDL_GetError());
        return false;
    }
    initialized = true;
    return true;
}

void MockupVideoDriver::CleanUp()
{
    if(!initialized)
        return;
    SDL_QuitSubSystem(SDL_INIT_VIDEO);
    initialized = false;
}

bool MockupVideoDriver::CreateScreen(const std::string& title, const VideoMode& newSize, bool fullscreen)
{
    ResizeScreen(newSize, fullscreen);
    SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 1);

    SDL_Surface* screen;

    if(!(screen = SDL_SetVideoMode(2, 2, 32, SDL_HWSURFACE | SDL_NOFRAME | SDL_OPENGL)))
    {
        bnw::cerr << SDL_GetError() << std::endl;
        return false;
    }
    return true;
}

bool MockupVideoDriver::ResizeScreen(const VideoMode& newSize, bool fullscreen)
{
    screenSize_ = newSize;
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

void MockupVideoDriver::ListVideoModes(std::vector<VideoMode>& video_modes) const {}

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
