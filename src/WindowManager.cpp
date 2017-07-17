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

#include "defines.h" // IWYU pragma: keep
#include "WindowManager.h"

#include "Settings.h"
#include "drivers/VideoDriverWrapper.h"
#include "Loader.h"
#include "CollisionDetection.h"
#include "Window.h"
#include "desktops/Desktop.h"
#include "ingameWindows/IngameWindow.h"
#include "drivers/ScreenResizeEvent.h"
#include "ogl/glArchivItem_Font.h"
#include "ogl/glArchivItem_Sound.h"
#include "Log.h"
#include "gameData/const_gui_ids.h"
#include <boost/foreach.hpp>
#include <algorithm>

WindowManager::WindowManager():
    disable_mouse(false), lastMousePos(Point<int>::Invalid()),
    screenSize(0, 0), lastLeftClickTime(0), lastLeftClickPos(0, 0)
{}

WindowManager::~WindowManager()
{
    CleanUp();
}

void WindowManager::CleanUp()
{
    for(IgwListIterator it = windows.begin(); it != windows.end(); ++it)
        delete (*it);
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
        Switch();

    // haben wir einen gültigen Desktop?
    if(!curDesktop)
        return;

    // ja, Msg_PaintBefore aufrufen
    curDesktop->Msg_PaintBefore();

    // und Desktop zeichnen
    curDesktop->Draw();

    // First close all marked windows
    CloseMarkedIngameWnds();
    BOOST_FOREACH(IngameWindow* wnd, windows)
    {
        // If the window is not minimized, call paintAfter
        if(!wnd->IsMinimized())
            wnd->Msg_PaintBefore();
    }
    BOOST_FOREACH(IngameWindow* wnd, windows)
        wnd->Draw();
    BOOST_FOREACH(IngameWindow* wnd, windows)
    {
        // If the window is not minimized, call paintAfter
        if(!wnd->IsMinimized())
            wnd->Msg_PaintAfter();
    }

    DrawToolTip();

    // Msg_PaintAfter aufrufen
    curDesktop->Msg_PaintAfter();
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
    }
    else if(!windows.empty())
    {
        // Nein, dann Nachricht an letztes Fenster weiterleiten
        CALL_MEMBER_FN(*windows.back() ,msg)(mc);
        windows.back()->RelayMouseMessage(msg, mc);
    }
}



/**
 *  Öffnet ein IngameWindow und fügt es zur Fensterliste hinzu.
 *
 *  @param[in] desktop       Pointer zum neuen Desktop, auf dem gewechselt werden soll
 *  @param[in] data          Daten für den neuen Desktop
 *  @param[in] disable_mouse Bei true wird bis zum nächsten Release die Maus deaktiviert (Switch-Anschließend-Drück-Bug)
 */
void WindowManager::Show(IngameWindow* window, bool mouse)
{
    RTTR_Assert(window);
    SetToolTip(NULL, "");

    // haben wir ein gültiges Fenster erhalten?
    if(!window)
        return;

    // haben wir einen gültigen Desktop?
    if(!curDesktop)
        return;

    // Check for already open windows with same ID
    for(IgwListIterator it = windows.begin(); it != windows.end(); ++it)
    {
        // Skip ones that are about to be closed
        if((*it)->ShouldBeClosed())
            continue;

        if(window->id_ == (*it)->id_)
        {
            // Special cases:
            // 1) Help windows simply replace other help windows
            if(window->id_ == CGI_HELP)
                (*it)->Close();
            else if(window->id_ == CGI_MISSION_STATEMENT || window->id_ == CGI_MSGBOX)
            {
                // 2) Mission statement and msg boxes get prepended (they are modal, so old needs to be closed first)
                windows.insert(it, window);
                return;
            }else
            {
                // Same ID -> Close and don't open again
                (*it)->Close();
                delete window;
                return;
            }
        }
    }

    // Fenster hinzufügen
    windows.push_back(window);

    // Desktop deaktivieren
    curDesktop->SetActive(false);

    // alle anderen Fenster deaktivieren
    for(IgwListIterator it = windows.begin(); it != windows.end(); ++it)
        (*it)->SetActive(false);

    // Fenster aktivieren
    window->SetActive(true);

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
 *  @param[in] disable_mouse Bei true wird bis zum nächsten Release die Maus deaktiviert (Switch-Anschließend-Drück-Bug)
 */
void WindowManager::Switch(Desktop* desktop, bool mouse)
{
    nextdesktop.reset(desktop);
    disable_mouse = mouse;
}

IngameWindow* WindowManager::FindWindowUnderMouse(const MouseCoords& mc) const{
    // Fenster durchgehen ( von hinten nach vorn, da die vordersten ja zuerst geprüft werden müssen !! )
    for(std::list<IngameWindow*>::const_reverse_iterator it = windows.rbegin(); it != windows.rend(); ++it)
    {
        // FensterRect für Kollisionsabfrage
        Rect window_rect = (*it)->GetDrawRect();

        // trifft die Maus auf ein Fenster?
        if(IsPointInRect(mc.x, mc.y, window_rect)){
            return *it;
        }
        // Check also if we are in the locked area of a window (e.g. dropdown extends outside of window)
        if((*it)->TestWindowInRegion(NULL, mc))
            return *it;
    }
    return NULL;
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
    LOADER.GetSoundN("sound", 112)->Play(255, false);

    // haben wir überhaupt fenster?
    if(windows.empty())
    {
        // nein, dann Desktop aktivieren
        curDesktop->SetActive(true);

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
            lastActiveWnd.SetActive();

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

    IngameWindow* foundWindow = FindWindowUnderMouse(mc);

    // aktives Fenster deaktivieren
    lastActiveWnd.SetActive(false);

    // Haben wir ein Fenster gefunden gehabt?
    if(foundWindow){
        // Fenster aus der Liste holen und vorne wieder anhängen
        windows.remove(foundWindow);
        windows.push_back(foundWindow);

        // aktivieren
        foundWindow->SetActive(true);

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

        // Desktop deaktivieren, falls aktiviert
        curDesktop->SetActive(false);
    }else{
        // Desktop aktivieren
        curDesktop->SetActive(true);

        // ist der Maus-Klick-Fix aktiv?
        if(!disable_mouse)
        {
            // nein, dann Msg_LeftDown aufrufen
            curDesktop->Msg_LeftDown(mc);

            // und allen unten drunter auch Bescheid sagen
            curDesktop->RelayMouseMessage(&Window::Msg_LeftDown, mc);;
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
    if((time_now - lastLeftClickTime) * 1000 / CLOCKS_PER_SEC < DOUBLE_CLICK_INTERVAL
        && mc.GetPos() == lastLeftClickPos)
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
        }
        else if(windows.back())
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
        IngameWindow* foundWindow = FindWindowUnderMouse(mc);
        if(windows.back()->IsModal())
        {
            // We have a modal window -> Activate it
            curDesktop->SetActive(false);
            windows.back()->SetActive(true);
            // Ignore actions in all other windows
            if(foundWindow != windows.back())
                return;
        }
        if(foundWindow){
            // Close it if requested
            if (foundWindow->GetCloseOnRightClick())
                foundWindow->Close();
            else
            {
                windows.back()->SetActive(false);

                // Fenster aus der Liste holen und vorne wieder anhängen
                windows.remove(foundWindow);
                windows.push_back(foundWindow);

                curDesktop->SetActive(false);

                foundWindow->SetActive(true);
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
    }
    else if(!windows.empty())
    {
        // dann Nachricht an Fenster weiterleiten
        windows.back()->RelayMouseMessage(&Window::Msg_RightDown, mc);
    }

    // letztes Fenster deaktivieren, da ja nun der Desktop aktiv werden soll
    if(!windows.empty())
        windows.back()->SetActive(false);

    // Desktop aktivieren
    curDesktop->SetActive(true);

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
        // nein, dann Desktop aktivieren
        curDesktop->SetActive(true);

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

    IngameWindow* foundWindow = FindWindowUnderMouse(mc);
    // ja, also aktives Fenster deaktivieren
    activeWnd.SetActive(false);

    if(foundWindow)
    {
        // Fenster aus der Liste holen und vorne wieder anhängen
        windows.remove(foundWindow);
        windows.push_back(foundWindow);

        // ja, dann aktivieren
        foundWindow->SetActive(true);

        // dann Msg_WheelUp aufrufen
        foundWindow->Msg_WheelUp(mc);

        // und allen unten drunter auch Bescheid sagen
        foundWindow->RelayMouseMessage(&Window::Msg_WheelUp, mc);

        // Desktop deaktivieren, falls aktiviert
        curDesktop->SetActive(false);
    }else {
        // Desktop aktivieren
        curDesktop->SetActive(true);

        // nein, dann Msg_WheelUpDown aufrufen
        curDesktop->Msg_WheelUp(mc);

        // und allen unten drunter auch Bescheid sagen
        curDesktop->RelayMouseMessage(&Window::Msg_WheelUp, mc);;
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
        curDesktop->SetActive(true);
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
    IngameWindow* foundWindow = FindWindowUnderMouse(mc);
    activeWnd.SetActive(false);

    if(foundWindow)
    {
        windows.remove(foundWindow);
        windows.push_back(foundWindow);
        foundWindow->SetActive(true);
        foundWindow->Msg_WheelDown(mc);
        foundWindow->RelayMouseMessage(&Window::Msg_WheelDown, mc);
        curDesktop->SetActive(false);
    }else{
        curDesktop->SetActive(true);
        curDesktop->Msg_WheelDown(mc);
        curDesktop->RelayMouseMessage(&Window::Msg_WheelDown, mc);;
    }
}

/**
 *  Verarbeitung des Verschiebens der Maus.
 *
 *  @param[in] mc Mauskoordinaten Struktur
 */
void WindowManager::Msg_MouseMove(const MouseCoords& mc)
{
    lastMousePos = Point<int>(mc.x, mc.y);

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
    }
    else if(!windows.empty())
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
#ifdef _WIN32
        VIDEODRIVER.ResizeScreen(SETTINGS.video.fullscreen_width,
                                                SETTINGS.video.fullscreen_height,
                                                !VIDEODRIVER.IsFullscreen());
#else
        VIDEODRIVER.ResizeScreen(VIDEODRIVER.IsFullscreen() ? SETTINGS.video.windowed_width : SETTINGS.video.fullscreen_width,
                                                VIDEODRIVER.IsFullscreen() ? SETTINGS.video.windowed_height : SETTINGS.video.fullscreen_height,
                                                !VIDEODRIVER.IsFullscreen());
#endif
    }
    else
        RelayKeyboardMessage(&Window::Msg_KeyDown, ke);
}

/**
 *  Verarbeitung Spielfenstergröße verändert (vom Betriebssystem aus)
 *  Liefert evtl. eine Größe die wir nicht wollen und daher korrigieren
 *  oder falls ok durchlassen.
 *  Eigentliche Verarbeitung dann in Msg_ScreenResize.
 *
 *  @param[in] width  neue Breite
 *  @param[in] height neue Höhe
 */
void WindowManager::ScreenResized(unsigned short newWidth, unsigned short newHeight)
{
    VIDEODRIVER.RenewViewport();
    Msg_ScreenResize(VIDEODRIVER.GetScreenSize());
    LOG.writeToFile("Resized screen. Requested %ux%u, got %ux%u\n") % newWidth % newHeight % VIDEODRIVER.GetScreenWidth() % VIDEODRIVER.GetScreenHeight();
}

/**
 *  Verarbeitung Spielfenstergröße verändert (vom Spiel aus)
 *  Liefert immer eine sinnvolle Größe, mind. 800x600.
 *
 *  @param[in] width  neue Breite
 *  @param[in] height neue Höhe
 */
void WindowManager::Msg_ScreenResize(const Extent& newSize)
{
    // Falls sich nichts ändert, brauchen wir auch nicht reagieren
    // (Evtl hat sich ja nur der Modus Fenster/Vollbild geändert)
    if(newSize == screenSize)
        return;

    ScreenResizeEvent sr(screenSize, Extent(std::max(800u, newSize.x), std::max(600u, newSize.y)));
    screenSize = sr.newSize;

    SETTINGS.video.fullscreen = VIDEODRIVER.IsFullscreen(); //-V807

    if(!SETTINGS.video.fullscreen)
    {
        if(newSize.x  >= 800) SETTINGS.video.windowed_width  = newSize.x;
        if(newSize.y >= 600) SETTINGS.video.windowed_height = newSize.y;
    }

    // ist unser Desktop gültig?
    if(!curDesktop)
        return;

    curDesktop->Msg_ScreenResize(sr);

    // IngameWindow verschieben falls nötig, so dass sie komplett sichtbar sind
    for(IgwListIterator it = windows.begin(); it != windows.end(); ++it)
    {
        DrawPoint delta = (*it)->GetPos() + DrawPoint((*it)->GetSize()) - DrawPoint(sr.newSize);
        if(delta.x > 0 || delta.y > 0)
            (*it)->Move(std::min(-delta.x, 0), -std::min(-delta.y, 0), /*absolute=*/false);
    }
}


const Window* WindowManager::GetTopMostWindow() const
{
    if(windows.empty())
        return NULL;
    else
        return windows.back();
}

struct IsWindowId
{
    const unsigned id;
    IsWindowId(const unsigned id) : id(id) {}

    bool operator()(IngameWindow* wnd)
    {
        return wnd->GetID() == id;
    }
};

void WindowManager::Close(const IngameWindow* window)
{
    // ist das Fenster gültig?
    if(!window)
        return;

    IgwListIterator it = std::find(windows.begin(), windows.end(), window);
    if(it == windows.end())
        return; // Window already closed -> Out

    SetToolTip(NULL, "");

    // War es an vorderster Stelle?
    if(window == windows.back())
    {
        // haben wir noch Fenster zum aktivieren?
        if(window != windows.front())
        {
            // ja, also das nächste aktivieren. Save it as we need it later!
            IgwListIterator tmp = it;
            --tmp;

            // Activate window or desktop if window invalid
            (*tmp)->SetActive(true);
        }
        else
        {
            // nein, also Desktop aktivieren
            curDesktop->SetActive(true);
        }
    }

    // Fenster löschen
    delete window;
    // und aus der Liste entfernen
    windows.erase(it);
}

/**
 *  Closes _ALL_ windows with the given ID
 *
 *  @param[in] id ID des/der Fenster(s) welche(s) geschlossen werden soll
 */
void WindowManager::Close(unsigned int id)
{
    IgwListIterator it = std::find_if(windows.begin(), windows.end(), IsWindowId(id));
    while (it != windows.end())
    {
        Close(*it);
        it = std::find_if(windows.begin(), windows.end(), IsWindowId(id));
    }
}


/**
 *  wechselt den Desktop in den neuen Desktop
 */
void WindowManager::Switch()
{
    RTTR_Assert(nextdesktop);
    VIDEODRIVER.ClearScreen();

    SetToolTip(NULL, "");

    // haben wir einen aktuell gültigen Desktop?
    if(curDesktop)
    {
        // Alle (alten) Fenster zumachen
        for(IgwListIterator it = windows.begin(); it != windows.end(); ++it)
            delete (*it);
        windows.clear();
    }

    // Desktop auf Neuen umstellen
    curDesktop.reset(nextdesktop.release());
    curDesktop->SetActive(true);

    for(std::vector<IngameWindow*>::iterator it = nextWnds.begin(); it != nextWnds.end(); ++it)
        Show(*it);
    nextWnds.clear();

    // Dummy mouse move to init hovering etc
    Msg_MouseMove(MouseCoords(VIDEODRIVER.GetMouseX(), VIDEODRIVER.GetMouseY(), false, false, false));
}

struct IsWndMarkedForClose
{
    bool operator()(const IngameWindow* wnd) const
    {
        return wnd->ShouldBeClosed();
    }
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

void WindowManager::SetToolTip(const Window* ttw, const std::string& tooltip)
{
    // Max width of tooltip
    const unsigned short MAX_TOOLTIP_WIDTH = 260;
    static const Window* lttw = NULL;

    if(tooltip.empty() && (!ttw || lttw == ttw))
        this->curTooltip.clear();
    else if(!tooltip.empty())
    {
        glArchivItem_Font::WrapInfo wi = NormalFont->GetWrapInfo(tooltip, MAX_TOOLTIP_WIDTH, MAX_TOOLTIP_WIDTH);
        std::vector<std::string> lines = wi.CreateSingleStrings(tooltip);
        curTooltip.clear();
        for(std::vector<std::string>::iterator it = lines.begin(); it != lines.end(); ++it)
        {
            if(!curTooltip.empty())
                curTooltip += "\n";
            curTooltip += *it;
        }
        lttw = ttw;
    }
}

void WindowManager::DrawToolTip()
{
    // Tooltip zeichnen
    if(curTooltip.length() && lastMousePos.isValid())
    {
        const unsigned spacing = 30;
        unsigned text_width = NormalFont->getWidth(curTooltip);
        DrawPoint ttPos = DrawPoint(lastMousePos.x + spacing, lastMousePos.y);
        unsigned right_edge = ttPos.x + text_width + 2;

        // links neben der Maus, wenn es über den Rand gehen würde
        if(right_edge > VIDEODRIVER.GetScreenWidth() )
            ttPos.x = lastMousePos.x - spacing - text_width;

        unsigned int numLines = 1;
        size_t pos = curTooltip.find('\n');
        while(pos != std::string::npos)
        {
            numLines++;
            pos = curTooltip.find('\n', pos + 1);
        }

        Window::DrawRectangle(ttPos - DrawPoint(2, 2), text_width + 4, 4 + numLines * NormalFont->getDy(), 0x9F000000);
        NormalFont->Draw(ttPos, curTooltip, glArchivItem_Font::DF_TOP, COLOR_YELLOW);
    }
}
