// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "VideoSDL2.h"
#include "driver/Interface.h"
#include "driver/VideoDriverLoaderInterface.h"
#include "driver/VideoInterface.h"
#include "enum_cast.hpp"
#include "helpers/LSANUtils.h"
#include "helpers/containerUtils.h"
#include "icon.h"
#include "openglCfg.hpp"
#include <s25util/utf8.h>
#include <boost/nowide/iostream.hpp>
#include <SDL.h>
#include <algorithm>
#include <memory>

#ifdef _WIN32
#    include <boost/nowide/convert.hpp>
#    include <SDL_syswm.h>
#endif // _WIN32

#define CHECK_SDL(call)                 \
    do                                  \
    {                                   \
        if((call) == -1)                \
            PrintError(SDL_GetError()); \
    } while(false)

namespace {
template<typename T>
struct SDLMemoryDeleter
{
    void operator()(T* p) const { SDL_free(p); }
};

template<typename T>
using SDL_memory = std::unique_ptr<T, SDLMemoryDeleter<T>>;
} // namespace

IVideoDriver* CreateVideoInstance(VideoDriverLoaderInterface* CallBack)
{
    return new VideoSDL2(CallBack);
}

void FreeVideoInstance(IVideoDriver* driver)
{
    delete driver;
}

const char* GetDriverName()
{
    return "(SDL2) OpenGL via SDL2-Library";
}

VideoSDL2::VideoSDL2(VideoDriverLoaderInterface* CallBack) : VideoDriver(CallBack), window(nullptr), context(nullptr) {}

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
    rttr::ScopedLeakDisabler _;
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
    SDL_Quit();
    initialized = false;
}

void VideoSDL2::UpdateCurrentSizes()
{
    int w, h, w2, h2;
    SDL_GetWindowSize(window, &w, &h);
    SDL_GL_GetDrawableSize(window, &w2, &h2);
    SetNewSize(VideoMode(w, h), Extent(w2, h2));
}

bool VideoSDL2::CreateScreen(const std::string& title, const VideoMode& size, DisplayMode displayMode)
{
    if(!initialized)
        return false;

    // GL-Attributes
    CHECK_SDL(SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, RTTR_OGL_MAJOR));
    CHECK_SDL(SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, RTTR_OGL_MINOR));
    SDL_GLprofile profile;
    if((RTTR_OGL_ES))
        profile = SDL_GL_CONTEXT_PROFILE_ES;
    else if((RTTR_OGL_COMPAT))
        profile = SDL_GL_CONTEXT_PROFILE_COMPATIBILITY;
    else
        profile = SDL_GL_CONTEXT_PROFILE_CORE;
    CHECK_SDL(SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, profile));

    CHECK_SDL(SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8));
    CHECK_SDL(SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8));
    CHECK_SDL(SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8));
    CHECK_SDL(SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1));

    const int wndPos = SDL_WINDOWPOS_CENTERED;
    const auto fullscreen = (displayMode & DisplayMode::Fullscreen) != DisplayMode::None;
    const auto resizable = (displayMode & DisplayMode::Resizable) != DisplayMode::None;
    const auto requestedSize = fullscreen ? FindClosestVideoMode(size) : size;
    const unsigned commonFlags = SDL_WINDOW_OPENGL | (resizable ? SDL_WINDOW_RESIZABLE : 0);
    const unsigned fullscreenFlag = (fullscreen ? SDL_WINDOW_FULLSCREEN : 0);

    window = SDL_CreateWindow(title.c_str(), wndPos, wndPos, requestedSize.width, requestedSize.height,
                              commonFlags | fullscreenFlag);

    // Fallback to non-fullscreen
    if(!window && fullscreen)
        window =
          SDL_CreateWindow(title.c_str(), wndPos, wndPos, requestedSize.width, requestedSize.height, commonFlags);

    if(!window)
    {
        PrintError(SDL_GetError());
        return false;
    }

    const auto flags = SDL_GetWindowFlags(window);
    SetFullscreenFlag((flags & SDL_WINDOW_FULLSCREEN) != 0);
    SetResizableFlag((flags & SDL_WINDOW_RESIZABLE) != 0);
    UpdateCurrentSizes();

    if(!IsFullscreen())
        MoveWindowToCenter();

    SDL_Surface* iconSurf =
      SDL_CreateRGBSurfaceFrom(image.data(), 48, 48, 32, 48 * 4, 0xFF000000, 0x00FF0000, 0x0000FF00, 0x000000FF);
    if(iconSurf)
    {
        SDL_SetWindowIcon(window, iconSurf);
        SDL_FreeSurface(iconSurf);
    } else
        PrintError(SDL_GetError());

    context = SDL_GL_CreateContext(window);

#ifdef _WIN32
    SetWindowTextW(GetConsoleWindow(), boost::nowide::widen(title).c_str());
#endif

    std::fill(keyboard.begin(), keyboard.end(), false);

    SDL_ShowCursor(0);

    return true;
}

bool VideoSDL2::ResizeScreen(const VideoMode& newSize, DisplayMode displayMode)
{
    if(!initialized)
        return false;

    const auto fullscreen = (displayMode & DisplayMode::Fullscreen) != DisplayMode::None;
    const auto resizable = (displayMode & DisplayMode::Resizable) != DisplayMode::None;

    if(IsFullscreen() != fullscreen)
    {
        SDL_SetWindowFullscreen(window, fullscreen ? SDL_WINDOW_FULLSCREEN : 0);
        SetFullscreenFlag((SDL_GetWindowFlags(window) & SDL_WINDOW_FULLSCREEN) != 0);
        if(!IsFullscreen())
            MoveWindowToCenter();
    }

    if(displayMode_ != displayMode)
    {
        if(!IsFullscreen())
#if SDL_VERSION_ATLEAST(2, 0, 5)
            SDL_SetWindowResizable(window, static_cast<SDL_bool>(resizable));
#endif
        SetResizableFlag((SDL_GetWindowFlags(window) & SDL_WINDOW_RESIZABLE) != 0);
    }

    if(newSize != GetWindowSize())
    {
        if(IsFullscreen())
        {
            auto const targetMode = FindClosestVideoMode(newSize);
            SDL_DisplayMode target;
            target.w = targetMode.width;
            target.h = targetMode.height;
            target.format = 0;           // don't care
            target.refresh_rate = 0;     // don't care
            target.driverdata = nullptr; // initialize to 0
            // Explicitly change the window size to avoid a bug with SDL reporting the wrong size until alt+tab
            SDL_SetWindowSize(window, target.w, target.h);
            if(SDL_SetWindowDisplayMode(window, &target) < 0)
            {
                PrintError(SDL_GetError());
                return false;
            }
        } else
        {
            SDL_SetWindowSize(window, newSize.width, newSize.height);
        }
        UpdateCurrentSizes();
    }

    return true;
}

void VideoSDL2::PrintError(const std::string& msg) const
{
    boost::nowide::cerr << msg << std::endl;
}

void VideoSDL2::ShowErrorMessage(const std::string& title, const std::string& message)
{
    // window==nullptr is okay too ("no parent")
    SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, title.c_str(), message.c_str(), window);
}

void VideoSDL2::HandlePaste()
{
    if(!SDL_HasClipboardText())
        return;

    SDL_memory<char> text(SDL_GetClipboardText());
    if(!text || *text == '\0') // empty string indicates error
        PrintError(text ? SDL_GetError() : "Paste failed.");

    KeyEvent ke = {KeyType::Char, 0, false, false, false};
    for(const char32_t c : s25util::utf8to32(text.get()))
    {
        ke.c = static_cast<unsigned>(c);
        CallBack->Msg_KeyDown(ke);
    }
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
                        SetFullscreenFlag((SDL_GetWindowFlags(window) & SDL_WINDOW_FULLSCREEN) != 0);
                        VideoMode newSize(ev.window.data1, ev.window.data2);
                        if(newSize != GetWindowSize())
                        {
                            UpdateCurrentSizes();
                            CallBack->WindowResized();
                        }
                    }
                    break;
                }
            }
            break;

            case SDL_KEYDOWN:
            {
                KeyEvent ke = {KeyType::Invalid, 0, false, false, false};

                switch(ev.key.keysym.sym)
                {
                    default:
                    {
                        // Die 12 F-Tasten
                        if(ev.key.keysym.sym >= SDLK_F1 && ev.key.keysym.sym <= SDLK_F12)
                            ke.kt = static_cast<KeyType>(rttr::enum_cast(KeyType::F1) + ev.key.keysym.sym - SDLK_F1);
                    }
                    break;
                    case SDLK_RETURN: ke.kt = KeyType::Return; break;
                    case SDLK_SPACE: ke.kt = KeyType::Space; break;
                    case SDLK_LEFT: ke.kt = KeyType::Left; break;
                    case SDLK_RIGHT: ke.kt = KeyType::Right; break;
                    case SDLK_UP: ke.kt = KeyType::Up; break;
                    case SDLK_DOWN: ke.kt = KeyType::Down; break;
                    case SDLK_BACKSPACE: ke.kt = KeyType::Backspace; break;
                    case SDLK_DELETE: ke.kt = KeyType::Delete; break;
                    case SDLK_LSHIFT:
                    case SDLK_RSHIFT: ke.kt = KeyType::Shift; break;
                    case SDLK_TAB: ke.kt = KeyType::Tab; break;
                    case SDLK_HOME: ke.kt = KeyType::Home; break;
                    case SDLK_END: ke.kt = KeyType::End; break;
                    case SDLK_ESCAPE: ke.kt = KeyType::Escape; break;
                    case SDLK_PRINTSCREEN: ke.kt = KeyType::Print; break;
                    // case SDLK_BACKQUOTE: ev.key.keysym.scancode = '^'; break;
                    case SDLK_v:
                        if(SDL_GetModState() & KMOD_CTRL)
                        {
                            HandlePaste();
                            continue;
                        }
                        break;
                }

                if(ke.kt == KeyType::Invalid)
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
                const std::u32string text = s25util::utf8to32(ev.text.text);
                SDL_Keymod mod = SDL_GetModState();
                KeyEvent ke = {KeyType::Char, 0, (mod & KMOD_CTRL) != 0, (mod & KMOD_SHIFT) != 0,
                               (mod & KMOD_ALT) != 0};
                for(char32_t c : text)
                {
                    ke.c = static_cast<unsigned>(c);
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
#if SDL_VERSION_ATLEAST(2, 0, 4)
                if(ev.wheel.direction == SDL_MOUSEWHEEL_FLIPPED)
                    y = -y;
#endif
                if(y > 0)
                    CallBack->Msg_WheelUp(mouse_xy);
                else if(y < 0)
                    CallBack->Msg_WheelDown(mouse_xy);
            }
            break;
            case SDL_MOUSEMOTION:
                mouse_xy.pos = Position(ev.motion.x, ev.motion.y);
                CallBack->Msg_MouseMove(mouse_xy);
                break;
        }
    }

    return true;
}

unsigned long VideoSDL2::GetTickCount() const
{
    return SDL_GetTicks();
}

void VideoSDL2::ListVideoModes(std::vector<VideoMode>& video_modes) const
{
    int display = SDL_GetWindowDisplayIndex(window);
    if(display < 0)
        display = 0;
    for(int i = SDL_GetNumDisplayModes(display) - 1; i >= 0; --i)
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

void VideoSDL2::SetMousePos(Position pos)
{
    mouse_xy.pos = pos;
    SDL_WarpMouseInWindow(window, pos.x, pos.y);
}

KeyEvent VideoSDL2::GetModKeyState() const
{
    const SDL_Keymod modifiers = SDL_GetModState();
    const KeyEvent ke = {KeyType::Invalid, 0, ((modifiers & KMOD_CTRL) != 0), ((modifiers & KMOD_SHIFT) != 0),
                         ((modifiers & KMOD_ALT) != 0)};
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
    return nullptr;
#endif
}

void VideoSDL2::MoveWindowToCenter()
{
    SDL_Rect usableBounds;
#if SDL_VERSION_ATLEAST(2, 0, 5)
    CHECK_SDL(SDL_GetDisplayUsableBounds(SDL_GetWindowDisplayIndex(window), &usableBounds));
    int top, left, bottom, right;
    CHECK_SDL(SDL_GetWindowBordersSize(window, &top, &left, &bottom, &right));
    usableBounds.w -= left + right;
    usableBounds.h -= top + bottom;
#else
    CHECK_SDL(SDL_GetDisplayBounds(SDL_GetWindowDisplayIndex(window), &usableBounds));
    // rough estimates
    usableBounds.w -= 10;
    usableBounds.h -= 30;
#endif
    if(usableBounds.w < GetWindowSize().width || usableBounds.h < GetWindowSize().height)
    {
        SDL_SetWindowSize(window, usableBounds.w, usableBounds.h);
        UpdateCurrentSizes();
    }
    SDL_SetWindowPosition(window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
}
