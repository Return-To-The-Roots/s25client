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

#include "driverDefines.h" // IWYU pragma: keep
#include "VideoSDL2.h"
#include "VideoDriverLoaderInterface.h"
#include "VideoInterface.h"
#include "helpers/containerUtils.h"
#include "icon.h"
#include "openglCfg.hpp"
#include "libutf8/utf8.h"
#include <boost/interprocess/smart_ptr/unique_ptr.hpp>
#include <boost/nowide/iostream.hpp>
#include <SDL.h>
#include <algorithm>

#ifdef _WIN32
#include "libutil/ucString.h"
#include <SDL_syswm.h>
#endif // _WIN32

#define CHECK_SDL(call)                 \
    do                                  \
    {                                   \
        if((call) == -1)                \
            PrintError(SDL_GetError()); \
    } while(false)

DRIVERDLLAPI IVideoDriver* CreateVideoInstance(VideoDriverLoaderInterface* CallBack)
{
    return new VideoSDL2(CallBack);
}

DRIVERDLLAPI void FreeVideoInstance(IVideoDriver* driver)
{
    delete driver;
}

DRIVERDLLAPI const char* GetDriverName()
{
    return "(SDL2) OpenGL via SDL2-Library";
}

VideoSDL2::VideoSDL2(VideoDriverLoaderInterface* CallBack) : VideoDriver(CallBack), window(NULL) {}

VideoSDL2::~VideoSDL2()
{
    CleanUp();
}

const char* VideoSDL2::GetName() const
{
    return GetDriverName();
}

bool VideoSDL2::Initialize()
{
    initialized = false;
    if(SDL_InitSubSystem(SDL_INIT_VIDEO) < 0)
    {
        PrintError(SDL_GetError());
        return false;
    }

    initialized = true;
    return initialized;
}

void VideoSDL2::CleanUp()
{
    if(!initialized)
        return;

    if(context)
        SDL_GL_DeleteContext(context);
    if(window)
        SDL_DestroyWindow(window);
    SDL_QuitSubSystem(SDL_INIT_VIDEO);
    initialized = false;
}

bool VideoSDL2::CreateScreen(const std::string& title, const VideoMode& newSize, bool fullscreen)
{
    if(!initialized)
        return false;

    // GL-Attributes
    CHECK_SDL(SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, RTTR_OGL_MAJOR));
    CHECK_SDL(SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, RTTR_OGL_MINOR));
    SDL_GLprofile profile;
    if(RTTR_OGL_ES)
        profile = SDL_GL_CONTEXT_PROFILE_ES;
    else if(RTTR_OGL_COMPAT)
        profile = SDL_GL_CONTEXT_PROFILE_COMPATIBILITY;
    else
        profile = SDL_GL_CONTEXT_PROFILE_CORE;
    CHECK_SDL(SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, profile));

    CHECK_SDL(SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8));
    CHECK_SDL(SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8));
    CHECK_SDL(SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8));
    CHECK_SDL(SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1));

    int wndPos = SDL_WINDOWPOS_CENTERED;

    screenSize_ = (fullscreen) ? FindClosestVideoMode(newSize) : newSize;

    window = SDL_CreateWindow(title.c_str(), wndPos, wndPos, newSize.width, newSize.height,
                              SDL_WINDOW_OPENGL | (fullscreen ? SDL_WINDOW_FULLSCREEN : SDL_WINDOW_RESIZABLE));

    // Fallback to non-fullscreen
    if(!window && fullscreen)
    {
        window = SDL_CreateWindow(title.c_str(), wndPos, wndPos, newSize.width, newSize.height,
                                  SDL_WINDOW_OPENGL | (fullscreen ? SDL_WINDOW_FULLSCREEN : SDL_WINDOW_RESIZABLE));
    }

    if(!window)
    {
        PrintError(SDL_GetError());
        return false;
    }

    isFullscreen_ = (SDL_GetWindowFlags(window) & SDL_WINDOW_FULLSCREEN) != 0;

    SDL_Surface* iconSurf = SDL_CreateRGBSurfaceFrom(image, 48, 48, 32, 48 * 4, 0xFF000000, 0x00FF0000, 0x0000FF00, 0x000000FF);
    if(iconSurf)
    {
        SDL_SetWindowIcon(window, iconSurf);
        SDL_FreeSurface(iconSurf);
    } else
        PrintError(SDL_GetError());

    context = SDL_GL_CreateContext(window);

#ifdef _WIN32
    SetWindowTextW(GetConsoleWindow(), cvUTF8ToWideString(title).c_str());
#endif

    std::fill(keyboard.begin(), keyboard.end(), false);

    SDL_ShowCursor(0);

    return true;
}

bool VideoSDL2::ResizeScreen(const VideoMode& newSize, bool fullscreen)
{
    if(!initialized)
        return false;

    if(isFullscreen_ != fullscreen)
    {
        SDL_SetWindowFullscreen(window, fullscreen ? SDL_WINDOW_FULLSCREEN : 0);
        isFullscreen_ = (SDL_GetWindowFlags(window) & SDL_WINDOW_FULLSCREEN) != 0;
    }

    if(newSize != screenSize_)
    {
        if(isFullscreen_)
        {
            SDL_DisplayMode target, closest;
            target.w = newSize.width;
            target.h = newSize.height;
            target.format = 0;       // don't care
            target.refresh_rate = 0; // don't care
            target.driverdata = 0;   // initialize to 0
            if(!SDL_GetClosestDisplayMode(SDL_GetWindowDisplayIndex(window), &target, &closest))
            {
                PrintError(SDL_GetError());
                return false;
            }
            if(SDL_SetWindowDisplayMode(window, &closest) < 0)
            {
                PrintError(SDL_GetError());
                return false;
            }
            screenSize_ = VideoMode(closest.w, closest.h);
        } else
        {
            SDL_SetWindowSize(window, newSize.width, newSize.height);
        }
    }
    return true;
}

void VideoSDL2::PrintError(const std::string& msg) const
{
    bnw::cerr << msg << std::endl;
}

void VideoSDL2::HandlePaste()
{
#ifdef _WIN32
    if(!IsClipboardFormatAvailable(CF_UNICODETEXT))
        return;

    OpenClipboard(NULL);

    HANDLE hData = GetClipboardData(CF_UNICODETEXT);
    const wchar_t* pData = (const wchar_t*)GlobalLock(hData);

    KeyEvent ke = {KT_INVALID, 0, false, false, false};
    while(pData && *pData)
    {
        ke.c = *(pData++);
        if(ke.c == L' ')
            ke.kt = KT_SPACE;
        else
            ke.kt = KT_CHAR;
        CallBack->Msg_KeyDown(ke);
    }

    GlobalUnlock(hData);
    CloseClipboard();
#endif
}

void VideoSDL2::DestroyScreen()
{
    CleanUp();
}

bool VideoSDL2::SwapBuffers()
{
    SDL_GL_SwapWindow(window);
    return true;
}

bool VideoSDL2::MessageLoop()
{
    static bool mouseMoved = false;

    SDL_Event ev;
    while(SDL_PollEvent(&ev))
    {
        switch(ev.type)
        {
            default: break;

            case SDL_QUIT: return false;
            case SDL_WINDOWEVENT:
            {
                switch(ev.window.event)
                {
                    case SDL_WINDOWEVENT_RESIZED:
                    {
                        VideoMode newSize(ev.window.data1, ev.window.data2);
                        if(newSize != screenSize_)
                        {
                            ResizeScreen(newSize, isFullscreen_);
                            CallBack->ScreenResized(screenSize_.width, screenSize_.height);
                        }
                    }
                    break;
                }
            }
            break;

            case SDL_KEYDOWN:
            {
                KeyEvent ke = {KT_INVALID, 0, false, false, false};

                switch(ev.key.keysym.sym)
                {
                    default:
                    {
                        // Die 12 F-Tasten
                        if(ev.key.keysym.sym >= SDLK_F1 && ev.key.keysym.sym <= SDLK_F12)
                            ke.kt = static_cast<KeyType>(KT_F1 + ev.key.keysym.sym - SDLK_F1);
                    }
                    break;
                    case SDLK_RETURN: ke.kt = KT_RETURN; break;
                    case SDLK_SPACE: ke.kt = KT_SPACE; break;
                    case SDLK_LEFT: ke.kt = KT_LEFT; break;
                    case SDLK_RIGHT: ke.kt = KT_RIGHT; break;
                    case SDLK_UP: ke.kt = KT_UP; break;
                    case SDLK_DOWN: ke.kt = KT_DOWN; break;
                    case SDLK_BACKSPACE: ke.kt = KT_BACKSPACE; break;
                    case SDLK_DELETE: ke.kt = KT_DELETE; break;
                    case SDLK_LSHIFT: ke.kt = KT_SHIFT; break;
                    case SDLK_RSHIFT: ke.kt = KT_SHIFT; break;
                    case SDLK_TAB: ke.kt = KT_TAB; break;
                    case SDLK_HOME: ke.kt = KT_HOME; break;
                    case SDLK_END: ke.kt = KT_END; break;
                    case SDLK_ESCAPE: ke.kt = KT_ESCAPE; break;
                    case SDLK_PRINTSCREEN:
                        ke.kt = KT_PRINT;
                        break;
                    // case SDLK_BACKQUOTE: ev.key.keysym.scancode = '^'; break;
                    case SDLK_v:
                        if(SDL_GetModState() & KMOD_CTRL)
                        {
                            HandlePaste();
                            continue;
                        }
                        break;
                }

                if(ke.kt == KT_INVALID)
                    break;

                /// Strg, Alt, usw gedrückt?
                if(ev.key.keysym.mod & KMOD_CTRL)
                    ke.ctrl = true;
                if(ev.key.keysym.mod & KMOD_SHIFT)
                    ke.shift = true;
                if(ev.key.keysym.mod & KMOD_ALT)
                    ke.alt = true;

                CallBack->Msg_KeyDown(ke);
            }
            break;
            case SDL_TEXTINPUT:
            {
                typedef utf8::iterator<std::string::const_iterator> iterator;
                const std::string text = ev.text.text;
                iterator end(text.end(), text.begin(), text.end());
                SDL_Keymod mod = SDL_GetModState();
                KeyEvent ke = {KT_CHAR, 0, (mod & KMOD_CTRL) != 0, (mod & KMOD_SHIFT) != 0, (mod & KMOD_ALT) != 0};
                for(iterator it(text.begin(), text.begin(), text.end()); it != end; ++it)
                {
                    ke.c = *it;
                    CallBack->Msg_KeyDown(ke);
                }
                break;
            }
            case SDL_MOUSEBUTTONDOWN:
                mouse_xy.pos = Position(ev.button.x, ev.button.y);

                if(/*!mouse_xy.ldown && */ ev.button.button == SDL_BUTTON_LEFT)
                {
                    mouse_xy.ldown = true;
                    CallBack->Msg_LeftDown(mouse_xy);
                }
                if(/*!mouse_xy.rdown &&*/ ev.button.button == SDL_BUTTON_RIGHT)
                {
                    mouse_xy.rdown = true;
                    CallBack->Msg_RightDown(mouse_xy);
                }
                break;
            case SDL_MOUSEBUTTONUP:
                mouse_xy.pos = Position(ev.button.x, ev.button.y);

                if(/*mouse_xy.ldown &&*/ ev.button.button == SDL_BUTTON_LEFT)
                {
                    mouse_xy.ldown = false;
                    CallBack->Msg_LeftUp(mouse_xy);
                }
                if(/*mouse_xy.rdown &&*/ ev.button.button == SDL_BUTTON_RIGHT)
                {
                    mouse_xy.rdown = false;
                    CallBack->Msg_RightUp(mouse_xy);
                }
                break;
            case SDL_MOUSEWHEEL:
            {
                int y = ev.wheel.y;
                if(ev.wheel.direction == SDL_MOUSEWHEEL_FLIPPED)
                    y = -y;
                if(y < 0)
                    CallBack->Msg_WheelUp(mouse_xy);
                else if(y > 0)
                    CallBack->Msg_WheelDown(mouse_xy);
            }
            break;
            case SDL_MOUSEMOTION:
                // Handle only 1st mouse move
                if(!mouseMoved)
                {
                    mouse_xy.pos = Position(ev.button.x, ev.button.y);

                    CallBack->Msg_MouseMove(mouse_xy);
                    mouseMoved = true;
                }
                break;
        }
    }

    mouseMoved = false;
    return true;
}

unsigned long VideoSDL2::GetTickCount() const
{
    return SDL_GetTicks();
}

void VideoSDL2::ListVideoModes(std::vector<VideoMode>& video_modes) const
{
    int display = SDL_GetWindowDisplayIndex(window);
    for(int i = SDL_GetNumDisplayModes(display); i > 0; --i)
    {
        SDL_DisplayMode mode;
        if(SDL_GetDisplayMode(display, i, &mode) != 0)
            PrintError(SDL_GetError());
        else
        {
            VideoMode vm(mode.w, mode.h);
            if(!helpers::contains(video_modes, vm))
                video_modes.push_back(vm);
        }
    }
}

OpenGL_Loader_Proc VideoSDL2::GetLoaderFunction() const
{
    return SDL_GL_GetProcAddress;
}

void VideoSDL2::SetMousePos(int x, int y)
{
    mouse_xy.pos = Position(x, y);
    SDL_WarpMouseInWindow(window, x, y);
}

KeyEvent VideoSDL2::GetModKeyState() const
{
    const SDL_Keymod modifiers = SDL_GetModState();
    const KeyEvent ke = {KT_INVALID, 0, ((modifiers & KMOD_CTRL) != 0), ((modifiers & KMOD_SHIFT) != 0), ((modifiers & KMOD_ALT) != 0)};
    return ke;
}

void* VideoSDL2::GetMapPointer() const
{
#ifdef WIN32
    SDL_SysWMinfo wmInfo;
    SDL_VERSION(&wmInfo.version);
    SDL_GetWindowWMInfo(window, &wmInfo);
    // return (void*)wmInfo.info.win.window;
    return (void*)wmInfo.info.win.window;
#else
    return NULL;
#endif
}
