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

#include "rttrDefines.h" // IWYU pragma: keep
#include "WindowManager.h"
#include "CollisionDetection.h"
#include "Loader.h"
#include "RttrConfig.h"
#include "Settings.h"
#include "Window.h"
#include "desktops/Desktop.h"
#include "drivers/ScreenResizeEvent.h"
#include "drivers/VideoDriverWrapper.h"
#include "files.h"
#include "helpers/containerUtils.h"
#include "ingameWindows/IngameWindow.h"
#include "ogl/FontStyle.h"
#include "ogl/SoundEffectItem.h"
#include "ogl/glArchivItem_Font.h"
#include "gameData/const_gui_ids.h"
#include "libsiedler2/ArchivItem_Bitmap_Raw.h"
#include "libsiedler2/ErrorCodes.h"
#include "libsiedler2/PixelBufferARGB.h"
#include "libsiedler2/libsiedler2.h"
#include "libutil/Log.h"
#include "libutil/MyTime.h"
#include <algorithm>

WindowManager::WindowManager()
    : disable_mouse(false), lastMousePos(Position::Invalid()), curRenderSize(0, 0), lastLeftClickTime(0), lastLeftClickPos(0, 0)
{}

WindowManager::~WindowManager()
{
    CleanUp();
}

void WindowManager::CleanUp()
{
    for(auto window : windows)
        delete window;
    windows.clear();

    curDesktop.reset();
    nextdesktop.reset();
}

/**
 *  Zeichenfunktion WindowManager-Klasse.
 *  Zeichnet Desktop und alle Fenster.
 */
void WindowManager::Draw()
{
    // ist ein neuer Desktop eingetragen? Wenn ja, wechseln
    if(nextdesktop)
        DoDesktopSwitch();

    if(!curDesktop)
        return;

    curDesktop->Msg_PaintBefore();
    curDesktop->Draw();
    curDesktop->Msg_PaintAfter();

    // First close all marked windows
    CloseMarkedIngameWnds();
    for(IngameWindow* wnd : windows)
    {
        // If the window is not minimized, call paintAfter
        if(!wnd->IsMinimized())
            wnd->Msg_PaintBefore();
        wnd->Draw();
        // If the window is not minimized, call paintAfter
        if(!wnd->IsMinimized())
            wnd->Msg_PaintAfter();
    }

    DrawToolTip();
}

/**
 *  liefert ob der aktuelle Desktop den Focus besitzt oder nicht.
 *
 *  @return liefert @p true bei aktivem Desktop,
 *                  @p false wenn der Desktop nicht den Fokus besitzt.
 */
bool WindowManager::IsDesktopActive()
{
    if(curDesktop)
        return curDesktop->IsActive();

    return false;
}

/**
 *  schickt eine Nachricht an das aktive Fenster bzw den aktiven Desktop.
 *
 *  @param[in] msg   Nachricht welche geschickt werden soll
 *  @param[in] id    ID des Steuerelements
 *  @param[in] param Parameter der Nachricht
 */

/// Sendet eine Tastaturnachricht an die Fenster.
void WindowManager::RelayKeyboardMessage(KeyboardMsgHandler msg, const KeyEvent& ke)
{
    // ist der Desktop gültig?
    if(!curDesktop)
        return;
    // ist der Desktop aktiv?
    if(curDesktop->IsActive())
    {
        // Ja, dann Nachricht an Desktop weiterleiten
        CALL_MEMBER_FN(*curDesktop, msg)(ke);
        curDesktop->RelayKeyboardMessage(msg, ke);
        return;
    }

    if(windows.empty())
        return; // No windows -> nothing to do

    // Letztes Fenster schließen? (Escape oder Alt-W)
    if(ke.kt == KT_ESCAPE || (ke.c == 'w' && ke.alt))
    {
        Close(windows.back());
        return;
    }
    // Nein, dann Nachricht an letztes Fenster weiterleiten
    if(!CALL_MEMBER_FN(*windows.back(), msg)(ke))
    {
        if(!windows.back()->RelayKeyboardMessage(msg, ke))
        {
            // Falls Nachrichten nicht behandelt wurden, an Desktop wieder senden
            CALL_MEMBER_FN(*curDesktop, msg)(ke);
            curDesktop->RelayKeyboardMessage(msg, ke);
        }
    }
}

/// Sendet eine Mausnachricht weiter an alle Fenster
void WindowManager::RelayMouseMessage(MouseMsgHandler msg, const MouseCoords& mc)
{
    // ist der Desktop gültig?
    if(!curDesktop)
        return;
    // ist der Desktop aktiv?
    if(curDesktop->IsActive())
    {
        // Ja, dann Nachricht an Desktop weiterleiten
        CALL_MEMBER_FN(*curDesktop, msg)(mc);
        curDesktop->RelayMouseMessage(msg, mc);
    } else if(!windows.empty())
    {
        // Nein, dann Nachricht an letztes Fenster weiterleiten
        CALL_MEMBER_FN(*windows.back(), msg)(mc);
        windows.back()->RelayMouseMessage(msg, mc);
    }
}

/**
 *  Öffnet ein IngameWindow und fügt es zur Fensterliste hinzu.
 */
void WindowManager::Show(IngameWindow* window, bool mouse)
{
    RTTR_Assert(window);
    RTTR_Assert(!helpers::contains(windows, window));

    SetToolTip(nullptr, "");

    // No desktop -> Out
    if(!curDesktop)
        return;

    // Non-Modal windows can only be opened once
    if(!window->IsModal())
    {
        // Check for already open windows with same ID (ignoring to-be-closed windows)
        auto itOther = helpers::findPred(windows, [window](const IngameWindow* curWnd) {
            return !curWnd->ShouldBeClosed() && !curWnd->IsModal() && curWnd->GetID() == window->GetID();
        });
        if(itOther != windows.end())
        {
            // Special case:
            // Help windows simply replace other help windows
            if(window->GetID() == CGI_HELP)
                (*itOther)->Close();
            else
            {
                // Same ID -> Close and don't open again
                (*itOther)->Close();
                delete window;
                return;
            }
        }
    }

    // All windows are inserted before the first modal window (shown behind)
    auto itModal = helpers::findPred(windows, [](const IngameWindow* curWnd) { return curWnd->IsModal(); });
    // Note that if there is no other modal window it will be put at the back which is what we want
    windows.insert(itModal, window);

    // Make the new window active (special cases handled in the function)
    SetActiveWindow(*window);

    // Maus deaktivieren, bis sie losgelassen wurde (Fix des Switch-Anschließend-Drück-Bugs)
    disable_mouse = mouse;
}

void WindowManager::ShowAfterSwitch(IngameWindow* window)
{
    RTTR_Assert(window);
    nextWnds.push_back(window);
}

/**
 *  merkt einen Desktop zum Wechsel vor.
 *
 *  @param[in] desktop       Pointer zum neuen Desktop, auf dem gewechselt werden soll
 *  @param[in] data          Daten für den neuen Desktop
 */
void WindowManager::Switch(Desktop* desktop)
{
    nextdesktop.reset(desktop);
    // Disable the mouse till the next desktop is shown to avoid e.g. double-switching
    disable_mouse = true;
}

IngameWindow* WindowManager::FindWindowAtPos(const Position& pos) const
{
    // Fenster durchgehen ( von hinten nach vorn, da die vordersten ja zuerst geprüft werden müssen !! )
    for(std::list<IngameWindow*>::const_reverse_iterator it = windows.rbegin(); it != windows.rend(); ++it)
    {
        // FensterRect für Kollisionsabfrage
        Rect window_rect = (*it)->GetDrawRect();

        // trifft die Maus auf ein Fenster?
        if(IsPointInRect(pos, window_rect))
        {
            return *it;
        }
        // Check also if we are in the locked area of a window (e.g. dropdown extends outside of window)
        if((*it)->IsInLockedRegion(pos))
            return *it;
    }
    return nullptr;
}

/**
 *  Verarbeitung des Drückens der Linken Maustaste.
 *
 *  @param[in] mc Mauskoordinaten Struktur
 */
void WindowManager::Msg_LeftDown(MouseCoords mc)
{
    // ist unser Desktop gültig?
    if(!curDesktop)
        return;

    // Sound abspielen
    SoundEffectItem* sound = LOADER.GetSoundN("sound", 112);
    if(sound)
        sound->Play(255, false);

    // haben wir überhaupt fenster?
    if(windows.empty())
    {
        // nein, dann Desktop aktivieren
        SetActiveWindow(*curDesktop);

        // ist der Maus-Klick-Fix aktiv?
        if(!disable_mouse)
        {
            // nein, Msg_LeftDown aufrufen
            curDesktop->Msg_LeftDown(mc);

            // und allen unten drunter auch Bescheid sagen
            curDesktop->RelayMouseMessage(&Window::Msg_LeftDown, mc);
        }

        // und raus
        return;
    }

    // ist das zuletzt aktiv gewesene Fenster Modal?
    IngameWindow& lastActiveWnd = *windows.back();
    if(lastActiveWnd.IsModal())
    {
        if(!lastActiveWnd.IsActive())
            SetActiveWindow(lastActiveWnd);

        // ja es ist modal, ist der Maus-Klick-Fix aktiv?
        if(!disable_mouse)
        {
            // nein, Msg_LeftDownaufrufen
            lastActiveWnd.Msg_LeftDown(mc);

            // und allen unten drunter auch Bescheid sagen
            lastActiveWnd.RelayMouseMessage(&Window::Msg_LeftDown, mc);

            // und noch MouseLeftDown vom Fenster aufrufen
            lastActiveWnd.MouseLeftDown(mc);
        }

        // und raus
        return;
    }

    IngameWindow* foundWindow = FindWindowAtPos(mc.GetPos());

    // Haben wir ein Fenster gefunden gehabt?
    if(foundWindow)
    {
        SetActiveWindow(*foundWindow);

        // ist der Maus-Klick-Fix aktiv?
        if(!disable_mouse)
        {
            // nein, dann Msg_LeftDown aufrufen
            foundWindow->Msg_LeftDown(mc);

            // und allen unten drunter auch Bescheid sagen
            foundWindow->RelayMouseMessage(&Window::Msg_LeftDown, mc);

            // und noch MouseLeftDown vom Fenster aufrufen
            foundWindow->MouseLeftDown(mc);
        }
    } else
    {
        SetActiveWindow(*curDesktop);

        // ist der Maus-Klick-Fix aktiv?
        if(!disable_mouse)
        {
            // nein, dann Msg_LeftDown aufrufen
            curDesktop->Msg_LeftDown(mc);

            // und allen unten drunter auch Bescheid sagen
            curDesktop->RelayMouseMessage(&Window::Msg_LeftDown, mc);
        }
    }
}

/**
 *  Verarbeitung des Loslassens der Linken Maustaste.
 *
 *  @param[in] mc Mauskoordinaten Struktur
 */
void WindowManager::Msg_LeftUp(MouseCoords mc)
{
    // ist unser Desktop gültig?
    if(!curDesktop)
        return;

    // Ggf. Doppelklick untersuche
    unsigned time_now = VIDEODRIVER.GetTickCount();
    if(time_now - lastLeftClickTime < DOUBLE_CLICK_INTERVAL && mc.GetPos() == lastLeftClickPos)
    {
        mc.dbl_click = true;
    } else
    {
        // Werte wieder erneut speichern
        lastLeftClickPos = mc.GetPos();
        lastLeftClickTime = time_now;
    }

    // ist der Maus-Klick-Fix aktiv?
    if(!disable_mouse)
    {
        // ist der Desktop aktiv?
        if(curDesktop->IsActive())
        {
            // ja, dann Msg_LeftUp aufrufen
            curDesktop->Msg_LeftUp(mc);

            // und die Fenster darunter auch
            curDesktop->RelayMouseMessage(&Window::Msg_LeftUp, mc);
        } else if(!windows.empty())
        {
            // ja, dann Msg_LeftUp aufrufen
            IngameWindow& activeWnd = *windows.back();
            activeWnd.Msg_LeftUp(mc);

            // und den anderen Fenstern auch Bescheid geben
            activeWnd.RelayMouseMessage(&Window::Msg_LeftUp, mc);

            // und noch MouseLeftUp vom Fenster aufrufen
            activeWnd.MouseLeftUp(mc);
        }
    }

    // Maus-Klick-Fix deaktivieren
    if(disable_mouse && !nextdesktop)
        disable_mouse = false;
}

/**
 *  Verarbeitung des Drückens der Rechten Maustaste.
 *
 *  @param[in] mc Mauskoordinaten Struktur
 */
void WindowManager::Msg_RightDown(const MouseCoords& mc)
{
    // ist unser Desktop gültig?
    if(!curDesktop)
        return;

    // Sind Fenster vorhanden && ist das aktive Fenster ok
    if(!windows.empty())
    {
        IngameWindow* foundWindow = FindWindowAtPos(mc.GetPos());
        if(windows.back()->IsModal())
        {
            // We have a modal window -> Activate it
            SetActiveWindow(*windows.back());
            // Ignore actions in all other windows
            if(foundWindow != windows.back())
                return;
        }
        if(foundWindow)
        {
            // Close it if requested
            if(foundWindow->GetCloseOnRightClick())
                foundWindow->Close();
            else
            {
                SetActiveWindow(*foundWindow);
                foundWindow->Msg_RightDown(mc);
            }
            return;
        }
    }

    // ist der Desktop aktiv?
    if(curDesktop->IsActive())
    {
        // ja, dann Msg_RightDown aufrufen
        curDesktop->Msg_RightDown(mc);

        // und die Fenster darunter auch
        curDesktop->RelayMouseMessage(&Window::Msg_RightDown, mc);
    } else if(!windows.empty())
    {
        // dann Nachricht an Fenster weiterleiten
        windows.back()->RelayMouseMessage(&Window::Msg_RightDown, mc);
    }

    SetActiveWindow(*curDesktop);

    // ja, dann Msg_RightDown aufrufen
    curDesktop->Msg_RightDown(mc);

    // und die Fenster darunter auch
    curDesktop->RelayMouseMessage(&Window::Msg_RightDown, mc);
}

void WindowManager::Msg_RightUp(const MouseCoords& mc)
{
    RelayMouseMessage(&Window::Msg_RightUp, mc);
}

/**
 *  Verarbeitung Mausrad hoch.
 *
 *  @param[in] mc Mauskoordinaten Struktur
 */
void WindowManager::Msg_WheelUp(const MouseCoords& mc)
{
    // ist unser Desktop gültig?
    if(!curDesktop)
        return;

    // haben wir überhaupt fenster?
    if(windows.empty())
    {
        SetActiveWindow(*curDesktop);

        // nein, Msg_LeftDown aufrufen
        curDesktop->Msg_WheelUp(mc);

        // und allen unten drunter auch Bescheid sagen
        curDesktop->RelayMouseMessage(&Window::Msg_WheelUp, mc);

        // und raus
        return;
    }

    // ist das zuletzt aktiv gewesene Fenster Modal?
    IngameWindow& activeWnd = *windows.back();
    if(activeWnd.IsModal())
    {
        // Msg_LeftDownaufrufen
        activeWnd.Msg_WheelUp(mc);

        // und allen unten drunter auch Bescheid sagen
        activeWnd.RelayMouseMessage(&Window::Msg_WheelUp, mc);

        // und raus
        return;
    }

    IngameWindow* foundWindow = FindWindowAtPos(mc.GetPos());

    if(foundWindow)
    {
        SetActiveWindow(*foundWindow);

        // dann Msg_WheelUp aufrufen
        foundWindow->Msg_WheelUp(mc);

        // und allen unten drunter auch Bescheid sagen
        foundWindow->RelayMouseMessage(&Window::Msg_WheelUp, mc);
    } else
    {
        SetActiveWindow(*curDesktop);

        // nein, dann Msg_WheelUpDown aufrufen
        curDesktop->Msg_WheelUp(mc);

        // und allen unten drunter auch Bescheid sagen
        curDesktop->RelayMouseMessage(&Window::Msg_WheelUp, mc);
    }
}

/**
 *  Verarbeitung Mausrad runter
 *
 *  @param[in] mc Mauskoordinaten Struktur
 */
void WindowManager::Msg_WheelDown(const MouseCoords& mc)
{
    if(!curDesktop)
        return;
    if(windows.empty())
    {
        SetActiveWindow(*curDesktop);
        curDesktop->Msg_WheelDown(mc);
        curDesktop->RelayMouseMessage(&Window::Msg_WheelDown, mc);
        // und raus
        return;
    }
    IngameWindow& activeWnd = *windows.back();
    if(activeWnd.IsModal())
    {
        activeWnd.Msg_WheelDown(mc);
        activeWnd.RelayMouseMessage(&Window::Msg_WheelDown, mc);
        return;
    }
    IngameWindow* foundWindow = FindWindowAtPos(mc.GetPos());

    if(foundWindow)
    {
        SetActiveWindow(*foundWindow);
        foundWindow->Msg_WheelDown(mc);
        foundWindow->RelayMouseMessage(&Window::Msg_WheelDown, mc);
    } else
    {
        SetActiveWindow(*curDesktop);
        curDesktop->Msg_WheelDown(mc);
        curDesktop->RelayMouseMessage(&Window::Msg_WheelDown, mc);
    }
}

/**
 *  Verarbeitung des Verschiebens der Maus.
 *
 *  @param[in] mc Mauskoordinaten Struktur
 */
void WindowManager::Msg_MouseMove(const MouseCoords& mc)
{
    lastMousePos = mc.pos;

    // ist unser Desktop gültig?
    if(!curDesktop)
        return;

    // nein, ist unser Desktop aktiv?
    if(curDesktop->IsActive())
    {
        // ja, dann Msg_MouseMove aufrufen
        curDesktop->Msg_MouseMove(mc);

        // und alles drunter auch benachrichtigen
        curDesktop->RelayMouseMessage(&Window::Msg_MouseMove, mc);
    } else if(!windows.empty())
    {
        IngameWindow& activeWnd = *windows.back();
        // und MouseMove vom Fenster aufrufen
        activeWnd.MouseMove(mc);

        // ja, dann Msg_MouseMove aufrufen
        activeWnd.Msg_MouseMove(mc);

        // und alles drunter auch benachrichtigen
        activeWnd.RelayMouseMessage(&Window::Msg_MouseMove, mc);
    }
}

void WindowManager::Msg_KeyDown(const KeyEvent& ke)
{
    if(ke.alt && (ke.kt == KT_RETURN))
    {
        // Switch Fullscreen/Windowed
        const auto newScreenSize = !SETTINGS.video.fullscreen ? SETTINGS.video.fullscreenSize : SETTINGS.video.windowedSize; //-V807
        VIDEODRIVER.ResizeScreen(newScreenSize, !SETTINGS.video.fullscreen);
        SETTINGS.video.fullscreen = VIDEODRIVER.IsFullscreen();
    } else if(ke.kt == KT_PRINT)
        TakeScreenshot();
    else
        RelayKeyboardMessage(&Window::Msg_KeyDown, ke);
}

/**
 *  Handle resize of the window or change of resolution
 */
void WindowManager::WindowResized()
{
    VIDEODRIVER.RenewViewport();
    Msg_ScreenResize(VIDEODRIVER.GetRenderSize());
}

/**
 *  React to change of the render size
 */
void WindowManager::Msg_ScreenResize(const Extent& newSize)
{
    // Don't handle it if nothing changed
    if(newSize == curRenderSize)
        return;

    ScreenResizeEvent sr(curRenderSize, elMax(Extent(800, 600), newSize));
    curRenderSize = sr.newSize;

    // Don't change fullscreen size (only in menu)
    if(!SETTINGS.video.fullscreen)
        SETTINGS.video.windowedSize = VIDEODRIVER.GetWindowSize();

    // ist unser Desktop gültig?
    if(!curDesktop)
        return;

    curDesktop->Msg_ScreenResize(sr);

    // IngameWindow verschieben falls nötig, so dass sie komplett sichtbar sind
    for(auto window : windows)
    {
        DrawPoint delta = window->GetPos() + DrawPoint(window->GetSize()) - DrawPoint(sr.newSize);
        if(delta.x > 0 || delta.y > 0)
            window->SetPos(window->GetPos() - elMax(delta, DrawPoint(0, 0)));
    }
}

const IngameWindow* WindowManager::GetTopMostWindow() const
{
    if(windows.empty())
        return nullptr;
    else
        return windows.back();
}

struct IsWindowId
{
    const unsigned id;
    IsWindowId(const unsigned id) : id(id) {}

    bool operator()(IngameWindow* wnd) { return wnd->GetID() == id; }
};

void WindowManager::Close(const IngameWindow* window)
{
    // ist das Fenster gültig?
    if(!window)
        return;

    IgwListIterator it = std::find(windows.begin(), windows.end(), window);
    if(it == windows.end())
        return; // Window already closed -> Out

    SetToolTip(nullptr, "");

    // War es an vorderster Stelle?
    const bool isActiveWnd = window == windows.back();

    // Remove from list and notify parent
    windows.erase(it);
    if(isActiveWnd)
    {
        if(windows.empty())
            SetActiveWindow(*curDesktop);
        else
            SetActiveWindow(*windows.back());
    }
    curDesktop->Msg_WindowClosed(const_cast<IngameWindow&>(*window));
    // Then delete
    delete window;
}

/**
 *  Closes _ALL_ windows with the given ID
 *
 *  @param[in] id ID des/der Fenster(s) welche(s) geschlossen werden soll
 */
void WindowManager::Close(unsigned id)
{
    IgwListIterator it = std::find_if(windows.begin(), windows.end(), IsWindowId(id));
    while(it != windows.end())
    {
        Close(*it);
        it = std::find_if(windows.begin(), windows.end(), IsWindowId(id));
    }
}

/**
 *  wechselt den Desktop in den neuen Desktop
 */
void WindowManager::DoDesktopSwitch()
{
    RTTR_Assert(nextdesktop);
    VIDEODRIVER.ClearScreen();

    SetToolTip(nullptr, "");

    // haben wir einen aktuell gültigen Desktop?
    if(curDesktop)
    {
        // Alle (alten) Fenster zumachen
        for(auto window : windows)
            delete window;
        windows.clear();
    }

    // Desktop auf Neuen umstellen
    curDesktop.reset(nextdesktop.release());
    curDesktop->SetActive(true);

    for(auto nextWnd : nextWnds)
        Show(nextWnd);
    nextWnds.clear();

    if(!VIDEODRIVER.IsLeftDown())
        disable_mouse = false;

    // Dummy mouse move to init hovering etc
    Msg_MouseMove(MouseCoords(VIDEODRIVER.GetMousePos()));
}

struct IsWndMarkedForClose
{
    bool operator()(const IngameWindow* wnd) const { return wnd->ShouldBeClosed(); }
};

void WindowManager::CloseMarkedIngameWnds()
{
    IgwListIterator it = std::find_if(windows.begin(), windows.end(), IsWndMarkedForClose());
    while(it != windows.end())
    {
        Close(*it);
        it = std::find_if(windows.begin(), windows.end(), IsWndMarkedForClose());
    }
}

template<class T_Windows>
void SetActiveWindowImpl(Window& wnd, Desktop& desktop, T_Windows& windows)
{
    auto itWnd = helpers::find(windows, &wnd);
    if(itWnd != windows.end())
    {
        // If we have a modal window, don't make this active unless it is the top most one
        if(&wnd != windows.back() && helpers::containsPred(windows, [](const IngameWindow* wnd) { return wnd->IsModal(); }))
        {
            return;
        }
        desktop.SetActive(false);
        // Move window to the end (itWnd+1 -> itWnd -> end())
        std::rotate(itWnd, std::next(itWnd), windows.end());
    } else if(&wnd == &desktop)
        desktop.SetActive(true);
    else
        return;
    for(auto curWnd : windows)
        curWnd->SetActive(curWnd == &wnd);
}

void WindowManager::SetActiveWindow(Window& wnd)
{
    if(curDesktop)
        SetActiveWindowImpl(wnd, *curDesktop, windows);
    if(nextdesktop)
        SetActiveWindowImpl(wnd, *nextdesktop, nextWnds);
}

void WindowManager::TakeScreenshot()
{
    libsiedler2::PixelBufferARGB buffer(curRenderSize.x, curRenderSize.y);
    glReadPixels(0, 0, curRenderSize.x, curRenderSize.y, GL_BGRA, GL_UNSIGNED_BYTE, buffer.getPixelPtr());
    libsiedler2::ArchivItem_Bitmap_Raw* bmp = new libsiedler2::ArchivItem_Bitmap_Raw;
    libsiedler2::Archiv archive;
    archive.push(bmp);
    bmp->create(buffer);
    bmp->flipVertical();
    bfs::path outFilepath = bfs::path(RTTRCONFIG.ExpandPath(FILE_PATHS[100])) / (s25util::Time::FormatTime("%Y-%m-%d_%H-%i-%s") + ".bmp");
    if(int ec = libsiedler2::Write(outFilepath.string(), archive))
        LOG.write(_("Error writing screenshot: %1%\n")) % libsiedler2::getErrorString(ec);
    else
        LOG.write(_("Screenshot saved to %1%\n")) % outFilepath;
}

void WindowManager::SetToolTip(const ctrlBaseTooltip* ttw, const std::string& tooltip)
{
    // Max width of tooltip
    const unsigned short MAX_TOOLTIP_WIDTH = 260;
    static const ctrlBaseTooltip* lttw = nullptr;

    if(tooltip.empty() && (!ttw || lttw == ttw))
        this->curTooltip.clear();
    else if(!tooltip.empty())
    {
        glArchivItem_Font::WrapInfo wi = NormalFont->GetWrapInfo(tooltip, MAX_TOOLTIP_WIDTH, MAX_TOOLTIP_WIDTH);
        std::vector<std::string> lines = wi.CreateSingleStrings(tooltip);
        curTooltip.clear();
        for(const auto& line : lines)
        {
            if(!curTooltip.empty())
                curTooltip += "\n";
            curTooltip += line;
        }
        lttw = ttw;
    }
}

void WindowManager::DrawToolTip()
{
    // Tooltip zeichnen
    if(curTooltip.length() && lastMousePos.isValid())
    {
        // Horizontal pace between mouse position and tooltip border
        const unsigned rightSpacing = 30;
        const unsigned leftSpacing = 10;
        unsigned text_width = NormalFont->getWidth(curTooltip);
        DrawPoint ttPos = DrawPoint(lastMousePos.x + rightSpacing, lastMousePos.y);
        unsigned right_edge = ttPos.x + text_width + 2;

        // links neben der Maus, wenn es über den Rand gehen würde
        if(right_edge > curRenderSize.x)
            ttPos.x = lastMousePos.x - leftSpacing - text_width;

        unsigned numLines = 1;
        size_t pos = curTooltip.find('\n');
        while(pos != std::string::npos)
        {
            numLines++; //-V127
            pos = curTooltip.find('\n', pos + 1);
        }

        Rect bgRect(ttPos - DrawPoint(2, 2), text_width + 4, 4 + numLines * NormalFont->getDy());
        Window::DrawRectangle(bgRect, 0x9F000000);
        NormalFont->Draw(ttPos, curTooltip, FontStyle::TOP, COLOR_YELLOW);
    }
}
