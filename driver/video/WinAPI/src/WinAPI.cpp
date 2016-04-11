// Copyright (c) 2005 - 2015 Settlers Freaks (sf-team at siedler25.org)
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

///////////////////////////////////////////////////////////////////////////////
// Header
#include "main.h" // IWYU pragma: keep
#include "WinAPI.h"

#include "../../../../win32/resource.h"
#include "VideoDriverLoaderInterface.h"
#include <build_version.h>
#include <VideoInterface.h>
#include <GL/gl.h>
#include <cstdlib>
#include <cstring>
#include <sstream>

// Include last!
#include "DebugNew.h" // IWYU pragma: keep

///////////////////////////////////////////////////////////////////////////////
/**
 *  Zeiger auf die aktuelle Instanz.
 *
 *  @author FloSoft
 */
static VideoWinAPI* pVideoWinAPI = NULL;

///////////////////////////////////////////////////////////////////////////////
/**
 *  Instanzierungsfunktion von @p VideoWinAPI.
 *
 *  @param[in] CallBack DriverCallback für Rückmeldungen.
 *
 *  @return liefert eine Instanz des jeweiligen Treibers
 *
 *  @author FloSoft
 */
DRIVERDLLAPI IVideoDriver* CreateVideoInstance(VideoDriverLoaderInterface* CallBack)
{
    return new VideoWinAPI(CallBack);
}

DRIVERDLLAPI void FreeVideoInstance(IVideoDriver* driver)
{
    delete driver;
}

DRIVERDLLAPI const char* GetDriverName(void)
{
    return "(WinAPI) OpenGL via the glorious WinAPI";
}

///////////////////////////////////////////////////////////////////////////////
/** @class VideoWinAPI
 *
 *  Klasse für den WinAPI Videotreiber.
 *
 *  @author FloSoft
 */

///////////////////////////////////////////////////////////////////////////////
/** @var VideoWinAPI::dm_prev
 *
 *  Bildschirmmodus.
 *
 *  @author FloSoft
 */

///////////////////////////////////////////////////////////////////////////////
/** @var VideoWinAPI::screen
 *
 *  Fensterhandle.
 *
 *  @author FloSoft
 */

///////////////////////////////////////////////////////////////////////////////
/** @var VideoWinAPI::screen_dc
 *
 *  Zeichenkontext des Fensters.
 *
 *  @author FloSoft
 */

///////////////////////////////////////////////////////////////////////////////
/** @var VideoWinAPI::screen_rc
 *
 *  OpenGL-Kontext des Fensters.
 *
 *  @author FloSoft
 */

///////////////////////////////////////////////////////////////////////////////
/**
 *  Konstruktor von @p VideoWinAPI.
 *
 *  @param[in] CallBack DriverCallback für Rückmeldungen.
 *
 *  @author FloSoft
 */
VideoWinAPI::VideoWinAPI(VideoDriverLoaderInterface* CallBack):
    VideoDriver(CallBack), mouse_l(false), mouse_r(false), mouse_z(0),
    screen(NULL), screen_dc(NULL), screen_rc(NULL), isWindowResizable(false)
{
    memset(&dm_prev, 0, sizeof(DEVMODE));
    dm_prev.dmSize = sizeof(DEVMODE);
    pVideoWinAPI = this;
}

VideoWinAPI::~VideoWinAPI(void)
{
    pVideoWinAPI = NULL;
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  Funktion zum Auslesen des Treibernamens.
 *
 *  @return liefert den Treibernamen zurück
 *
 *  @author FloSoft
 */
const char* VideoWinAPI::GetName(void) const
{
    return GetDriverName();
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  Treiberinitialisierungsfunktion.
 *
 *  @return @p true bei Erfolg, @p false bei Fehler
 *
 *  @author FloSoft
 */
bool VideoWinAPI::Initialize(void)
{
    memset(&dm_prev, 0, sizeof(DEVMODE));
    dm_prev.dmSize = sizeof(DEVMODE);

    screen = NULL;
    screen_dc = NULL;
    screen_rc = NULL;

    if(CallBack != NULL)
        initialized = true;

    return initialized;
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  Treiberaufräumfunktion.
 *
 *  @author FloSoft
 */
void VideoWinAPI::CleanUp(void)
{
    // Fenster zerstören
    DestroyScreen();

    memset(&dm_prev, 0, sizeof(DEVMODE));
    dm_prev.dmSize = sizeof(DEVMODE);

    // nun sind wir nicht mehr initalisiert
    initialized = false;
}

std::wstring AnsiToUtf8(LPCSTR tSource, int nLength = -1)
{
    int nConvertedLength = MultiByteToWideChar(CP_UTF8, 0, tSource, nLength, NULL, 0);
    if(!nConvertedLength)
        return std::wstring();
    std::vector<wchar_t> wTarget(nConvertedLength);
    int nResult = MultiByteToWideChar(CP_UTF8, 0, tSource, nLength, &wTarget[0], nConvertedLength);
    if(nResult != nConvertedLength)
        wTarget.clear();

    return &wTarget[0];
}

///////////////////////////////////////////////////////////////////////////////
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
 *
 *  @author FloSoft
 */
bool VideoWinAPI::CreateScreen(unsigned short width, unsigned short height, const bool fullscreen)
{
    if(!initialized)
        return false;

    std::wstring wTitle = AnsiToUtf8(GetWindowTitle());

    WNDCLASSW  wc;
    wc.style            = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
    wc.lpfnWndProc      = WindowProc;
    wc.cbClsExtra       = 0;
    wc.cbWndExtra       = 0;
    wc.hInstance        = GetModuleHandle(NULL);
    wc.hIcon            = LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_SYMBOL));
    wc.hCursor          = NULL;
    wc.hbrBackground    = NULL;
    wc.lpszMenuName     = NULL;
    wc.lpszClassName    = wTitle.c_str();

    // Fensterklasse registrieren
    if (!RegisterClassW(&wc))
        return false;

    DWORD dwExStyle = WS_EX_APPWINDOW | WS_EX_WINDOWEDGE;
    DWORD dwStyle   = WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_OVERLAPPED | WS_SYSMENU | WS_MINIMIZEBOX;

    if(fullscreen)
    {
        dwExStyle   = WS_EX_APPWINDOW;
        dwStyle     = WS_POPUP;

        EnumDisplaySettings(0, ENUM_CURRENT_SETTINGS, &dm_prev);
    }
    else
    {
        // Bei Fensteranwendung die Breiten und Hoehen der Fensterrahmen, Titelleiste draufaddieren
        width += 2 * GetSystemMetrics(SM_CXFIXEDFRAME);
        height += 2 * GetSystemMetrics(SM_CXFIXEDFRAME) + GetSystemMetrics(SM_CYCAPTION);
    }

    // Fenster erstellen
    screen = CreateWindowExW(dwExStyle, wTitle.c_str(), wTitle.c_str(), dwStyle, CW_USEDEFAULT, CW_USEDEFAULT, width, height, NULL, NULL, GetModuleHandle(NULL), NULL);

    if(screen == NULL)
        return false;

    SetClipboardViewer(screen);

    std::stringstream title;
    title << GetWindowTitle() << " - v" << GetWindowVersion() << "-" << GetWindowRevisionShort();

    wTitle = AnsiToUtf8(title.str().c_str());

    SetWindowTextW(screen, wTitle.c_str());
    SetWindowTextW(GetConsoleWindow(), wTitle.c_str());

    // Pixelformat zuweisen
    GLuint PixelFormat;
    static PIXELFORMATDESCRIPTOR pfd =
    {
        sizeof(PIXELFORMATDESCRIPTOR),
        1,
        PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER,
        PFD_TYPE_RGBA,
        8, // 8 Bit
        8, // red
        0,
        8, // green
        0,
        8, // blue
        0,
        8, // alpha
        0,
        0,
        0,
        0,
        0,
        0,
        32, // 32 Bit
        0,
        0,
        PFD_MAIN_PLANE,
        0,
        0,
        0,
        0
    };

    screen_dc = GetDC(screen);
    if(screen_dc == NULL)
        return false;

    // Pixelformat auswaehlen
    PixelFormat = ChoosePixelFormat(screen_dc, &pfd);
    if(PixelFormat == 0)
        return false;

    // Pixelformat zuweisen
    if(!SetPixelFormat(screen_dc, PixelFormat, &pfd))
        return false;

    // Renderingkontext erstellen
    screen_rc = wglCreateContext(screen_dc);
    if(screen_rc == NULL)
        return false;

    // Renderingkontext aktivieren
    if(!wglMakeCurrent(screen_dc, screen_rc))
        return false;

    // Mauscursor ausblenden
    ShowCursor(FALSE);

    // Bei Fullscreen Aufloesung umstellen
    if(fullscreen)
    {
        // Aktuelle Framerate holen und die spaeter dann benutzen
        DEVMODE prev;
        EnumDisplaySettings(0, ENUM_CURRENT_SETTINGS, &prev);

        DEVMODE dm;
        memset(&dm, 0, sizeof(dm));
        dm.dmSize = sizeof(dm);
        dm.dmFields = DM_DISPLAYFREQUENCY | DM_PELSWIDTH | DM_PELSHEIGHT;
        dm.dmDisplayFrequency = prev.dmDisplayFrequency;
        dm.dmPelsWidth = width;
        dm.dmPelsHeight = height;

        ChangeDisplaySettings(&dm, CDS_FULLSCREEN);
    }

    this->screenWidth  = width;
    this->screenHeight = height;
    this->isFullscreen_ = fullscreen;

    // Das Fenster anzeigen
    ShowWindow(screen, SW_SHOW);
    // Das Fenster in den Vordergrund rcken
    SetForegroundWindow(screen);
    // Dem Fenster den Eingabefokus geben
    SetFocus(screen);

    return true;
}

///////////////////////////////////////////////////////////////////////////////
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
 *
 *  @author FloSoft
 */
bool VideoWinAPI::ResizeScreen(unsigned short width, unsigned short height, const bool fullscreen)
{
    if(!initialized || !isWindowResizable)
        return false;

    if(this->isFullscreen_ && !fullscreen)
        ChangeDisplaySettings(NULL, 0);

    ShowWindow(screen, SW_HIDE);

    // Fensterstyle ggf. ändern
    DWORD wStyle   = fullscreen ? WS_POPUP : (WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_OVERLAPPEDWINDOW | WS_SYSMENU | WS_MINIMIZEBOX | WS_CAPTION);
    DWORD wExStyle = fullscreen ? WS_EX_APPWINDOW : (WS_EX_APPWINDOW | WS_EX_WINDOWEDGE);
    SetWindowLongPtr(screen, GWL_STYLE, wStyle);
    SetWindowLongPtr(screen, GWL_EXSTYLE, wExStyle);
    //SetWindowPos(screen, NULL, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);

    RECT wRect;
    if(fullscreen)
    {
        wRect.left = 0;
        wRect.top  = 0;
    }else{
        RECT workArea;
        SystemParametersInfo(SPI_GETWORKAREA, 0, &workArea, 0);
        unsigned short waWidth  = static_cast<unsigned short>(workArea.right - workArea.left);
        unsigned short waHeight = static_cast<unsigned short>(workArea.bottom - workArea.top);
        width = std::min(width, waWidth);
        height = std::min(height, waHeight);
        wRect.left = (waWidth - width)  / 2;
        wRect.top  = (waHeight - height) / 2;
    }
    wRect.right  = wRect.left + width;
    wRect.bottom = wRect.top  + height;
    // Calculate real right/bottom based on the window style
    AdjustWindowRectEx(&wRect, wStyle, false, wExStyle);

    // Fenstergröße ändern
    UINT flags = SWP_SHOWWINDOW | SWP_DRAWFRAME | SWP_FRAMECHANGED;

    this->screenWidth = width;
    this->screenHeight = height;
    this->isFullscreen_ = fullscreen;

    SetWindowPos(screen, HWND_TOP, wRect.left, wRect.top, wRect.right - wRect.left, wRect.bottom - wRect.top, flags);

    // Bei Vollbild Auflösung umstellen
    if(fullscreen)
    {
        DEVMODE dm;
        memset(&dm, 0, sizeof(dm));
        dm.dmSize = sizeof(dm);
        dm.dmFields = DM_PELSWIDTH | DM_PELSHEIGHT;
        dm.dmPelsWidth = width;
        dm.dmPelsHeight = height;

        ChangeDisplaySettings(&dm, CDS_FULLSCREEN);
    }

    // Das Fenster anzeigen
    ShowWindow(screen, SW_SHOW);
    // Das Fenster in den Vordergrund rcken
    SetForegroundWindow(screen);
    // Dem Fenster den Eingabefokus geben
    SetFocus(screen);

    return true;
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  Schliesst das Fenster.
 *
 *  @author FloSoft
 */
void VideoWinAPI::DestroyScreen(void)
{
    // Fenster schliessen
    EndDialog(screen, 0);

    if(dm_prev.dmBitsPerPel != 0)
        ChangeDisplaySettings(&dm_prev, CDS_RESET);

    if(screen_rc)
    {
        if(!wglMakeCurrent(NULL, NULL))
            return;

        if(!wglDeleteContext(screen_rc))
            return;

        screen_rc = NULL;
    }

    if((screen_dc) && (!ReleaseDC(screen, screen_dc)))
        return;

    screen_dc = NULL;

    if((screen) && (!DestroyWindow(screen)))
        return;

    screen = NULL;

    UnregisterClassA(GetWindowTitle(), GetModuleHandle(NULL));

    isFullscreen_ = false;
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  Wechselt die OpenGL-Puffer.
 *
 *  @return @p true bei Erfolg, @p false bei Fehler
 *
 *  @author FloSoft
 */
bool VideoWinAPI::SwapBuffers(void)
{
    if(!screen_dc)
        return false;

    // Puffer wechseln
    ::SwapBuffers(screen_dc);

    return true;
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  Die Nachrichtenschleife.
 *
 *  @return @p true bei Erfolg, @p false bei Fehler
 *
 *  @author FloSoft
 */
bool VideoWinAPI::MessageLoop(void)
{
    MSG msg;
    if(PeekMessage(&msg, screen, 0, 0, PM_REMOVE))
    {
        if(msg.message == WM_QUIT)
            return false;

        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return true;
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  Funktion zum Auslesen des TickCounts.
 *
 *  @return liefert den TickCount
 *
 *  @author FloSoft
 */
unsigned long VideoWinAPI::GetTickCount(void) const
{
    return ::GetTickCount();
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  Funktion zum Holen einer Subfunktion.
 *
 *  @param[in] function Name der Funktion welche geholt werden soll.
 *
 *  @return Adresse der Funktion bei Erfolg, @p NULL bei Fehler
 *
 *  @author FloSoft
 */
void* VideoWinAPI::GetFunction(const char* function) const
{
    union
    {
        PROC procAddress;
        void* func;
    };// Avoid warning about pointer conversion
    procAddress = wglGetProcAddress(function);
    return func;
}

/// Listet verfügbare Videomodi auf
void VideoWinAPI::ListVideoModes(std::vector<VideoMode>& video_modes) const
{
    DEVMODE dm;
    unsigned m = 0;
    while(EnumDisplaySettings(NULL, m++, &dm))
    {
        // Prüfen, ob es die Auflösung nicht schonmal gab (kann es noch mit unterschiedlichen Bit-Tiefen geben
        bool already_in_vector = false;
        for(size_t i = 0; i < video_modes.size(); ++i)
        {
            if(dm.dmPelsWidth == video_modes[i].width && dm.dmPelsHeight == video_modes[i].height)
            {
                already_in_vector = true;
                break;
            }
        }

        if(already_in_vector)
            continue;

        // Es gibt die Auflösung noch nicht --> hinzufügen
        VideoMode vm(static_cast<unsigned short>(dm.dmPelsWidth), static_cast<unsigned short>(dm.dmPelsHeight));
        video_modes.push_back(vm);
    }
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  Funktion zum Setzen der Mauskoordinaten.
 *
 *  @param[in] x X-Koordinate
 *  @param[in] y Y-Koordinate
 *
 *  @author FloSoft
 */
void VideoWinAPI::SetMousePos(int x, int y)
{
    mouse_xy.x = x;
    mouse_xy.y = y;

    POINT p = {x, y};
    ClientToScreen(screen, &p);
    SetCursorPos(p.x, p.y);
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  Funktion zum Setzen der X-Koordinate der Maus.
 *
 *  @param[in] x X-Koordinate
 *
 *  @author FloSoft
 */
void VideoWinAPI::SetMousePosX(int x)
{
    SetMousePos(x, mouse_xy.y);
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  Funktion zum Setzen der Y-Koordinate der Maus.
 *
 *  @param[in] y Y-Koordinate
 *
 *  @author FloSoft
 */
void VideoWinAPI::SetMousePosY(int y)
{
    SetMousePos(mouse_xy.x, y);
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  Funktion zum Senden einer gedrückten Taste.
 *
 *  @param[in] c Tastencode
 *
 *  @author FloSoft
 */
void VideoWinAPI::OnWMChar(unsigned int c, bool disablepaste, LPARAM lParam)
{
    // Keine Leerzeichen als Extra-Zeichen senden!
    if(c == ' ')
        return;

    KeyEvent ke = {KT_CHAR, c,
                   (GetKeyState(VK_CONTROL) & 0x8000) != 0,
                   (GetKeyState(VK_SHIFT)   & 0x8000) != 0,
                   (lParam& KF_ALTDOWN) != 0
                  };

    if(c == 'V' || c == 'v' || c == 0x16)
        if( !disablepaste && ke.ctrl != 0)
        {
            OnWMPaste();
            return;
        }

    CallBack->Msg_KeyDown(ke);
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  Funktion zum Senden einer gedrückten Taste.
 *
 *  @param[in] c Tastencode
 *
 *  @author FloSoft
 */
void VideoWinAPI::OnWMKeyDown(unsigned int c, LPARAM lParam)
{
    KeyEvent ke = {KT_INVALID, 0,
                   (GetKeyState(VK_CONTROL) & 0x8000) != 0,
                   (GetKeyState(VK_SHIFT)   & 0x8000) != 0,
                   (lParam& KF_ALTDOWN) != 0
                  };

    switch(c)
    {
        case VK_RETURN:
        {
            // Don't report Alt+Return events, as WinAPI seems to fire them in a lot of cases
            ke.kt = KT_RETURN;
            ke.alt = false;
        } break;
        case VK_SPACE:  ke.kt = KT_SPACE; break;
        case VK_LEFT:   ke.kt = KT_LEFT; break;
        case VK_RIGHT:  ke.kt = KT_RIGHT; break;
        case VK_UP:     ke.kt = KT_UP; break;
        case VK_DOWN:   ke.kt = KT_DOWN; break;
        case VK_BACK:   ke.kt = KT_BACKSPACE; break;
        case VK_DELETE: ke.kt = KT_DELETE; break;
        case VK_LSHIFT: ke.kt = KT_SHIFT; break;
        case VK_RSHIFT: ke.kt = KT_SHIFT; break;
        case VK_TAB:    ke.kt = KT_TAB; break;
        case VK_END:    ke.kt = KT_END; break;
        case VK_HOME:   ke.kt = KT_HOME; break;
        case VK_ESCAPE: ke.kt = KT_ESCAPE; break;
        default:
        {
            // Die 12 F-Tasten
            if(c >= VK_F1 && c <= VK_F12)
                ke.kt = static_cast<KeyType>(KT_F1 + int(c) - VK_F1);
        } break;
    }

    if(ke.kt != KT_INVALID)
        CallBack->Msg_KeyDown(ke);
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  Funktion zum Pasten von Text aus dem Clipboard.
 *
 *  @author FloSoft
 */
void VideoWinAPI::OnWMPaste()
{
    if (!IsClipboardFormatAvailable(CF_TEXT))
        return;

    OpenClipboard(NULL);

    HANDLE hData = GetClipboardData(CF_TEXT);
    const char* pData = (const char*)GlobalLock(hData);

    while( pData && *pData )
    {
        char c = *(pData++);
        OnWMKeyDown( c );
        OnWMChar( c, true );
    }

    GlobalUnlock(hData);
    CloseClipboard();
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  Callbackfunktion der WinAPI.
 *
 *  @param[in] window Fensterhandle
 *  @param[in] msg    Fensternachricht
 *  @param[in] wParam Erster Nachrichtenparameter
 *  @param[in] wParam Zweiter Nachrichtenparameter
 *
 *  @author FloSoft
 */
LRESULT CALLBACK VideoWinAPI::WindowProc(HWND window, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch(msg)
    {
        case WM_PASTE:
        {
            pVideoWinAPI->OnWMPaste();
        } break;
        case WM_CLOSE:
        {
            PostQuitMessage(0);
            return 0;
        } break;
        case WM_SIZE:
            if(wParam != SIZE_MAXIMIZED && wParam != SIZE_RESTORED)
                break;
            if(pVideoWinAPI->screenWidth != LOWORD(lParam) || pVideoWinAPI->screenHeight != HIWORD(lParam))
            {
                pVideoWinAPI->screenWidth = LOWORD(lParam);
                pVideoWinAPI->screenHeight = HIWORD(lParam);

                pVideoWinAPI->isWindowResizable = false;
                pVideoWinAPI->CallBack->ScreenResized(pVideoWinAPI->screenWidth, pVideoWinAPI->screenHeight);
                pVideoWinAPI->isWindowResizable = true;
            }
            break;
        case WM_ACTIVATE:
        {
            switch(wParam)
            {
                default:
                case WA_ACTIVE:
                {
                    ShowCursor(0);
                } break;
                case WA_INACTIVE:
                {
                    ShowCursor(1);
                } break;
            }
        } break;
        case WM_SYSCOMMAND:
        {
            switch (wParam)
            {
                case SC_SCREENSAVE:
                case SC_MONITORPOWER:
                    return 0;
                case SC_KEYMENU: // F10-Fehler beheben -> will sonst Fenster verschieben, was das Zeichnen unterbindet
                    pVideoWinAPI->OnWMKeyDown(VK_F10, 0); // pretend we got a F10 stroke
                    return 0;
            }
        } break;
        case WM_MOUSEMOVE:
        {
            pVideoWinAPI->mouse_xy.x = LOWORD(lParam);
            pVideoWinAPI->mouse_xy.y = HIWORD(lParam);
            pVideoWinAPI->CallBack->Msg_MouseMove(pVideoWinAPI->mouse_xy);
        } break;
        case WM_LBUTTONDOWN:
        {
            pVideoWinAPI->mouse_l = true;
            pVideoWinAPI->mouse_xy.ldown = true;
            pVideoWinAPI->CallBack->Msg_LeftDown(pVideoWinAPI->mouse_xy);
        } break;
        case WM_LBUTTONUP:
        {
            pVideoWinAPI->mouse_l = false;
            pVideoWinAPI->mouse_xy.ldown = false;
            pVideoWinAPI->CallBack->Msg_LeftUp(pVideoWinAPI->mouse_xy);
        } break;
        case WM_RBUTTONDOWN:
        {
            pVideoWinAPI->mouse_r = true;
            pVideoWinAPI->mouse_xy.rdown = true;
            pVideoWinAPI->CallBack->Msg_RightDown(pVideoWinAPI->mouse_xy);
        } break;
        case WM_RBUTTONUP:
        {
            pVideoWinAPI->mouse_r = false;
            pVideoWinAPI->mouse_xy.rdown = false;
            pVideoWinAPI->CallBack->Msg_RightUp(pVideoWinAPI->mouse_xy);
        } break;
        case WM_MOUSEWHEEL:
        {
            // Obtain scrolling distance. For every multiple of WHEEL_DELTA, we have to fire an event, because we treat the wheel like two buttons.
            // One wheel "step" usually produces a mouse_z  of +/- WHEEL_DELTA. But there may exist wheels without "steps" that result in lower values we have to cumulate.
            pVideoWinAPI->mouse_z += GET_WHEEL_DELTA_WPARAM(wParam);

            // We don't want to crash if there were even wheels that produce higher values...
            while (std::abs(pVideoWinAPI->mouse_z) >= WHEEL_DELTA)
            {
                if (pVideoWinAPI->mouse_z > 0) // Scrolled to top
                {
                    pVideoWinAPI->mouse_z -= WHEEL_DELTA;
                    pVideoWinAPI->CallBack->Msg_WheelUp(pVideoWinAPI->mouse_xy);
                }
                else // Scrolled to bottom
                {
                    pVideoWinAPI->mouse_z += WHEEL_DELTA;
                    pVideoWinAPI->CallBack->Msg_WheelDown(pVideoWinAPI->mouse_xy);
                }
            }
        } break;
        case WM_KEYDOWN:
//  case WM_SYSKEYDOWN: // auch abfangen, wenn linkes ALT mit gedrückt wurde
        {
            pVideoWinAPI->OnWMKeyDown((unsigned int)wParam, lParam);
        } return 0;
        case WM_CHAR:
        case WM_SYSCHAR: // auch abfangen, wenn linkes ALT mit gedrückt wurde
        {
            pVideoWinAPI->OnWMChar((unsigned int)wParam, false, lParam);
        } return 0;
    }
    return DefWindowProcW(window, msg, wParam, lParam);
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  Get state of the modifier keys
 *
 *  @author Divan
 */
KeyEvent VideoWinAPI::GetModKeyState(void) const
{
    const KeyEvent ke = { KT_INVALID, 0,
                          (GetKeyState(VK_CONTROL) & 0x8000) != 0,
                          (GetKeyState(VK_SHIFT)   & 0x8000) != 0,
                          (GetKeyState(VK_MENU)    & 0x8000) != 0
                        };
    return ke;
}

/// Gibt Pointer auf ein Fenster zurück (device-dependent!), HWND unter Windows
void* VideoWinAPI::GetMapPointer() const
{
    return (void*)this->screen;
}
