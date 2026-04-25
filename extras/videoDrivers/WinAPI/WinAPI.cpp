// Copyright (C) 2005 - 2026 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "WinAPI.h"
#include "RTTR_Assert.h"
#include "driver/Interface.h"
#include "driver/VideoDriverLoaderInterface.h"
#include "driver/VideoInterface.h"
#include "enum_cast.hpp"
#include "helpers/containerUtils.h"
#include "s25clientResources.h"
#include <boost/nowide/convert.hpp>
#include <GL/gl.h>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <limits>
#include <winuser.h>

namespace {
void setSpecialKeys(KeyEvent& ke, LPARAM lParam)
{
    ke.ctrl = (GetKeyState(VK_CONTROL) & 0x8000) != 0;
    ke.shift = (GetKeyState(VK_SHIFT) & 0x8000) != 0;
    ke.alt = (HIWORD(lParam) & KF_ALTDOWN) != 0;
}
} // namespace

static VideoWinAPI* pVideoWinAPI = nullptr;

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

VideoWinAPI::VideoWinAPI(VideoDriverLoaderInterface* CallBack)
    : VideoDriver(CallBack), mouse_z(0), screen(nullptr), screen_dc(nullptr), screen_rc(nullptr),
      isDisplayModeChangeable(false), isMinimized(true)
{
    pVideoWinAPI = this;
}

VideoWinAPI::~VideoWinAPI()
{
    DestroyScreen();
    pVideoWinAPI = nullptr;
}

const char* VideoWinAPI::GetName() const
{
    return GetDriverName();
}

bool VideoWinAPI::Initialize()
{
    screen = nullptr;
    screen_dc = nullptr;
    screen_rc = nullptr;

    if(CallBack != nullptr)
        initialized = true;

    return initialized;
}

void VideoWinAPI::CleanUp()
{
    DestroyScreen();
    initialized = false;
}

bool VideoWinAPI::CreateScreen(const std::string& title, const VideoMode newSize, DisplayMode displayMode)
{
    if(!initialized)
        return false;

    if(!RegisterAndCreateWindow(title, newSize, displayMode))
        return false;

    if(displayMode == DisplayMode::Fullscreen && !MakeFullscreen(GetWindowSize()))
        return false;

    displayMode_ = displayMode;
    if(!InitOGL())
        return false;

    // Hide mouse cursor by resetting the texture. This way we still have the resize mouse cursor etc.
    SetCursor(nullptr);

    ShowWindow(screen, SW_SHOW);
    SetForegroundWindow(screen);
    SetFocus(screen);

    return true;
}

bool VideoWinAPI::ResizeScreen(const VideoMode newSize, DisplayMode displayMode)
{
    if(!initialized || !isDisplayModeChangeable)
        return false;

    if(displayMode == displayMode_ && newSize == GetWindowSize())
        return true;

    VideoMode windowSize = (displayMode == DisplayMode::Fullscreen) ? FindClosestVideoMode(newSize) : newSize;
    // Try to switch full screen first
    if(displayMode_ == DisplayMode::Fullscreen && displayMode != DisplayMode::Fullscreen)
    {
        ShowWindow(screen, SW_HIDE);
        if(ChangeDisplaySettings(nullptr, 0) != DISP_CHANGE_SUCCESSFUL)
        {
            ShowWindow(screen, SW_SHOW);
            return false;
        }
    } else if(displayMode == DisplayMode::Fullscreen)
    {
        ShowWindow(screen, SW_HIDE);
        if(!MakeFullscreen(windowSize))
        {
            ShowWindow(screen, SW_SHOW);
            return false;
        }
    }

    // Adjust style
    const auto [style, exStyle] = GetStyleFlags(displayMode);
    SetWindowLongPtr(screen, GWL_STYLE, style);
    SetWindowLongPtr(screen, GWL_EXSTYLE, exStyle);
    displayMode_ = displayMode;

    const auto [wndArea, adjustedSize] = CalculateWindowRect(displayMode, windowSize);

    // Set size and position
    UINT flags = SWP_SHOWWINDOW | SWP_DRAWFRAME | SWP_FRAMECHANGED;
    SetWindowPos(screen, HWND_TOP, wndArea.left, wndArea.top, wndArea.getSize().x, wndArea.getSize().y, flags);
    SetNewSize(adjustedSize, Extent(adjustedSize.width, adjustedSize.height));

    ShowWindow(screen, SW_SHOW);
    SetActiveWindow(screen);
    SetForegroundWindow(screen);
    SetFocus(screen);

    return true;
}

std::pair<DWORD, DWORD> VideoWinAPI::GetStyleFlags(const DisplayMode mode) const
{
    if(mode == DisplayMode::Fullscreen)
        return std::make_pair(WS_POPUP, WS_EX_APPWINDOW);
    else
    {
        auto result = std::make_pair(WS_CLIPSIBLINGS | WS_CLIPCHILDREN, WS_EX_APPWINDOW);
        if(mode == DisplayMode::BorderlessWindow)
            result.first |= WS_POPUP;
        else
        {
            result.first |= WS_OVERLAPPEDWINDOW;
            result.second |= WS_EX_WINDOWEDGE;
            if(!mode.resizeable)
                result.first &= ~WS_THICKFRAME;
        }
        return result;
    }
}

std::pair<Rect, VideoMode> VideoWinAPI::CalculateWindowRect(const DisplayMode mode, VideoMode requestedSize) const
{
    switch(mode.type)
    {
        case DisplayMode::Fullscreen:
        {
            const auto size = FindClosestVideoMode(requestedSize);
            return std::make_pair(Rect(Position::all(0), size.width, size.height), size);
        };
        case DisplayMode::BorderlessWindow:
        {
            const auto size = getDesktopSize(requestedSize);
            return std::make_pair(Rect(Position::all(0), size.width, size.height), size);
        }
        case DisplayMode::Windowed:
        {
            RECT workArea;
            SystemParametersInfo(SPI_GETWORKAREA, 0, &workArea, 0);
            const unsigned waWidth = workArea.right - workArea.left;
            const unsigned waHeight = workArea.bottom - workArea.top;
            const VideoMode size(std::min<uint16_t>(requestedSize.width, waWidth),
                                 std::min<uint16_t>(requestedSize.height, waHeight));
            RECT wRect;
            wRect.left = (waWidth - size.width) / 2;
            wRect.top = (waHeight - size.height) / 2;
            wRect.right = wRect.left + size.width;
            wRect.bottom = wRect.top + size.height;
            // Calculate real right/bottom based on the window style
            const auto [style, exStyle] = GetStyleFlags(mode);
            AdjustWindowRectEx(&wRect, style, false, exStyle);
            // Make sure border/title bar is visible
            wRect.left = std::max<LONG>(wRect.left, 0);
            wRect.top = std::max<LONG>(wRect.top, 0);
            return std::make_pair(Rect(wRect.left, wRect.top, wRect.right - wRect.left, wRect.bottom - wRect.top),
                                  size);
        }
    }

    BOOST_UNREACHABLE_RETURN(std::make_pair(RECT{0, 0, requestedSize.width, requestedSize.height}, requestedSize));
}

bool VideoWinAPI::RegisterAndCreateWindow(const std::string& title, const VideoMode wndSize, DisplayMode displayMode)
{
    std::wstring wTitle = boost::nowide::widen(title);
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
    const auto [wndArea, adjustedSize] = CalculateWindowRect(displayMode, wndSize);
    const auto [style, exStyle] = GetStyleFlags(displayMode);
    screen =
      CreateWindowExW(exStyle, windowClassName.c_str(), wTitle.c_str(), style, wndArea.left, wndArea.top,
                      wndArea.getSize().x, wndArea.getSize().y, nullptr, nullptr, GetModuleHandle(nullptr), nullptr);

    if(screen == nullptr)
        return false;

    SetNewSize(adjustedSize, Extent(adjustedSize.width, adjustedSize.height));

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

bool VideoWinAPI::MakeFullscreen(const VideoMode resolution)
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

VideoMode VideoWinAPI::getDesktopSize(const VideoMode fallback) const
{
    HMONITOR monitor;
    if(screen)
        monitor = MonitorFromWindow(screen, MONITOR_DEFAULTTONEAREST);
    else
        monitor = MonitorFromPoint(POINT{0, 0}, MONITOR_DEFAULTTOPRIMARY);

    MONITORINFO mi{};
    mi.cbSize = sizeof(mi);
    if(GetMonitorInfo(monitor, &mi) == TRUE)
    {
        return VideoMode(static_cast<unsigned short>(mi.rcMonitor.right - mi.rcMonitor.left),
                         static_cast<unsigned short>(mi.rcMonitor.bottom - mi.rcMonitor.top));
    } else
        return fallback;
}

void VideoWinAPI::DestroyScreen()
{
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
}

bool VideoWinAPI::SwapBuffers()
{
    if(!screen_dc)
        return false;

    ::SwapBuffers(screen_dc);
    return true;
}

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

void VideoWinAPI::ShowErrorMessage(const std::string& title, const std::string& message)
{
    MessageBoxA(nullptr, message.c_str(), title.c_str(), MB_OK | MB_ICONEXCLAMATION);
}

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

std::vector<VideoMode> VideoWinAPI::ListVideoModes() const
{
    DEVMODE dm;
    memset(&dm, 0, sizeof(dm));
    dm.dmSize = sizeof(dm);
    unsigned m = 0;
    std::vector<VideoMode> video_modes;
    while(EnumDisplaySettings(nullptr, m++, &dm))
    {
        VideoMode vm(static_cast<unsigned short>(dm.dmPelsWidth), static_cast<unsigned short>(dm.dmPelsHeight));
        if(!helpers::contains(video_modes, vm))
            video_modes.push_back(vm);
    }
    return video_modes;
}

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

void VideoWinAPI::OnWMChar(unsigned c, bool disablepaste, LPARAM lParam)
{
    // Keine Leerzeichen als Extra-Zeichen senden!
    if(c == ' ')
        return;

    KeyEvent ke(c);
    setSpecialKeys(ke, lParam);

    if(c == 'V' || c == 'v' || c == 0x16)
    {
        if(!disablepaste && ke.ctrl)
        {
            OnWMPaste();
            return;
        }
    }

    CallBack->Msg_KeyDown(ke);
}

void VideoWinAPI::OnWMKeyDown(unsigned c, LPARAM lParam)
{
    KeyEvent ke;
    setSpecialKeys(ke, lParam);

    switch(c)
    {
        case VK_RETURN:
        {
            // Don't report Alt+Return events, as WinAPI seems to fire them in a lot of cases
            ke.kt = KeyType::Return;
            ke.alt = false;
        }
        break;
        case VK_SPACE: ke.kt = KeyType::Space; break;
        case VK_LEFT: ke.kt = KeyType::Left; break;
        case VK_RIGHT: ke.kt = KeyType::Right; break;
        case VK_UP: ke.kt = KeyType::Up; break;
        case VK_DOWN: ke.kt = KeyType::Down; break;
        case VK_BACK: ke.kt = KeyType::Backspace; break;
        case VK_DELETE: ke.kt = KeyType::Delete; break;
        case VK_LSHIFT: ke.kt = KeyType::Shift; break;
        case VK_RSHIFT: ke.kt = KeyType::Shift; break;
        case VK_TAB: ke.kt = KeyType::Tab; break;
        case VK_END: ke.kt = KeyType::End; break;
        case VK_HOME: ke.kt = KeyType::Home; break;
        case VK_ESCAPE: ke.kt = KeyType::Escape; break;
        case VK_PRINT: ke.kt = KeyType::Print; break;
        default:
        {
            // Die 12 F-Tasten
            if(c >= VK_F1 && c <= VK_F12)
                ke.kt = static_cast<KeyType>(rttr::enum_cast(KeyType::F1) + c - VK_F1);
        }
        break;
    }

    if(ke.kt != KeyType::Invalid)
        CallBack->Msg_KeyDown(ke);
}

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

                pVideoWinAPI->isDisplayModeChangeable = false;
                pVideoWinAPI->CallBack->WindowResized();
                pVideoWinAPI->isDisplayModeChangeable = true;
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
            pVideoWinAPI->mouse_xy.ldown = true;
            pVideoWinAPI->CallBack->Msg_LeftDown(pVideoWinAPI->mouse_xy);
            break;
        case WM_LBUTTONUP:
            pVideoWinAPI->mouse_xy.ldown = false;
            pVideoWinAPI->CallBack->Msg_LeftUp(pVideoWinAPI->mouse_xy);
            break;
        case WM_RBUTTONDOWN:
            pVideoWinAPI->mouse_xy.rdown = true;
            pVideoWinAPI->CallBack->Msg_RightDown(pVideoWinAPI->mouse_xy);
            break;
        case WM_RBUTTONUP:
            pVideoWinAPI->mouse_xy.rdown = false;
            pVideoWinAPI->CallBack->Msg_RightUp(pVideoWinAPI->mouse_xy);
            break;
        case WM_MBUTTONDOWN:
            pVideoWinAPI->mouse_xy.mdown = true;
            pVideoWinAPI->CallBack->Msg_MiddleDown(pVideoWinAPI->mouse_xy);
            break;
        case WM_MBUTTONUP:
            pVideoWinAPI->mouse_xy.mdown = false;
            pVideoWinAPI->CallBack->Msg_MiddleUp(pVideoWinAPI->mouse_xy);
            break;
        case WM_MOUSEWHEEL:
            // Obtain scrolling distance. For every multiple of WHEEL_DELTA, we have to fire an event, because we treat
            // the wheel like two buttons. One wheel "step" usually produces a mouse_z  of +/- WHEEL_DELTA. But there
            // may exist wheels without "steps" that result in lower values we have to cumulate.
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

KeyEvent VideoWinAPI::GetModKeyState() const
{
    KeyEvent ke;
    ke.ctrl = (GetKeyState(VK_CONTROL) & 0x8000) != 0;
    ke.shift = (GetKeyState(VK_SHIFT) & 0x8000) != 0;
    ke.alt = (GetKeyState(VK_MENU) & 0x8000) != 0;
    return ke;
}

void* VideoWinAPI::GetMapPointer() const
{
    return (void*)this->screen;
}
