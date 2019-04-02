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

#include "commonDefines.h" // IWYU pragma: keep
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

MockupVideoDriver::~MockupVideoDriver()
{
    if(initialized)
        SDL_QuitSubSystem(SDL_INIT_VIDEO);
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

bool MockupVideoDriver::CreateScreen(const std::string& title, const VideoMode& newSize, bool fullscreen)
{
    ResizeScreen(newSize, fullscreen);
    SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 1);

#if SDL_MAJOR_VERSION == 1
    RTTR_UNUSED(title);
    auto* window = SDL_SetVideoMode(2, 2, 32, SDL_HWSURFACE | SDL_NOFRAME | SDL_OPENGL);
#else
    auto* window = SDL_CreateWindow(title.c_str(), SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 2, 2, SDL_WINDOW_OPENGL);
#endif
    if(!window)
    {
        bnw::cerr << SDL_GetError() << std::endl;
        return false;
    }
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

OpenGL_Loader_Proc MockupVideoDriver::GetLoaderFunction() const
{
    return SDL_GL_GetProcAddress;
}

void MockupVideoDriver::ListVideoModes(std::vector<VideoMode>& /*video_modes*/) const {}

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
