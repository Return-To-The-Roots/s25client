﻿// $Id: WindowManager.cpp 9357 2014-04-25 15:35:25Z FloSoft $
//
// Copyright (c) 2005 - 2011 Settlers Freaks (sf-team at siedler25.org)
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
#include "defines.h"
#include "WindowManager.h"

#include "Settings.h"
#include "drivers/VideoDriverWrapper.h"
#include "Loader.h"

#include "Window.h"
#include "desktops/Desktop.h"
#include "ingameWindows/IngameWindow.h"

#include <ctime>
#include <algorithm>

///////////////////////////////////////////////////////////////////////////////
// Makros / Defines
#if defined _WIN32 && defined _DEBUG && defined _MSC_VER
#define new new(_NORMAL_BLOCK, THIS_FILE, __LINE__)
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

///////////////////////////////////////////////////////////////////////////////
/**
 *  Konstruktor von @p WindowManager.
 *
 *  @author OLiver
 */
WindowManager::WindowManager(void)
    : desktop(NULL), nextdesktop(NULL), nextdesktop_data(NULL), disable_mouse(false),
      mc(NULL), screenWidth(0), screenHeight(0), last_left_click_time(0), last_left_click_point(0, 0)
{
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  Destruktor von @p WindowManager.
 *
 *  @author OLiver
 */
WindowManager::~WindowManager(void)
{
    CleanUp();
}

void WindowManager::CleanUp()
{
    for(IgwListIterator it = windows.begin(); it != windows.end(); ++it)
        delete (*it);
    windows.clear();

    if(desktop)
        delete desktop;
    desktop = NULL;
    if(nextdesktop)
        delete nextdesktop;
    nextdesktop = NULL;

}

///////////////////////////////////////////////////////////////////////////////
/**
 *  Zeichenfunktion WindowManager-Klasse.
 *  Zeichnet Desktop und alle Fenster.
 *
 *  @author OLiver
 *  @author FloSoft
 */
void WindowManager::Draw(void)
{
    // ist ein neuer Desktop eingetragen? Wenn ja, wechseln
    if(nextdesktop)
        Switch();

    // haben wir einen gültigen Desktop?
    if(!desktop)
        return;

    // ja, Msg_PaintBefore aufrufen
    desktop->Msg_PaintBefore();

    // und Desktop zeichnen
    desktop->Draw();

    // haben wir Fenster?
    if(!windows.empty())
    {
        for(IgwListIterator it = windows.begin(); it != windows.end(); ++it)
        {
            // Soll Fenster geschlossen werden?
            if((*it)->ShouldBeClosed())
            {
                // Fenster schliessen
                Close(*it);

                // und raus (korruption der liste verhindern)
                break;
            }

            // Fenster zeichnen
            (*it)->Draw();

            // wurde es minimiert?
            if(!(*it)->GetMinimized())
            {
                // nein, Msg_PaintAfter aufrufen
                (*it)->Msg_PaintAfter();
            }
        }
    }

    DrawToolTip();

    // Msg_PaintAfter aufrufen
    desktop->Msg_PaintAfter();
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  liefert ob der aktuelle Desktop den Focus besitzt oder nicht.
 *
 *  @return liefert @p true bei aktivem Desktop,
 *                  @p false wenn der Desktop nicht den Fokus besitzt.
 *
 *  @author OLiver
 */
bool WindowManager::IsDesktopActive(void)
{
    if(desktop)
        return desktop->GetActive();

    return false;
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  schickt eine Nachricht an das aktive Fenster bzw den aktiven Desktop.
 *
 *  @param[in] msg   Nachricht welche geschickt werden soll
 *  @param[in] id    ID des Steuerelements
 *  @param[in] param Parameter der Nachricht
 *
 *  @author OLiver
 */

/// Sendet eine Tastaturnachricht an die Fenster.
void WindowManager::RelayKeyboardMessage(bool (Window::*msg)(const KeyEvent&), const KeyEvent& ke)
{
    // ist der Desktop gültig?
    if(!desktop)
        return;
    // ist der Desktop aktiv?
    if(desktop->GetActive())
    {
        // Ja, dann Nachricht an Desktop weiterleiten
        (desktop->*msg)(ke);
        desktop->RelayKeyboardMessage(msg, ke);
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
    if(!(windows.back()->*msg)(ke))
    {
        if(!windows.back()->RelayKeyboardMessage(msg, ke))
        {
            // Falls Nachrichten nicht behandelt wurden, an Desktop wieder senden
            (desktop->*msg)(ke);
            desktop->RelayKeyboardMessage(msg, ke);
        }
    }
}

/// Sendet eine Mausnachricht weiter an alle Fenster
void WindowManager::RelayMouseMessage(bool (Window::*msg)(const MouseCoords&), const MouseCoords& mc)
{
    // ist der Desktop gültig?
    if(!desktop)
        return;
    // ist der Desktop aktiv?
    if(desktop->GetActive())
    {
        // Ja, dann Nachricht an Desktop weiterleiten
        (desktop->*msg)(mc);
        desktop->RelayMouseMessage(msg, mc);
    }
    else if(!windows.empty())
    {
        // Nein, dann Nachricht an letztes Fenster weiterleiten
        (windows.back()->*msg)(mc);
        windows.back()->RelayMouseMessage(msg, mc);
    }
}



///////////////////////////////////////////////////////////////////////////////
/**
 *  Öffnet ein IngameWindow und fügt es zur Fensterliste hinzu.
 *
 *  @param[in] desktop       Pointer zum neuen Desktop, auf dem gewechselt werden soll
 *  @param[in] data          Daten für den neuen Desktop
 *  @param[in] disable_mouse Bei true wird bis zum nächsten Release die Maus deaktiviert (Switch-Anschließend-Drück-Bug)
 *
 *  @author OLiver
 */
void WindowManager::Show(IngameWindow* window, bool mouse)
{
    SetToolTip(NULL, "");

    // haben wir ein gültiges Fenster erhalten?
    if(!window)
        return;

    // haben wir einen gültigen Desktop?
    if(!desktop)
        return;

    // war das Fenster schon offen?
    for(IgwListIterator it = windows.begin(); it != windows.end(); ++it)
    {
        // Evtl wird es schon geschlossen?
        if((*it)->ShouldBeClosed())
            continue;

        if(window->id == (*it)->id)
        {
            // Ja, also vorheriges schliessen
            (*it)->Close();
            delete window;
            return;
        }
    }

    // Fenster hinzufügen
    windows.push_back(window);

    // Desktop deaktivieren
    desktop->SetActive(false);

    // alle anderen Fenster deaktivieren
    for(IgwListIterator it = windows.begin(); it != windows.end(); ++it)
    {
        (*it)->SetActive(false);
    }

    // Fenster aktivieren
    window->SetActive(true);


    // Maus deaktivieren, bis sie losgelassen wurde (Fix des Switch-Anschließend-Drück-Bugs)
    disable_mouse = mouse;
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  merkt einen Desktop zum Wechsel vor.
 *
 *  @param[in] desktop       Pointer zum neuen Desktop, auf dem gewechselt werden soll
 *  @param[in] data          Daten für den neuen Desktop
 *  @param[in] disable_mouse Bei true wird bis zum nächsten Release die Maus deaktiviert (Switch-Anschließend-Drück-Bug)
 *
 *  @author OLiver
 */
void WindowManager::Switch(Desktop* desktop, void* data, bool mouse)
{
    nextdesktop = desktop;
    nextdesktop_data = data;
    disable_mouse = mouse;
}

IngameWindow* WindowManager::FindWindowUnderMouse(const MouseCoords& mc) const{
    // Fenster durchgehen ( von hinten nach vorn, da die vordersten ja zuerst geprüft werden müssen !! )
    for(std::list<IngameWindow*>::const_reverse_iterator it = windows.rbegin(); it != windows.rend(); ++it)
    {
        // FensterRect für Kollisionsabfrage
        Rect window_rect = (*it)->GetRect();

        // trifft die Maus auf ein Fenster?
        if(Coll(mc.x, mc.y, window_rect)){
            return *it;
        }
    }
    return NULL;
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  Verarbeitung des Drückens der Linken Maustaste.
 *
 *  @param[in] mc Mauskoordinaten Struktur
 *
 *  @author OLiver
 */
void WindowManager::Msg_LeftDown(MouseCoords mc)
{
    // ist unser Desktop gültig?
    if(!desktop)
        return;

    // Sound abspielen
    LOADER.GetSoundN("sound", 112)->Play(255, false);

    // Ggf. Doppelklick untersuche
    unsigned time_now = VIDEODRIVER.GetTickCount();
    if((time_now - last_left_click_time) * 1000 / CLOCKS_PER_SEC < DOUBLE_CLICK_INTERVAL
            && Point<int>(mc.x, mc.y) == last_left_click_point)
        mc.dbl_click = true;

    // Werte wieder erneut speichern
    last_left_click_point = Point<int>(mc.x, mc.y);
    last_left_click_time = time_now;

    // haben wir überhaupt fenster?
    if(windows.empty())
    {
        // nein, dann Desktop aktivieren
        desktop->SetActive(true);

        // ist der Maus-Klick-Fix aktiv?
        if(!disable_mouse)
        {
            // nein, Msg_LeftDown aufrufen
            desktop->Msg_LeftDown(mc);

            // und allen unten drunter auch Bescheid sagen
            desktop->RelayMouseMessage(&Window::Msg_LeftDown, mc);
        }

        // und raus
        return;
    }

    // ist das zuletzt aktiv gewesene Fenster Modal?
    if( windows.back()->GetModal())
    {
        // ja es ist modal, ist der Maus-Klick-Fix aktiv?
        if(!disable_mouse)
        {
            // nein, Msg_LeftDownaufrufen
            windows.back()->Msg_LeftDown(mc);

            // und allen unten drunter auch Bescheid sagen
            windows.back()->RelayMouseMessage(&Window::Msg_LeftDown, mc);

            // und noch MouseLeftDown vom Fenster aufrufen
            windows.back()->MouseLeftDown(mc);
        }

        // und raus
        return;
    }

    IngameWindow* foundWindow = FindWindowUnderMouse(mc);

    // aktives Fenster deaktivieren
    windows.back()->SetActive(false);

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
        desktop->SetActive(false);
    }else{
        // Desktop aktivieren
        desktop->SetActive(true);

        // ist der Maus-Klick-Fix aktiv?
        if(!disable_mouse)
        {
            // nein, dann Msg_LeftDown aufrufen
            desktop->Msg_LeftDown(mc);

            // und allen unten drunter auch Bescheid sagen
            desktop->RelayMouseMessage(&Window::Msg_LeftDown, mc);;
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  Verarbeitung des Loslassens der Linken Maustaste.
 *
 *  @param[in] mc Mauskoordinaten Struktur
 *
 *  @author OLiver
 */
void WindowManager::Msg_LeftUp(const MouseCoords& mc)
{
    // ist unser Desktop gültig?
    if(!desktop)
        return;

    // ist der Maus-Klick-Fix aktiv?
    if(!disable_mouse)
    {
        // ist der Desktop aktiv?
        if(desktop->GetActive())
        {
            // ja, dann Msg_LeftUp aufrufen
            desktop->Msg_LeftUp(mc);

            // und die Fenster darunter auch
            desktop->RelayMouseMessage(&Window::Msg_LeftUp, mc);
        }
        else if(windows.back())
        {
            // ja, dann Msg_LeftUp aufrufen
            windows.back()->Msg_LeftUp(mc);

            // und den anderen Fenstern auch Bescheid geben
            windows.back()->RelayMouseMessage(&Window::Msg_LeftUp, mc);

            // und noch MouseLeftUp vom Fenster aufrufen
            windows.back()->MouseLeftUp(mc);
        }
    }

    // Maus-Klick-Fix deaktivieren
    disable_mouse = false;
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  Verarbeitung des Drückens der Rechten Maustaste.
 *
 *  @param[in] mc Mauskoordinaten Struktur
 *
 *  @author OLiver
 */
void WindowManager::Msg_RightDown(const MouseCoords& mc)
{
    // ist unser Desktop gültig?
    if(!desktop)
        return;

    // Sind Fenster vorhanden && ist das aktive Fenster ok
    if(!windows.empty())
    {
        // ist das Fenster modal? wenn ja, dann raus
        if (windows.back()->GetModal())
            return;

        // prüfen ob Fenster geschlossen werden muss
        IngameWindow* foundWindow = FindWindowUnderMouse(mc);
        if(foundWindow){
            // ja, dann schliessen
            if (foundWindow->GetCloseOnRightClick())
                foundWindow->Close();
            else
            {
                windows.back()->SetActive(false);

                // Fenster aus der Liste holen und vorne wieder anhängen
                windows.remove(foundWindow);
                windows.push_back(foundWindow);

                desktop->SetActive(false);

                foundWindow->SetActive(true);
                foundWindow->Msg_RightDown(mc);
            }

            return;
        }
    }

    // ist der Desktop aktiv?
    if(desktop->GetActive())
    {
        // ja, dann Msg_RightDown aufrufen
        desktop->Msg_RightDown(mc);

        // und die Fenster darunter auch
        desktop->RelayMouseMessage(&Window::Msg_RightDown, mc);
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
    desktop->SetActive(true);

    // ja, dann Msg_RightDown aufrufen
    desktop->Msg_RightDown(mc);

    // und die Fenster darunter auch
    desktop->RelayMouseMessage(&Window::Msg_RightDown, mc);
}

void WindowManager::Msg_RightUp(const MouseCoords& mc)
{
    RelayMouseMessage(&Window::Msg_RightUp, mc);
}

///////////////////////////////////////////////////////////////////////
/*
 *  Verarbeitung Mausrad hoch.
 *
 *  @param[in] mc Mauskoordinaten Struktur
 *
 *  @author Divan
 */
void WindowManager::Msg_WheelUp(const MouseCoords& mc)
{
    // ist unser Desktop gültig?
    if(!desktop)
        return;

    // haben wir überhaupt fenster?
    if(windows.empty())
    {
        // nein, dann Desktop aktivieren
        desktop->SetActive(true);

        // nein, Msg_LeftDown aufrufen
        desktop->Msg_WheelUp(mc);

        // und allen unten drunter auch Bescheid sagen
        desktop->RelayMouseMessage(&Window::Msg_WheelUp, mc);

        // und raus
        return;
    }

    // ist das zuletzt aktiv gewesene Fenster Modal?
    if(windows.back()->GetModal())
    {
        // Msg_LeftDownaufrufen
        windows.back()->Msg_WheelUp(mc);

        // und allen unten drunter auch Bescheid sagen
        windows.back()->RelayMouseMessage(&Window::Msg_WheelUp, mc);

        // und raus
        return;
    }

    IngameWindow* foundWindow = FindWindowUnderMouse(mc);
    // ja, also aktives Fenster deaktivieren (falls ok)
    if(windows.back())
        windows.back()->SetActive(false);

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
        desktop->SetActive(false);
    }else {
        // Desktop aktivieren
        desktop->SetActive(true);

        // nein, dann Msg_WheelUpDown aufrufen
        desktop->Msg_WheelUp(mc);

        // und allen unten drunter auch Bescheid sagen
        desktop->RelayMouseMessage(&Window::Msg_WheelUp, mc);;
    }
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  Verarbeitung Mausrad runter
 *
 *  @param[in] mc Mauskoordinaten Struktur
 *
 *  @author Divan
 */
void WindowManager::Msg_WheelDown(const MouseCoords& mc)
{
    if(!desktop)
        return;
    if(windows.empty())
    {
        desktop->SetActive(true);
        desktop->Msg_WheelDown(mc);
        desktop->RelayMouseMessage(&Window::Msg_WheelDown, mc);
        // und raus
        return;
    }
    if(windows.back()->GetModal())
    {
        windows.back()->Msg_WheelDown(mc);
        windows.back()->RelayMouseMessage(&Window::Msg_WheelDown, mc);
        return;
    }
    IngameWindow* foundWindow = FindWindowUnderMouse(mc);
    windows.back()->SetActive(false);

    if(foundWindow)
    {
        windows.remove(foundWindow);
        windows.push_back(foundWindow);
        foundWindow->SetActive(true);
        foundWindow->Msg_WheelDown(mc);
        foundWindow->RelayMouseMessage(&Window::Msg_WheelDown, mc);
        desktop->SetActive(false);
    }else{
        desktop->SetActive(true);
        desktop->Msg_WheelDown(mc);
        desktop->RelayMouseMessage(&Window::Msg_WheelDown, mc);;
    }
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  Verarbeitung des Verschiebens der Maus.
 *
 *  @param[in] mc Mauskoordinaten Struktur
 *
 *  @author OLiver
 */
void WindowManager::Msg_MouseMove(const MouseCoords& mc)
{
    this->mc = &mc;

    // ist unser Desktop gültig?
    if(!desktop)
        return;

    // ist der Maus-Klick-Fix aktiv?
    if(!disable_mouse)
    {
        // nein, ist unser Desktop aktiv?
        if(desktop->GetActive())
        {
            // ja, dann Msg_MouseMove aufrufen
            desktop->Msg_MouseMove(mc);

            // und alles drunter auch benachrichtigen
            desktop->RelayMouseMessage(&Window::Msg_MouseMove, mc);
        }
        else if(!windows.empty())
        {
            // und MouseMove vom Fenster aufrufen
            windows.back()->MouseMove(mc);

            // ja, dann Msg_MouseMove aufrufen
            windows.back()->Msg_MouseMove(mc);

            // und alles drunter auch benachrichtigen
            windows.back()->RelayMouseMessage(&Window::Msg_MouseMove, mc);
        }
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

///////////////////////////////////////////////////////////////////////////////
/**
 *  Verarbeitung Spielfenstergröße verändert (vom Betriebssystem aus)
 *  Liefert evtl. eine Größe die wir nicht wollen und daher korrigieren
 *  oder falls ok durchlassen.
 *  Eigentliche Verarbeitung dann in Msg_ScreenResize.
 *
 *  @param[in] width  neue Breite
 *  @param[in] height neue Höhe
 *
 *  @author Divan
 */
void WindowManager::ScreenResized(unsigned short width, unsigned short height)
{
    unsigned short newWidth  = width;
    unsigned short newHeight = height;

//  bool mustResize = false;
    // Minimale Ausdehnung erfällt?
//  if(newWidth  < 800 || newHeight < 600)
//  {
//      mustResize = true;
//      if(newWidth  < 800) newWidth  = 800;
//      if(newHeight < 600) newHeight = 600;
//  }

    // Es kann passieren dass wir versuchen ein 800x600-Fenster zu erstellen,
    // aber das Betriebssystem es immer wieder verkleinert. Hier sollten wir
    // uns nicht auf einen endlosen Kampf einlassen, denn der Klägere gibt nach.
    // Problem: Böse Windowmanager wie Metacity feuern ständig Resize-Events,
    // wenn man den die Fenstergröße zieht und wir sie wieder zu vergrößern
    // versuchen.
    // Wir müssen also durch Warten halbwegs ausschließen, dass wir es
    // mit dem Nutzer zu tun haben.
    // TODO: Dann den Treiber zwingen, ein nicht resizable Fenster zu öffnen
    // und erst wieder, wenn der Nutzer im Menü nochmal eine Auflösung einstellt,
    // das Resizen zulassen.
//  if(width < 800 || height < 600)
//  if(lastScreenWidthSignal  == width)
//  if(lastScreenHeightSignal == height)
//  if(lastScreenSignalCount == 500)
//  {
//      VIDEODRIVER.ResizeScreen(width, height, VIDEODRIVER.IsFullscreen());
//      return;
//  }

    // Letzten Wert merken
//  if(lastScreenWidthSignal == width
//     && lastScreenHeightSignal == height)
//  {
//      ++lastScreenSignalCount;
//  }
//  else
//  {
//      lastScreenWidthSignal  = width;
//      lastScreenHeightSignal = height;
//      lastScreenSignalCount  = 0;
//  }

    // Und los
    VIDEODRIVER.ResizeScreen(newWidth, newHeight, VIDEODRIVER.IsFullscreen());
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  Verarbeitung Spielfenstergröße verändert (vom Spiel aus)
 *  Liefert immer eine sinnvolle Größe, mind. 800x600.
 *
 *  @param[in] width  neue Breite
 *  @param[in] height neue Höhe
 *
 *  @author Divan
 */
void WindowManager::Msg_ScreenResize(unsigned short width, unsigned short height)
{
    // Falls sich nichts ändert, brauchen wir auch nicht reagieren
    // (Evtl hat sich ja nur der Modus Fenster/Vollbild geändert)
    if(screenWidth == width && screenHeight == height)
        return;

    ScreenResizeEvent sr;
    sr.oldWidth  = screenWidth;
    sr.oldHeight = screenHeight;
    sr.newWidth  = screenWidth  = (width < 800 ? 800 : width);
    sr.newHeight = screenHeight = (height < 600 ? 600 : height);

    SETTINGS.video.fullscreen = VIDEODRIVER.IsFullscreen();
    // Wenn es absolut nicht anders geht, lassen wir im temporï¿½r doch
    // kleiner als 800x600 zu, abspeichern tun wir die aber nie.
    if(!SETTINGS.video.fullscreen)
    {
        if(width  >= 800) SETTINGS.video.windowed_width  = width;
        if(height >= 600) SETTINGS.video.windowed_height = height;
    }

    // ist unser Desktop gültig?
    if(!desktop)
        return;

    desktop->Msg_ScreenResize(sr);

    // IngameWindow verschieben falls nötig, so dass sie komplett sichtbar sind
    for(IgwListIterator it = windows.begin(); it != windows.end(); ++it)
    {
        const short dx = (*it)->GetX() + (*it)->GetWidth()  - sr.newWidth;
        const short dy = (*it)->GetY() + (*it)->GetHeight() - sr.newHeight;
        if(dx > 0 || dy > 0)
            (*it)->Move((dx > 0 ? -dx : 0), (dy > 0 ? -dy : 0) , /*absolute=*/false);
    }
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  schliesst ein IngameWindow und entfernt es aus der Fensterliste.
 *
 *  @param[in] it Iterator auf das Fenster in der Fensterliste
 *
 *  @author OLiver
 */
void WindowManager::Close(IngameWindow* window)
{
    // ist das Fenster gültig?
    if(!window)
        return;

    IgwListIterator it = std::find(windows.begin(), windows.end(), window);
    if( it == windows.end())
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
            desktop->SetActive(true);
        }
    }

    // Fenster löschen
    delete window;
    // und aus der Liste entfernen
    windows.erase(it);
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  Sucht ein Fenster mit der entsprechenden Fenster-ID
 *  und schließt es (falls es so eins gibt)
 *
 *  @param[in] id ID des/der Fenster(s) welche(s) geschlossen werden soll
 *
 *  @author OLiver
 */
void WindowManager::Close(unsigned int id)
{
    for(IgwListIterator it = windows.begin(); it != windows.end(); ++it)
    {
        if((*it)->id == id){
            Close(*it);
            return;
        }
    }
}


///////////////////////////////////////////////////////////////////////////////
/**
 *  wechselt den Desktop in den neuen Desktop
 *
 *  @author OLiver
 */
void WindowManager::Switch(void)
{
    // einmal richtig clearen
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    SetToolTip(NULL, "");

    // haben wir einen aktuell gültigen Desktop?
    if(desktop)
    {
        // Alle (alten) Fenster zumachen
        for(IgwListIterator it = windows.begin(); it != windows.end(); ++it)
            delete (*it);
        windows.clear();

        // Desktop löschen
        delete desktop;
    }

    // Desktop auf Neuen umstellen
    desktop = nextdesktop;

    // ist der neue Desktop gültig?
    if(desktop)
    {
        // Desktop aktivieren
        desktop->SetActive(true);

        // aufräumen
        nextdesktop = NULL;
        nextdesktop_data = NULL;
    }
}

void WindowManager::SetToolTip(Window* ttw, const std::string& tooltip)
{
    static Window* lttw = NULL;

    if(tooltip.length() == 0 && !ttw)
        this->tooltip = "";

    if(tooltip.length() == 0 && lttw == ttw)
        this->tooltip = "";
    if(tooltip.length())
    {
        this->tooltip = tooltip;
        lttw = ttw;
    }
}

void WindowManager::DrawToolTip()
{
    // Tooltip zeichnen
    if(tooltip.length() && mc)
    {
        unsigned text_width = NormalFont->getWidth(tooltip);
        unsigned right_edge = mc->x + 30 + text_width + 2;
        unsigned x = mc->x + 30;

        // links neben der Maus, wenn es über den Rand gehen würde
        if(right_edge > VIDEODRIVER.GetScreenWidth() )
            x = mc->x - 30 - text_width;

        unsigned int count = 0;
        size_t pos = 0;
        do
        {
            count++;
            if(pos != 0)
                pos++;
            pos = tooltip.find('\n', pos);
        }
        while( pos != std::string::npos && (pos < tooltip.length() - 2));

        Window::DrawRectangle(x - 2 , mc->y - 2, text_width + 4, 4 + count * NormalFont->getDy(), 0x9F000000);
        NormalFont->Draw(x, mc->y , tooltip, glArchivItem_Font::DF_TOP, COLOR_YELLOW);

    }
}
