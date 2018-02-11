// Copyright (c) 2005 - 2017 Settlers Freaks (sf-team at siedler25.org)
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
#include "VideoSDL.h"
#include "VideoDriverLoaderInterface.h"
#include "VideoInterface.h"
#include "helpers/containerUtils.h"
#include <boost/interprocess/smart_ptr/unique_ptr.hpp>
#include <boost/nowide/iostream.hpp>
#include <SDL.h>
#include <algorithm>

#ifdef _WIN32
#include "../../../../win32/s25clientResources.h"
#include "libutil/ucString.h"
#include <SDL_syswm.h>

namespace {
struct DeleterReleaseDC
{
    typedef HDC pointer;
    HWND wnd;
    DeleterReleaseDC(HWND wnd) : wnd(wnd) {}
    void operator()(HDC dc) const { ReleaseDC(wnd, dc); }
};
struct DeleterDeleteRC
{
    typedef HGLRC pointer;
    void operator()(HGLRC rc) const { wglDeleteContext(rc); }
};

} // namespace
#endif // _WIN32

/**
 *  Instanzierungsfunktion von @p VideoSDL.
 *
 *  @param[in] CallBack DriverCallback für Rückmeldungen.
 *
 *  @return liefert eine Instanz des jeweiligen Treibers
 */
DRIVERDLLAPI IVideoDriver* CreateVideoInstance(VideoDriverLoaderInterface* CallBack)
{
    return new VideoSDL(CallBack);
}

DRIVERDLLAPI void FreeVideoInstance(IVideoDriver* driver)
{
    delete driver;
}

/**
 *  Namensfunktion von @p VideoSDL.
 *
 *  @return liefert den Namen des Treibers.
 */
DRIVERDLLAPI const char* GetDriverName()
{
    return "(SDL) OpenGL via SDL-Library";
}

/** @class VideoSDL
 *
 *  Klasse für den SDL Videotreiber.
 */

/** @var VideoSDL::screen
 *
 *  Das Fenster-SDL-Surface.
 */

/**
 *  Konstruktor von @p VideoSDL.
 *
 *  @param[in] CallBack DriverCallback für Rückmeldungen.
 */
VideoSDL::VideoSDL(VideoDriverLoaderInterface* CallBack) : VideoDriver(CallBack), screen(NULL) {}

VideoSDL::~VideoSDL()
{
    CleanUp();
}

/**
 *  Funktion zum Auslesen des Treibernamens.
 *
 *  @return liefert den Treibernamen zurück
 */
const char* VideoSDL::GetName() const
{
    return GetDriverName();
}

/**
 *  Treiberinitialisierungsfunktion.
 *
 *  @return @p true bei Erfolg, @p false bei Fehler
 */
bool VideoSDL::Initialize()
{
    if(SDL_InitSubSystem(SDL_INIT_VIDEO) < 0)
    {
        fprintf(stderr, "%s\n", SDL_GetError());
        initialized = false;
        return false;
    }

    initialized = true;

    // Unicode-Support einschalten
    SDL_EnableUNICODE(1);

    // Key-Repeat einschalten
    SDL_EnableKeyRepeat(SDL_DEFAULT_REPEAT_DELAY, SDL_DEFAULT_REPEAT_INTERVAL);

    return initialized;
}

/**
 *  Treiberaufräumfunktion.
 */
void VideoSDL::CleanUp()
{
    if(!initialized)
        return;

    // Fenster vernichten
    SDL_QuitSubSystem(SDL_INIT_VIDEO);

    // nun sind wir nicht mehr initalisiert
    initialized = false;
}

/**
 *  Erstellt das Fenster mit entsprechenden Werten.
 *
 *  @param[in] width      Breite des Fensters
 *  @param[in] height     Höhe des Fensters
 *  @param[in] fullscreen Vollbildmodus ja oder nein
 *
 *  @return @p true bei Erfolg, @p false bei Fehler
 */
bool VideoSDL::CreateScreen(const std::string& title, const VideoMode& newSize, bool fullscreen)
{
    if(!initialized)
        return false;

    // TODO: Icon setzen

    // GL-Attribute setzen
    SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 1);

    // Set the video mode
    if(!SetVideoMode(newSize, fullscreen))
        return false;

    SDL_WM_SetCaption(title.c_str(), 0);

#ifdef _WIN32
    SetWindowTextW(GetConsoleWindow(), cvUTF8ToWideString(title).c_str());
#endif

    std::fill(keyboard.begin(), keyboard.end(), false);

    SDL_ShowCursor(SDL_DISABLE);

    return true;
}

/**
 *  Erstellt oder verändert das Fenster mit entsprechenden Werten.
 *
 *  @param[in] width      Breite des Fensters
 *  @param[in] height     Höhe des Fensters
 *  @param[in] fullscreen Vollbildmodus ja oder nein
 *
 *  @return @p true bei Erfolg, @p false bei Fehler
 *
 *  @todo Vollbildmodus ggf. wechseln
 */
bool VideoSDL::ResizeScreen(const VideoMode& newSize, bool fullscreen)
{
    if(!initialized)
        return false;

        // On windows the current ogl context gets destroyed. Hence we have to save it to avoid having to reinitialize all resources
        // Taken from http://www.bytehazard.com/articles/sdlres.html
#ifdef _WIN32
    SDL_SysWMinfo info;
    // get window handle from SDL
    SDL_VERSION(&info.version);
    if(SDL_GetWMInfo(&info) != 1)
    {
        PrintError("SDL_GetWMInfo #1 failed");
        return false;
    }

    // get device context handle
    boost::interprocess::unique_ptr<HDC, DeleterReleaseDC> tempDC(GetDC(info.window), DeleterReleaseDC(info.window));

    // create temporary context
    boost::interprocess::unique_ptr<HGLRC, DeleterDeleteRC> tempRC(wglCreateContext(tempDC.get()));
    if(!tempRC)
    {
        PrintError("wglCreateContext failed");
        return false;
    }

    // share resources to temporary context
    SetLastError(0);
    if(!wglShareLists(info.hglrc, tempRC.get()))
    {
        PrintError("wglShareLists #1 failed");
        return false;
    }

    // set video mode
    if(!SetVideoMode(newSize, fullscreen))
        return false;

    // previously used structure may possibly be invalid, to be sure we get it again
    SDL_VERSION(&info.version);
    if(SDL_GetWMInfo(&info) != 1)
    {
        PrintError("SDL_GetWMInfo #2 failed\n");
        return false;
    }

    // share resources to new SDL-created context
    if(!wglShareLists(tempRC.get(), info.hglrc))
    {
        PrintError("wglShareLists #2 failed\n");
        return false;
    }

    // success
    return true;
#else
    return SetVideoMode(newSize, fullscreen);
#endif // _WIN32
}

bool VideoSDL::SetVideoMode(const VideoMode& newSize, bool fullscreen)
{
    // putenv needs a char* not a const char* -.-
    static char CENTER_ENV[] = "SDL_VIDEO_CENTERED=center";
    static char UNCENTER_ENV[] = "SDL_VIDEO_CENTERED=";

    bool enteredWndMode = !screen || (isFullscreen_ && !fullscreen);

    screenSize_ = (fullscreen) ? FindClosestVideoMode(newSize) : newSize;

    if(enteredWndMode)
        SDL_putenv(CENTER_ENV);

    screen = SDL_SetVideoMode(screenSize_.width, screenSize_.height, 32,
                              SDL_HWSURFACE | SDL_OPENGL | (fullscreen ? SDL_FULLSCREEN : SDL_RESIZABLE));
    // Fallback to non-fullscreen
    if(!screen && fullscreen)
        screen = SDL_SetVideoMode(screenSize_.width, screenSize_.height, 32, SDL_HWSURFACE | SDL_OPENGL | SDL_RESIZABLE);

    if(enteredWndMode)
        SDL_putenv(UNCENTER_ENV);

    // Videomodus setzen
    if(!screen)
    {
        PrintError(SDL_GetError());
        return false;
    }

    isFullscreen_ = (screen->flags & SDL_FULLSCREEN) != 0;

#ifdef _WIN32
    // Grabbing input in fullscreen is very buggy on windows. For one the mouse might jump (#806) which can be fixed by patching SDL:
    // https://github.com/joncampbell123/dosbox-x/commit/f343dc2ec012699d04584b898925e3501a9b913c
    // But doing that on the build server is to much work as SDL2 has fixed that already. The work-around in user code (this file) did not work (#809)
    // Furthermore there is a bug (maybe SDL 1.2.14 only), that the cursor gets locked to the top left corner when alt-tab is used (#811)
    // So we force SDL to not grab the input
    if(isFullscreen_)
    {
        // This is a hack (flags should be read-only) but we cannot ungrab input in fullscreen mode
        screen->flags &= ~SDL_FULLSCREEN;
        SDL_WM_GrabInput(SDL_GRAB_OFF);
        screen->flags |= SDL_FULLSCREEN;
    }

    SDL_SysWMinfo info;
    // get window handle from SDL
    SDL_VERSION(&info.version);
    if(SDL_GetWMInfo(&info) == 1)
    {
        LPARAM icon = (LPARAM)LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_SYMBOL));
        SendMessage(info.window, WM_SETICON, ICON_BIG, icon);
        SendMessage(info.window, WM_SETICON, ICON_SMALL, icon);
    }
#endif // _WIN32

    return true;
}

void VideoSDL::PrintError(const std::string& msg)
{
    bnw::cerr << msg << std::endl;
}

void VideoSDL::HandlePaste()
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

/**
 *  Schliesst das Fenster.
 */
void VideoSDL::DestroyScreen()
{
    // Fenster schliessen
    CleanUp();
    Initialize();
}

/**
 *  Wechselt die OpenGL-Puffer.
 *
 *  @return @p true bei Erfolg, @p false bei Fehler
 */
bool VideoSDL::SwapBuffers()
{
    // Puffer wechseln
    SDL_GL_SwapBuffers();

    return true;
}

/**
 *  Die Nachrichtenschleife.
 *
 *  @return @p true bei Erfolg, @p false bei Fehler
 */
bool VideoSDL::MessageLoop()
{
    static bool mouseMoved = false;

    SDL_Event ev;
    while(SDL_PollEvent(&ev))
    {
        switch(ev.type)
        {
            default: break;

            case SDL_QUIT: return false;

            case SDL_ACTIVEEVENT:
                if((ev.active.state & SDL_APPACTIVE) && ev.active.gain && !isFullscreen_)
                {
                    // Window was restored. We need a resize to avoid a black screen
                    ResizeScreen(VideoMode(screenSize_.width, screenSize_.height), isFullscreen_);
                    CallBack->ScreenResized(screenSize_.width, screenSize_.height);
                }
                break;

            case SDL_VIDEORESIZE:
            {
                VideoMode newSize(ev.resize.w, ev.resize.h);
                if(newSize != screenSize_)
                {
                    ResizeScreen(newSize, isFullscreen_);
                    CallBack->ScreenResized(screenSize_.width, screenSize_.height);
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
                    case SDLK_BACKQUOTE: ev.key.keysym.unicode = '^'; break;
                    case SDLK_v:
                        if(SDL_GetModState() & KMOD_CTRL)
                        {
                            HandlePaste();
                            continue;
                        }
                        break;
                }

                /// Strg, Alt, usw gedrückt?
                if(ev.key.keysym.mod & KMOD_CTRL)
                    ke.ctrl = true;
                if(ev.key.keysym.mod & KMOD_SHIFT)
                    ke.shift = true;
                if(ev.key.keysym.mod & KMOD_ALT)
                    ke.alt = true;

                if(ke.kt == KT_INVALID)
                {
                    ke.kt = KT_CHAR;
                    ke.c = ev.key.keysym.unicode;
                }

                CallBack->Msg_KeyDown(ke);
            }
            break;
            case SDL_MOUSEBUTTONDOWN:
            {
                mouse_xy.x = ev.button.x;
                mouse_xy.y = ev.button.y;

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
            }
            break;
            case SDL_MOUSEBUTTONUP:
            {
                mouse_xy.x = ev.button.x;
                mouse_xy.y = ev.button.y;

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
                if(ev.button.button == SDL_BUTTON_WHEELUP)
                {
                    CallBack->Msg_WheelUp(mouse_xy);
                }
                if(ev.button.button == SDL_BUTTON_WHEELDOWN)
                {
                    CallBack->Msg_WheelDown(mouse_xy);
                }
            }
            break;
            case SDL_MOUSEMOTION:
            {
                // Handle only 1st mouse move
                if(!mouseMoved)
                {
                    mouse_xy.x = ev.motion.x;
                    mouse_xy.y = ev.motion.y;

                    CallBack->Msg_MouseMove(mouse_xy);
                    mouseMoved = true;
                }
            }
            break;
        }
    }

    mouseMoved = false;
    return true;
}

/**
 *  Funktion zum Auslesen des TickCounts.
 *
 *  @return liefert den TickCount
 */
unsigned long VideoSDL::GetTickCount() const
{
    return SDL_GetTicks();
}

/**
 *   Listet verfügbare Videomodi auf
 *
 *  @param[in,out] video_modes Der Vector mit den Videomodes
 */
void VideoSDL::ListVideoModes(std::vector<VideoMode>& video_modes) const
{
    SDL_Rect** modes = SDL_ListModes(NULL, SDL_FULLSCREEN | SDL_HWSURFACE);

    for(unsigned i = 0; modes[i]; ++i)
    {
        VideoMode vm(modes[i]->w, modes[i]->h);
        if(!helpers::contains(video_modes, vm))
            video_modes.push_back(vm);
    }
}

/**
 *  Funktion zum Holen einer Subfunktion.
 *
 *  @param[in] function Name der Funktion welche geholt werden soll.
 *
 *  @return Adresse der Funktion bei Erfolg, @p NULL bei Fehler
 */
void* VideoSDL::GetFunction(const char* function) const
{
    return SDL_GL_GetProcAddress(function);
}

/**
 *  Funktion zum Setzen der Mauskoordinaten.
 *
 *  @param[in] x X-Koordinate
 *  @param[in] y Y-Koordinate
 */
void VideoSDL::SetMousePos(int x, int y)
{
    mouse_xy.x = x;
    mouse_xy.y = y;
    SDL_WarpMouse(x, y);
}

/**
 *  Get state of the modifier keys
 */
KeyEvent VideoSDL::GetModKeyState() const
{
    const SDLMod modifiers = SDL_GetModState();
    const KeyEvent ke = {KT_INVALID, 0, ((modifiers & KMOD_CTRL) != 0), ((modifiers & KMOD_SHIFT) != 0), ((modifiers & KMOD_ALT) != 0)};
    return ke;
}

/// Gibt Pointer auf ein Fenster zurück (device-dependent!), HWND unter Windows
void* VideoSDL::GetMapPointer() const
{
#ifdef WIN32
    SDL_SysWMinfo wmInfo;
    SDL_VERSION(&wmInfo.version);
    SDL_GetWMInfo(&wmInfo);
    // return (void*)wmInfo.info.win.window;
    return (void*)wmInfo.window;
#else
    return NULL;
#endif
}
