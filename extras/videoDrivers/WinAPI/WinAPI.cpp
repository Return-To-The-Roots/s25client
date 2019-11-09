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

#include "commonDefines.h" // IWYU pragma: keep
#include "WinAPI.h"
#include "driver/Interface.h"
#include "driver/VideoDriverLoaderInterface.h"
#include "driver/VideoInterface.h"
#include "helpers/containerUtils.h"
#include "s25clientResources.h"
#include "s25util/ucString.h"
#include <GL/gl.h>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <limits>

/**
 *  Zeiger auf die aktuelle Instanz.
 */
static VideoWinAPI* pVideoWinAPI = nullptr;

/**
 *  Instanzierungsfunktion von @p VideoWinAPI.
 *
 *  @param[in] CallBack DriverCallback für Rückmeldungen.
 *
 *  @return liefert eine Instanz des jeweiligen Treibers
 */
IVideoDriver* CreateVideoInstance(VideoDriverLoaderInterface* CallBack)
{
    return new VideoWinAPI(CallBack);
}

void FreeVideoInstance(IVideoDriver* driver)
{
    delete driver;
}

const char* GetDriverName()
{
    return "(WinAPI) OpenGL via the glorious WinAPI";
}

/** @class VideoWinAPI
 *
 *  Klasse für den WinAPI Videotreiber.
 */

/** @var VideoWinAPI::dm_prev
 *
 *  Bildschirmmodus.
 */

/** @var VideoWinAPI::screen
 *
 *  Fensterhandle.
 */

/** @var VideoWinAPI::screen_dc
 *
 *  Zeichenkontext des Fensters.
 */

/** @var VideoWinAPI::screen_rc
 *
 *  OpenGL-Kontext des Fensters.
 */

/**
 *  Konstruktor von @p VideoWinAPI.
 *
 *  @param[in] CallBack DriverCallback für Rückmeldungen.
 */
VideoWinAPI::VideoWinAPI(VideoDriverLoaderInterface* CallBack)
    : VideoDriver(CallBack), mouse_l(false), mouse_r(false), mouse_z(0), screen(nullptr), screen_dc(nullptr), screen_rc(nullptr),
      isWindowResizable(false), isMinimized(true)
{
    pVideoWinAPI = this;
}

VideoWinAPI::~VideoWinAPI()
{
    DestroyScreen();
    pVideoWinAPI = nullptr;
}

/**
 *  Funktion zum Auslesen des Treibernamens.
 *
 *  @return liefert den Treibernamen zurück
 */
const char* VideoWinAPI::GetName() const
{
    return GetDriverName();
}

/**
 *  Treiberinitialisierungsfunktion.
 *
 *  @return @p true bei Erfolg, @p false bei Fehler
 */
bool VideoWinAPI::Initialize()
{
    screen = nullptr;
    screen_dc = nullptr;
    screen_rc = nullptr;

    if(CallBack != nullptr)
        initialized = true;

    return initialized;
}

/**
 *  Treiberaufräumfunktion.
 */
void VideoWinAPI::CleanUp()
{
    // Fenster zerstören
    DestroyScreen();

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
 *
 *  @bug Hardwarecursor ist bei Fenstermodus sichtbar,
 *       Cursor deaktivieren ist fehlerhaft
 */
bool VideoWinAPI::CreateScreen(const std::string& title, const VideoMode& newSize, bool fullscreen)
{
    if(!initialized)
        return false;

    if(!RegisterAndCreateWindow(title, newSize, fullscreen))
        return false;

    if(fullscreen && !MakeFullscreen(GetWindowSize()))
        return false;
    isFullscreen_ = fullscreen;

    if(!InitOGL())
        return false;

    // Hide mouse cursor by resetting the texture. This way we still have the resize mouse cursor etc.
    SetCursor(nullptr);

    ShowWindow(screen, SW_SHOW);
    SetForegroundWindow(screen);
    SetFocus(screen);

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
bool VideoWinAPI::ResizeScreen(const VideoMode& newSize, bool fullscreen)
{
    if(!initialized || !isWindowResizable)
        return false;

    if(isFullscreen_ == fullscreen && newSize == GetWindowSize())
        return true;

    ShowWindow(screen, SW_HIDE);

    VideoMode windowSize = fullscreen ? FindClosestVideoMode(newSize) : newSize;
    // Try to switch full screen first
    if(isFullscreen_ && !fullscreen)
    {
        if(ChangeDisplaySettings(nullptr, 0) != DISP_CHANGE_SUCCESSFUL)
            return false;
    } else if(isFullscreen_ || fullscreen)
    {
        if(!MakeFullscreen(windowSize))
            return false;
    }

    // Fensterstyle ggf. ändern
    std::pair<DWORD, DWORD> style = GetStyleFlags(isFullscreen_);
    SetWindowLongPtr(screen, GWL_STYLE, style.first);
    SetWindowLongPtr(screen, GWL_EXSTYLE, style.second);

    RECT wRect = CalculateWindowRect(isFullscreen_, windowSize);

    // Fenstergröße ändern
    UINT flags = SWP_SHOWWINDOW | SWP_DRAWFRAME | SWP_FRAMECHANGED;
    SetWindowPos(screen, HWND_TOP, wRect.left, wRect.top, wRect.right - wRect.left, wRect.bottom - wRect.top, flags);
    SetNewSize(windowSize, Extent(windowSize.width, windowSize.height));

    ShowWindow(screen, SW_SHOW);
    SetForegroundWindow(screen);
    SetFocus(screen);

    return true;
}

std::pair<DWORD, DWORD> VideoWinAPI::GetStyleFlags(bool fullscreen) const
{
    if(fullscreen)
        return std::make_pair(WS_POPUP, WS_EX_APPWINDOW);
    else
        return std::make_pair(WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_OVERLAPPEDWINDOW | WS_SYSMENU | WS_MINIMIZEBOX | WS_CAPTION,
                              WS_EX_APPWINDOW | WS_EX_WINDOWEDGE);
}

RECT VideoWinAPI::CalculateWindowRect(bool fullscreen, VideoMode& size) const
{
    RECT wRect;
    if(fullscreen)
    {
        wRect.left = 0;
        wRect.top = 0;
    } else
    {
        RECT workArea;
        SystemParametersInfo(SPI_GETWORKAREA, 0, &workArea, 0);
        unsigned waWidth = workArea.right - workArea.left;
        unsigned waHeight = workArea.bottom - workArea.top;
        size.width = std::min<uint16_t>(size.width, waWidth);
        size.height = std::min<uint16_t>(size.height, waHeight);
        wRect.left = (waWidth - size.width) / 2;
        wRect.top = (waHeight - size.height) / 2;
    }
    wRect.right = wRect.left + size.width;
    wRect.bottom = wRect.top + size.height;

    std::pair<DWORD, DWORD> style = GetStyleFlags(fullscreen);
    // Calculate real right/bottom based on the window style
    AdjustWindowRectEx(&wRect, style.first, false, style.second);

    return wRect;
}

bool VideoWinAPI::RegisterAndCreateWindow(const std::string& title, const VideoMode& wndSize, bool fullscreen)
{
    std::wstring wTitle = cvUTF8ToWideString(title);
    windowClassName = wTitle.substr(0, wTitle.find(' '));

    // Register window class
    WNDCLASSW wc;
    wc.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
    wc.lpfnWndProc = WindowProc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = GetModuleHandle(nullptr);
    wc.hIcon = LoadIcon(GetModuleHandle(nullptr), MAKEINTRESOURCE(IDI_SYMBOL));
    wc.hCursor = nullptr;
    wc.hbrBackground = nullptr;
    wc.lpszMenuName = nullptr;
    wc.lpszClassName = windowClassName.c_str();

    if(!RegisterClassW(&wc))
        return false;

    // Create window
    auto adjWindowSize = fullscreen ? FindClosestVideoMode(wndSize) : wndSize;
    RECT wRect = CalculateWindowRect(fullscreen, adjWindowSize);

    std::pair<DWORD, DWORD> style = GetStyleFlags(fullscreen);
    screen = CreateWindowExW(style.second, windowClassName.c_str(), wTitle.c_str(), style.first, wRect.left, wRect.top,
                             wRect.right - wRect.left, wRect.bottom - wRect.top, nullptr, nullptr, GetModuleHandle(nullptr), nullptr);

    if(screen == nullptr)
        return false;

    SetNewSize(adjWindowSize, Extent(adjWindowSize.width, adjWindowSize.height));

    SetClipboardViewer(screen);

    SetWindowTextW(screen, wTitle.c_str());
    SetWindowTextW(GetConsoleWindow(), wTitle.c_str());
    return true;
}

bool VideoWinAPI::InitOGL()
{
    RTTR_Assert(!screen_dc && !screen_rc);
    // Pixelformat zuweisen
    static PIXELFORMATDESCRIPTOR pfd = {sizeof(PIXELFORMATDESCRIPTOR),
                                        1,
                                        PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER,
                                        PFD_TYPE_RGBA,
                                        8, // 8 Bit
                                        8,
                                        0,
                                        8,
                                        0,
                                        8,
                                        0,
                                        8,
                                        0, // RGBA 8 bits, 0 bits shift
                                        0,
                                        0,
                                        0,
                                        0,
                                        0,  // No accum bits
                                        32, // 32 Bit depth
                                        0,
                                        0, // no stencil or aux buffers
                                        PFD_MAIN_PLANE,
                                        0,
                                        0,
                                        0,
                                        0}; // reserved and masks 0

    screen_dc = GetDC(screen);
    if(screen_dc == nullptr)
        return false;

    // Pixelformat auswaehlen
    GLuint PixelFormat = ChoosePixelFormat(screen_dc, &pfd);
    if(PixelFormat == 0)
        return false;

    // Pixelformat zuweisen
    if(!SetPixelFormat(screen_dc, PixelFormat, &pfd))
        return false;

    // Renderingkontext erstellen
    screen_rc = wglCreateContext(screen_dc);
    if(screen_rc == nullptr)
        return false;

    // Renderingkontext aktivieren
    if(!wglMakeCurrent(screen_dc, screen_rc))
        return false;

    return true;
}

bool VideoWinAPI::MakeFullscreen(const VideoMode& resolution)
{
    DEVMODE dm;
    memset(&dm, 0, sizeof(dm));
    dm.dmSize = sizeof(dm);

    EnumDisplaySettings(0, ENUM_CURRENT_SETTINGS, &dm);
    dm.dmPelsWidth = resolution.width;
    dm.dmPelsHeight = resolution.height;
    dm.dmFields = DM_PELSWIDTH | DM_PELSHEIGHT;

    LONG result = ChangeDisplaySettings(&dm, CDS_FULLSCREEN);
    if(result != DISP_CHANGE_SUCCESSFUL)
    {
        std::cerr << "Changing display mode failed with " << result << std::endl;
        return false;
    }
    return true;
}

/**
 *  Schliesst das Fenster.
 */
void VideoWinAPI::DestroyScreen()
{
    // Fenster schliessen
    EndDialog(screen, 0);

    // Reset display settings to defaults
    ChangeDisplaySettings(nullptr, 0);

    if(screen_rc)
    {
        if(!wglMakeCurrent(nullptr, nullptr))
            return;

        if(!wglDeleteContext(screen_rc))
            return;

        screen_rc = nullptr;
    }

    if(screen_dc && !ReleaseDC(screen, screen_dc))
        return;

    screen_dc = nullptr;

    if(screen && !DestroyWindow(screen))
        return;

    screen = nullptr;

    UnregisterClassW(windowClassName.c_str(), GetModuleHandle(nullptr));

    isFullscreen_ = false;
}

/**
 *  Wechselt die OpenGL-Puffer.
 *
 *  @return @p true bei Erfolg, @p false bei Fehler
 */
bool VideoWinAPI::SwapBuffers()
{
    if(!screen_dc)
        return false;

    // Puffer wechseln
    ::SwapBuffers(screen_dc);

    return true;
}

/**
 *  Die Nachrichtenschleife.
 *
 *  @return @p true bei Erfolg, @p false bei Fehler
 */
bool VideoWinAPI::MessageLoop()
{
    MSG msg;
    if(PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
    {
        if(msg.message == WM_QUIT)
            return false;

        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return true;
}

/**
 *  Funktion zum Auslesen des TickCounts.
 *
 *  @return liefert den TickCount
 */
unsigned long VideoWinAPI::GetTickCount() const
{
    return ::GetTickCount();
}

static void* wglGetProcAddress_Wrapper(const char* name)
{
    auto* func = reinterpret_cast<void*>(wglGetProcAddress(name));
    if(!func)
    {
        const auto opengl32DLL = LoadLibrary(L"opengl32.dll");
        func = reinterpret_cast<void*>(GetProcAddress(opengl32DLL, name));
    }
    return func;
}

OpenGL_Loader_Proc VideoWinAPI::GetLoaderFunction() const
{
    return wglGetProcAddress_Wrapper;
}

/// Listet verfügbare Videomodi auf
void VideoWinAPI::ListVideoModes(std::vector<VideoMode>& video_modes) const
{
    DEVMODE dm;
    memset(&dm, 0, sizeof(dm));
    dm.dmSize = sizeof(dm);
    unsigned m = 0;
    while(EnumDisplaySettings(nullptr, m++, &dm))
    {
        VideoMode vm(static_cast<unsigned short>(dm.dmPelsWidth), static_cast<unsigned short>(dm.dmPelsHeight));
        if(!helpers::contains(video_modes, vm))
            video_modes.push_back(vm);
    }
}

/**
 *  Funktion zum Setzen der Mauskoordinaten.
 *
 *  @param[in] x X-Koordinate
 *  @param[in] y Y-Koordinate
 */
void VideoWinAPI::SetMousePos(Position pos)
{
    if(GetActiveWindow())
    {
        mouse_xy.pos = pos;

        POINT p = {pos.x, pos.y};
        ClientToScreen(screen, &p);
        SetCursorPos(p.x, p.y);
    }
}

/**
 *  Funktion zum Senden einer gedrückten Taste.
 *
 *  @param[in] c Tastencode
 */
void VideoWinAPI::OnWMChar(unsigned c, bool disablepaste, LPARAM lParam)
{
    // Keine Leerzeichen als Extra-Zeichen senden!
    if(c == ' ')
        return;

    KeyEvent ke = {KT_CHAR, c, (GetKeyState(VK_CONTROL) & 0x8000) != 0, (GetKeyState(VK_SHIFT) & 0x8000) != 0, (lParam & KF_ALTDOWN) != 0};

    if(c == 'V' || c == 'v' || c == 0x16)
        if(!disablepaste && ke.ctrl != 0)
        {
            OnWMPaste();
            return;
        }

    CallBack->Msg_KeyDown(ke);
}

/**
 *  Funktion zum Senden einer gedrückten Taste.
 *
 *  @param[in] c Tastencode
 */
void VideoWinAPI::OnWMKeyDown(unsigned c, LPARAM lParam)
{
    KeyEvent ke = {KT_INVALID, 0, (GetKeyState(VK_CONTROL) & 0x8000) != 0, (GetKeyState(VK_SHIFT) & 0x8000) != 0,
                   (lParam & KF_ALTDOWN) != 0};

    switch(c)
    {
        case VK_RETURN:
        {
            // Don't report Alt+Return events, as WinAPI seems to fire them in a lot of cases
            ke.kt = KT_RETURN;
            ke.alt = false;
        }
        break;
        case VK_SPACE: ke.kt = KT_SPACE; break;
        case VK_LEFT: ke.kt = KT_LEFT; break;
        case VK_RIGHT: ke.kt = KT_RIGHT; break;
        case VK_UP: ke.kt = KT_UP; break;
        case VK_DOWN: ke.kt = KT_DOWN; break;
        case VK_BACK: ke.kt = KT_BACKSPACE; break;
        case VK_DELETE: ke.kt = KT_DELETE; break;
        case VK_LSHIFT: ke.kt = KT_SHIFT; break;
        case VK_RSHIFT: ke.kt = KT_SHIFT; break;
        case VK_TAB: ke.kt = KT_TAB; break;
        case VK_END: ke.kt = KT_END; break;
        case VK_HOME: ke.kt = KT_HOME; break;
        case VK_ESCAPE: ke.kt = KT_ESCAPE; break;
        case VK_PRINT: ke.kt = KT_PRINT; break;
        default:
        {
            // Die 12 F-Tasten
            if(c >= VK_F1 && c <= VK_F12)
                ke.kt = static_cast<KeyType>(KT_F1 + int(c) - VK_F1);
        }
        break;
    }

    if(ke.kt != KT_INVALID)
        CallBack->Msg_KeyDown(ke);
}

/**
 *  Funktion zum Pasten von Text aus dem Clipboard.
 */
void VideoWinAPI::OnWMPaste()
{
    if(!IsClipboardFormatAvailable(CF_UNICODETEXT))
        return;

    OpenClipboard(nullptr);

    HANDLE hData = GetClipboardData(CF_UNICODETEXT);
    const wchar_t* pData = (const wchar_t*)GlobalLock(hData);

    while(pData && *pData)
    {
        wchar_t c = *(pData++);
        OnWMKeyDown(c);
        OnWMChar(c, true);
    }

    GlobalUnlock(hData);
    CloseClipboard();
}

/**
 *  Callbackfunktion der WinAPI.
 *
 *  @param[in] window Fensterhandle
 *  @param[in] msg    Fensternachricht
 *  @param[in] wParam Erster Nachrichtenparameter
 *  @param[in] wParam Zweiter Nachrichtenparameter
 */
LRESULT CALLBACK VideoWinAPI::WindowProc(HWND window, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch(msg)
    {
        case WM_PASTE: pVideoWinAPI->OnWMPaste(); break;
        case WM_CLOSE: PostQuitMessage(0); return 0;
        case WM_SIZE:
        {
            if(wParam == SIZE_MINIMIZED)
                pVideoWinAPI->isMinimized = true;
            if(wParam != SIZE_MAXIMIZED && wParam != SIZE_RESTORED)
                break;
            VideoMode newSize(LOWORD(lParam), HIWORD(lParam));
            if(pVideoWinAPI->GetWindowSize() != newSize || pVideoWinAPI->isMinimized)
            {
                pVideoWinAPI->SetNewSize(newSize, Extent(newSize.width, newSize.height));

                pVideoWinAPI->isWindowResizable = false;
                pVideoWinAPI->CallBack->WindowResized();
                pVideoWinAPI->isWindowResizable = true;
            }
            pVideoWinAPI->isMinimized = false;
            break;
        }
        case WM_SYSCOMMAND:
            switch(wParam)
            {
                case SC_SCREENSAVE:
                case SC_MONITORPOWER: return 0;
                case SC_KEYMENU: // F10-Fehler beheben -> will sonst Fenster verschieben, was das Zeichnen unterbindet
                    pVideoWinAPI->OnWMKeyDown(VK_F10, 0); // pretend we got a F10 stroke
                    return 0;
            }
            break;
        case WM_SETCURSOR:
            // Set no cursor again
            SetCursor(nullptr);
            break;
        case WM_MOUSEMOVE:
            pVideoWinAPI->mouse_xy.pos.x = LOWORD(lParam);
            pVideoWinAPI->mouse_xy.pos.y = HIWORD(lParam);
            pVideoWinAPI->CallBack->Msg_MouseMove(pVideoWinAPI->mouse_xy);
            break;
        case WM_LBUTTONDOWN:
            pVideoWinAPI->mouse_l = true;
            pVideoWinAPI->mouse_xy.ldown = true;
            pVideoWinAPI->CallBack->Msg_LeftDown(pVideoWinAPI->mouse_xy);
            break;
        case WM_LBUTTONUP:
            pVideoWinAPI->mouse_l = false;
            pVideoWinAPI->mouse_xy.ldown = false;
            pVideoWinAPI->CallBack->Msg_LeftUp(pVideoWinAPI->mouse_xy);
            break;
        case WM_RBUTTONDOWN:
            pVideoWinAPI->mouse_r = true;
            pVideoWinAPI->mouse_xy.rdown = true;
            pVideoWinAPI->CallBack->Msg_RightDown(pVideoWinAPI->mouse_xy);
            break;
        case WM_RBUTTONUP:
            pVideoWinAPI->mouse_r = false;
            pVideoWinAPI->mouse_xy.rdown = false;
            pVideoWinAPI->CallBack->Msg_RightUp(pVideoWinAPI->mouse_xy);
            break;
        case WM_MOUSEWHEEL:
            // Obtain scrolling distance. For every multiple of WHEEL_DELTA, we have to fire an event, because we treat the wheel like two
            // buttons. One wheel "step" usually produces a mouse_z  of +/- WHEEL_DELTA. But there may exist wheels without "steps" that
            // result in lower values we have to cumulate.
            pVideoWinAPI->mouse_z += GET_WHEEL_DELTA_WPARAM(wParam);

            // We don't want to crash if there were even wheels that produce higher values...
            while(std::abs(pVideoWinAPI->mouse_z) >= WHEEL_DELTA)
            {
                if(pVideoWinAPI->mouse_z > 0) // Scrolled to top
                {
                    pVideoWinAPI->mouse_z -= WHEEL_DELTA;
                    pVideoWinAPI->CallBack->Msg_WheelUp(pVideoWinAPI->mouse_xy);
                } else // Scrolled to bottom
                {
                    pVideoWinAPI->mouse_z += WHEEL_DELTA;
                    pVideoWinAPI->CallBack->Msg_WheelDown(pVideoWinAPI->mouse_xy);
                }
            }
            break;
        case WM_KEYDOWN:
            //  case WM_SYSKEYDOWN: // auch abfangen, wenn linkes ALT mit gedrückt wurde
            pVideoWinAPI->OnWMKeyDown((unsigned)wParam, lParam);
            return 0;
        case WM_CHAR:
        case WM_SYSCHAR: // auch abfangen, wenn linkes ALT mit gedrückt wurde
            pVideoWinAPI->OnWMChar((unsigned)wParam, false, lParam);
            return 0;
    }
    return DefWindowProcW(window, msg, wParam, lParam);
}

/**
 *  Get state of the modifier keys
 */
KeyEvent VideoWinAPI::GetModKeyState() const
{
    const KeyEvent ke = {KT_INVALID, 0, (GetKeyState(VK_CONTROL) & 0x8000) != 0, (GetKeyState(VK_SHIFT) & 0x8000) != 0,
                         (GetKeyState(VK_MENU) & 0x8000) != 0};
    return ke;
}

/// Gibt Pointer auf ein Fenster zurück (device-dependent!), HWND unter Windows
void* VideoWinAPI::GetMapPointer() const
{
    return (void*)this->screen;
}
