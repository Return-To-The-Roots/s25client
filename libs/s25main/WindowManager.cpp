// Copyright (C) 2005 - 2024 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "WindowManager.h"
#include "CollisionDetection.h"
#include "Loader.h"
#include "RttrConfig.h"
#include "Settings.h"
#include "Window.h"
#include "commonDefines.h"
#include "desktops/Desktop.h"
#include "drivers/ScreenResizeEvent.h"
#include "drivers/VideoDriverWrapper.h"
#include "files.h"
#include "helpers/pointerContainerUtils.h"
#include "helpers/reverse.h"
#include "ingameWindows/IngameWindow.h"
#include "ogl/FontStyle.h"
#include "ogl/SoundEffectItem.h"
#include "ogl/glFont.h"
#include "ogl/saveBitmap.h"
#include "gameData/const_gui_ids.h"
#include "libsiedler2/PixelBufferBGRA.h"
#include "s25util/Log.h"
#include "s25util/MyTime.h"
#include <algorithm>

WindowManager::WindowManager()
    : cursor_(Cursor::Hand), disable_mouse(false), lastMousePos(Position::Invalid()), curRenderSize(0, 0),
      lastLeftClickTime(0), lastLeftClickPos(0, 0)
{}

WindowManager::~WindowManager() = default;

void WindowManager::CleanUp()
{
    windows.clear();
    curDesktop.reset();
    nextdesktop.reset();
}

void WindowManager::SetCursor(Cursor cursor)
{
    cursor_ = cursor;
}

void WindowManager::DrawCursor()
{
    auto resId = static_cast<unsigned>(cursor_);
    switch(cursor_)
    {
        case Cursor::Hand:
        case Cursor::Remove: resId += VIDEODRIVER.IsLeftDown() ? 1 : 0; break;
        default: break;
    }
    if(resId)
        LOADER.GetImageN("resource", resId)->DrawFull(VIDEODRIVER.GetMousePos());
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
    for(auto& wnd : windows)
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
    DrawCursor();
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
    // When there is no desktop, don't check it or any window
    if(!curDesktop)
        return;
    if(curDesktop->IsActive())
    {
        // Desktop active -> relay msg to desktop
        CALL_MEMBER_FN(*curDesktop, msg)(ke);
        curDesktop->RelayKeyboardMessage(msg, ke);
        return;
    }

    if(windows.empty())
        return; // No windows -> nothing to do

    // ESC or ALT+W closes the active window
    const auto escape = (ke.kt == KeyType::Escape);
    if(escape || (ke.c == 'w' && ke.alt))
    {
        // Find one which isn't yet marked for closing so multiple ESC in between draw calls can close multiple windows
        // ESC doesn't close pinned windows
        const auto itActiveWnd = std::find_if(windows.rbegin(), windows.rend(), [escape](const auto& wnd) {
            return !wnd->ShouldBeClosed() && !(escape && wnd->IsPinned());
        });
        if(itActiveWnd != windows.rend() && (*itActiveWnd)->getCloseBehavior() != CloseBehavior::Custom)
            (*itActiveWnd)->Close();
    } else if(!CALL_MEMBER_FN(*windows.back(), msg)(ke)) // send to active window
    {
        // If not handled yet, relay to active window
        if(!windows.back()->RelayKeyboardMessage(msg, ke))
        {
            // If message was not handled send to desktop
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
IngameWindow& WindowManager::DoShow(std::unique_ptr<IngameWindow> window, bool mouse)
{
    RTTR_Assert(window);
    RTTR_Assert(!helpers::contains(windows, window));
    // No desktop -> Out
    if(!curDesktop)
        throw std::runtime_error("No desktop active for window to be shown on");

    SetToolTip(nullptr, "");

    // All windows are inserted before the first modal window (shown behind)
    auto itModal = helpers::find_if(windows, [](const auto& curWnd) { return curWnd->IsModal(); });
    // Note that if there is no other modal window it will be put at the back which is what we want
    auto& result = **windows.emplace(itModal, std::move(window));

    // Make the new window active (special cases handled in the function)
    SetActiveWindow(result);

    // Maus deaktivieren, bis sie losgelassen wurde (Fix des Switch-Anschließend-Drück-Bugs)
    disable_mouse = mouse;
    return result;
}

IngameWindow* WindowManager::ShowAfterSwitch(std::unique_ptr<IngameWindow> window)
{
    RTTR_Assert(window);
    nextWnds.emplace_back(std::move(window));
    return nextWnds.back().get();
}

/**
 *  merkt einen Desktop zum Wechsel vor.
 *
 *  @param[in] desktop       Pointer zum neuen Desktop, auf dem gewechselt werden soll
 *  @param[in] data          Daten für den neuen Desktop
 */
Desktop* WindowManager::Switch(std::unique_ptr<Desktop> desktop)
{
    nextdesktop = std::move(desktop);
    // Disable the mouse till the next desktop is shown to avoid e.g. double-switching
    disable_mouse = true;
    return nextdesktop.get();
}

IngameWindow* WindowManager::FindWindowAtPos(const Position& pos) const
{
    // Fenster durchgehen ( von hinten nach vorn, da die vordersten ja zuerst geprüft werden müssen !! )
    for(const auto& window : helpers::reverse(windows))
    {
        // FensterRect für Kollisionsabfrage
        Rect window_rect = window->GetDrawRect();

        // trifft die Maus auf ein Fenster?
        if(IsPointInRect(pos, window_rect))
        {
            return window.get();
        }
        // Check also if we are in the locked area of a window (e.g. dropdown extends outside of window)
        if(window->IsInLockedRegion(pos))
            return window.get();
    }
    return nullptr;
}

IngameWindow* WindowManager::FindNonModalWindow(unsigned id) const
{
    auto itWnd = helpers::find_if(
      windows, [id](const auto& wnd) { return !wnd->ShouldBeClosed() && !wnd->IsModal() && wnd->GetID() == id; });
    return itWnd == windows.end() ? nullptr : itWnd->get();
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

    IngameWindow* foundWindow = FindWindowAtPos(mc.pos);

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
    if(time_now - lastLeftClickTime < DOUBLE_CLICK_INTERVAL && mc.pos == lastLeftClickPos)
    {
        mc.dbl_click = true;
    } else
    {
        // Werte wieder erneut speichern
        lastLeftClickPos = mc.pos;
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

    // Right-click closes (most) windows, so check that
    if(!windows.empty())
    {
        IngameWindow* foundWindow = FindWindowAtPos(mc.pos);
        if(windows.back()->IsModal())
        {
            // We have a modal window -> Activate it
            SetActiveWindow(*windows.back());
            // Ignore actions in all other windows
            if(foundWindow != GetTopMostWindow())
                return;
        }
        if(foundWindow)
        {
            // Close it if requested (unless pinned)
            if(foundWindow->getCloseBehavior() == CloseBehavior::Regular)
            {
                if(!foundWindow->IsPinned())
                    foundWindow->Close();
            } else
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

    IngameWindow* foundWindow = FindWindowAtPos(mc.pos);

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
    IngameWindow* foundWindow = FindWindowAtPos(mc.pos);

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
    if(ke.alt && (ke.kt == KeyType::Return))
    {
        // Switch Fullscreen/Windowed
        const auto newScreenSize =
          !SETTINGS.video.fullscreen ? SETTINGS.video.fullscreenSize : SETTINGS.video.windowedSize; //-V807
        VIDEODRIVER.ResizeScreen(newScreenSize, !SETTINGS.video.fullscreen);
        SETTINGS.video.fullscreen = VIDEODRIVER.IsFullscreen();
    } else if(ke.kt == KeyType::Print)
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
    for(const auto& window : windows)
    {
        DrawPoint delta = window->GetPos() + DrawPoint(window->GetSize()) - DrawPoint(sr.newSize);
        if(delta.x > 0 || delta.y > 0)
            window->SetPos(window->GetPos() - elMax(delta, DrawPoint(0, 0)));
    }
}

IngameWindow* WindowManager::GetTopMostWindow() const
{
    if(windows.empty())
        return nullptr;
    else
        return windows.back().get();
}

void WindowManager::DoClose(IngameWindow* window)
{
    const auto it = helpers::findPtr(windows, window);

    RTTR_Assert(it != windows.end());

    SetToolTip(nullptr, "");

    // Store if this was the active window
    const bool isActiveWnd = window == GetTopMostWindow();

    // Remove from list and notify parent, hold onto it till parent is notified
    const auto tmpHolder = std::move(*it);
    windows.erase(it);
    if(isActiveWnd)
    {
        if(windows.empty())
            SetActiveWindow(*curDesktop);
        else
            SetActiveWindow(*windows.back());
    }
    curDesktop->Msg_WindowClosed(*tmpHolder);
}

/**
 *  Closes _ALL_ windows with the given ID
 *
 *  @param[in] id ID of the window to be closed
 */
void WindowManager::Close(unsigned id)
{
    for(auto& wnd : windows)
    {
        if(wnd->GetID() == id && !wnd->ShouldBeClosed())
            wnd->Close();
    }
}

void WindowManager::CloseNow(IngameWindow* window)
{
    if(!window->ShouldBeClosed())
        window->Close();
    DoClose(window);
}

/**
 *  Actually process the desktop change
 */
void WindowManager::DoDesktopSwitch()
{
    RTTR_Assert(nextdesktop);
    VIDEODRIVER.ClearScreen();

    SetToolTip(nullptr, "");

    // If we have a current desktop close all windows
    if(curDesktop)
        windows.clear();

    // Do the switch
    curDesktop = std::move(nextdesktop);
    curDesktop->SetActive(true);

    for(auto& nextWnd : nextWnds)
        Show(std::move(nextWnd));
    nextWnds.clear();

    if(!VIDEODRIVER.IsLeftDown())
        disable_mouse = false;

    // Dummy mouse move to init hovering etc
    Msg_MouseMove(MouseCoords(VIDEODRIVER.GetMousePos()));
}

void WindowManager::CloseMarkedIngameWnds()
{
    auto isWndMarkedForClose = [](const auto& wnd) { return wnd->ShouldBeClosed(); };
    auto it = helpers::find_if(windows, isWndMarkedForClose);
    while(it != windows.end())
    {
        DoClose(it->get());
        it = helpers::find_if(windows, isWndMarkedForClose);
    }
}

template<class T_Windows>
void SetActiveWindowImpl(const Window& wnd, Desktop& desktop, T_Windows& windows)
{
    auto itWnd = helpers::find_if(windows, [&wnd](const auto& it) { return it.get() == &wnd; });
    if(itWnd != windows.end())
    {
        // If we have a modal window, don't make this active unless it is the top most one
        if(&wnd != windows.back().get() && helpers::contains_if(windows, [](const auto& it) { return it->IsModal(); }))
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
    for(const auto& curWnd : windows)
        curWnd->SetActive(&wnd == curWnd.get());
}

void WindowManager::SetActiveWindow(Window& wnd)
{
    if(curDesktop)
        SetActiveWindowImpl(wnd, *curDesktop, windows);
    if(nextdesktop) // NOLINTNEXTLINE(clang-analyzer-cplusplus.Move)
        SetActiveWindowImpl(wnd, *nextdesktop, nextWnds);
}

void WindowManager::TakeScreenshot() const
{
    const auto windowSize = VIDEODRIVER.GetWindowSize();
    libsiedler2::PixelBufferBGRA buffer(windowSize.width, windowSize.height);
    glReadPixels(0, 0, windowSize.width, windowSize.height, GL_BGRA, GL_UNSIGNED_BYTE, buffer.getPixelPtr());
    flipVertical(buffer);
    const bfs::path outFilepath =
      RTTRCONFIG.ExpandPath(s25::folders::screenshots) / (s25util::Time::FormatTime("%Y-%m-%d_%H-%i-%s") + ".bmp");
    try
    {
        saveBitmap(buffer, outFilepath);
        LOG.write(_("Screenshot saved to %1%\n")) % outFilepath;
    } catch(const std::runtime_error& e)
    {
        LOG.write(_("Error writing screenshot: %1%\n")) % e.what();
    }
}

class WindowManager::Tooltip
{
    static constexpr unsigned BORDER_SIZE = 2;
    const ctrlBaseTooltip* showingCtrl;
    const glFont* font;
    std::vector<std::string> lines;
    unsigned width = 0, height = 0;
    unsigned short maxWidth;

public:
    Tooltip(const ctrlBaseTooltip* showingCtrl, const std::string& text, unsigned short maxWidth)
        : showingCtrl(showingCtrl), font(NormalFont), maxWidth(maxWidth)
    {
        setText(text);
    }

    auto getShowingCtrl() const { return showingCtrl; }
    auto getWidth() const { return width; }

    void setText(const std::string& text)
    {
        lines = font->GetWrapInfo(text, maxWidth, maxWidth).CreateSingleStrings(text);
        if(lines.empty())
            return;
        width = 0;
        for(const auto& line : lines)
            width = std::max(width, font->getWidth(line));
        width += BORDER_SIZE * 2;
        height = lines.size() * font->getHeight() + BORDER_SIZE * 2;
    }

    void draw(DrawPoint pos) const
    {
        Window::DrawRectangle(Rect(pos, width, height), 0x9F000000);
        pos += DrawPoint::all(BORDER_SIZE);
        const auto fontHeight = font->getHeight();
        for(const auto& line : lines)
        {
            font->Draw(pos, line, FontStyle::TOP, COLOR_YELLOW);
            pos.y += fontHeight;
        }
    }
};

void WindowManager::SetToolTip(const ctrlBaseTooltip* ttw, const std::string& tooltip, bool updateCurrent)
{
    // Max width of tooltip
    constexpr unsigned short MAX_TOOLTIP_WIDTH = 260;

    if(tooltip.empty())
    {
        if(curTooltip && (!ttw || curTooltip->getShowingCtrl() == ttw))
            curTooltip.reset();
    } else if(updateCurrent)
    {
        if(curTooltip && curTooltip->getShowingCtrl() == ttw)
            curTooltip->setText(tooltip);
    } else
        curTooltip = std::make_unique<Tooltip>(ttw, tooltip, MAX_TOOLTIP_WIDTH);
}

void WindowManager::DrawToolTip()
{
    // Tooltip zeichnen
    if(curTooltip && lastMousePos.isValid())
    {
        constexpr unsigned cursorWidth = 32;
        constexpr unsigned cursorPadding = 5;
        // Horizontal space between mouse position and tooltip border
        constexpr unsigned rightSpacing = cursorWidth + cursorPadding;
        constexpr unsigned leftSpacing = cursorPadding;
        DrawPoint ttPos = DrawPoint(lastMousePos.x + rightSpacing, lastMousePos.y);
        unsigned right_edge = ttPos.x + curTooltip->getWidth();

        // links neben der Maus, wenn es über den Rand gehen würde
        if(right_edge > curRenderSize.x)
            ttPos.x = lastMousePos.x - leftSpacing - curTooltip->getWidth();
        curTooltip->draw(ttPos);
    }
}

SnapOffset WindowManager::snapWindow(Window* wnd, const Rect& wndRect) const
{
    const auto snapDistance = static_cast<int>(SETTINGS.interface.windowSnapDistance);
    if(snapDistance == 0)
        return SnapOffset::all(0); // No snapping

    /// Progressive minimum distance to another window edge; initial value limits to at most snapDistance
    auto minDist = Extent::all(snapDistance + 1);
    /// (Smallest) offset (signed distance) the window should be moved; initially zero, so no snapping
    SnapOffset minOffset = SnapOffset::all(0);

    const auto setOffsetIfLowerDist = [](int a, int b, unsigned& minDist, int& minOffset) {
        const int offset = b - a;
        const unsigned dist = std::abs(offset);
        if(dist < minDist)
        {
            minDist = dist;
            minOffset = offset;
        }
    };
    const auto setOffsetIfLowerDistX = [&](int x1, int x2) { setOffsetIfLowerDist(x1, x2, minDist.x, minOffset.x); };
    const auto setOffsetIfLowerDistY = [&](int y1, int y2) { setOffsetIfLowerDist(y1, y2, minDist.y, minOffset.y); };

    for(const auto& curWnd : windows)
    {
        if(curWnd.get() == wnd)
            continue;

        const Rect curWndRect = curWnd->GetBoundaryRect();

        // Calculate smallest offset for each parallel pair of window edges, iff the windows overlap along the
        // orthogonal axis (± snap distance)
        if(wndRect.top <= (curWndRect.bottom + snapDistance) && wndRect.bottom >= (curWndRect.top - snapDistance))
        {
            setOffsetIfLowerDistX(wndRect.left, curWndRect.left);
            setOffsetIfLowerDistX(wndRect.left, curWndRect.right);
            setOffsetIfLowerDistX(wndRect.right, curWndRect.left);
            setOffsetIfLowerDistX(wndRect.right, curWndRect.right);
        }

        if(wndRect.left <= (curWndRect.right + snapDistance) && wndRect.right >= (curWndRect.left - snapDistance))
        {
            setOffsetIfLowerDistY(wndRect.top, curWndRect.top);
            setOffsetIfLowerDistY(wndRect.top, curWndRect.bottom);
            setOffsetIfLowerDistY(wndRect.bottom, curWndRect.top);
            setOffsetIfLowerDistY(wndRect.bottom, curWndRect.bottom);
        }
    }

    return minOffset;
}
