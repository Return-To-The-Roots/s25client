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
#ifndef WINDOWMANAGER_H_INCLUDED
#define WINDOWMANAGER_H_INCLUDED

#pragma once

#include "Point.h"
#include "driver/VideoDriverLoaderInterface.h"
#include "helpers/Deleter.h"
#include "libutil/Singleton.h"
#include <boost/interprocess/smart_ptr/unique_ptr.hpp>
#include <list>
#include <string>
#include <vector>

class Window;
class Desktop;
class IngameWindow;
class MouseCoords;
struct KeyEvent;
class ctrlBaseTooltip;

/// Verwaltet alle (offenen) Fenster bzw Desktops samt ihren Controls und Messages
class WindowManager : public Singleton<WindowManager>, public VideoDriverLoaderInterface
{
public:
    typedef bool (Window::*KeyboardMsgHandler)(const KeyEvent&);
    typedef bool (Window::*MouseMsgHandler)(const MouseCoords&);

    WindowManager();
    ~WindowManager() override;
    void CleanUp();

    /// Zeichnet Desktop und alle Fenster.
    void Draw();
    /// Set the current frames per second
    void UpdateFps(unsigned newFps);
    /// liefert ob der aktuelle Desktop den Focus besitzt oder nicht.
    bool IsDesktopActive();

    /// schickt eine Nachricht an das aktive Fenster bzw den aktiven Desktop.
    /// Sendet eine Tastaturnachricht an die Steuerelemente.
    void RelayKeyboardMessage(KeyboardMsgHandler msg, const KeyEvent& ke);
    /// Sendet eine Mausnachricht weiter an alle Steuerelemente
    void RelayMouseMessage(MouseMsgHandler msg, const MouseCoords& mc);

    /// Öffnet ein IngameWindow und fügt es zur Fensterliste hinzu.
    void Show(IngameWindow* window, bool mouse = false);
    /// Registers a window to be shown after a desktop switch
    void ShowAfterSwitch(IngameWindow* window);
    /// schliesst ein IngameWindow und entfernt es aus der Fensterliste.
    void Close(const IngameWindow* window);
    /// Sucht ein Fenster mit der entsprechenden Fenster-ID und schließt es (falls es so eins gibt)
    void Close(unsigned id);
    /// merkt einen Desktop zum Wechsel vor.
    void Switch(Desktop* desktop);
    /// Verarbeitung des Drückens der Linken Maustaste.
    void Msg_LeftDown(MouseCoords mc) override;
    /// Verarbeitung des Loslassens der Linken Maustaste.
    void Msg_LeftUp(MouseCoords mc) override;
    /// Verarbeitung des Drückens der Rechten Maustaste.
    void Msg_RightUp(const MouseCoords& mc) override;
    /// Verarbeitung des Loslassens der Rechten Maustaste.
    void Msg_RightDown(const MouseCoords& mc) override;
    /// Verarbeitung des Drückens des Rad hoch.
    void Msg_WheelUp(const MouseCoords& mc) override;
    /// Verarbeitung Rad runter.
    void Msg_WheelDown(const MouseCoords& mc) override;
    /// Verarbeitung des Verschiebens der Maus.
    void Msg_MouseMove(const MouseCoords& mc) override;
    /// Verarbeitung Keyboard-Event
    void Msg_KeyDown(const KeyEvent& ke) override;
    // setzt den Tooltip
    void SetToolTip(const ctrlBaseTooltip* ttw, const std::string& tooltip);

    /// Verarbeitung Spielfenstergröße verändert (vom Betriebssystem aus)
    void ScreenResized(unsigned short width, unsigned short height) override;
    /// Verarbeitung Spielfenstergröße verändert (vom Spiel aus)
    // Achtung: nicht dieselbe Nachricht, die die Window-Klasse empfängt
    void Msg_ScreenResize(const Extent& newSize);

    /// Return the window currently on the top (probably active)
    const Window* GetTopMostWindow() const;
    IngameWindow* FindWindowAtPos(const Position& pos) const;

protected:
    void DrawToolTip();

private:
    typedef std::list<IngameWindow*> IgwList;                   /// Fensterlistentyp
    typedef std::list<IngameWindow*>::iterator IgwListIterator; /// Fensterlistentypiterator

    void TakeScreenshot();
    /// wechselt einen Desktop
    void DoDesktopSwitch();
    /// Actually close all ingame windows marked for closing
    void CloseMarkedIngameWnds();

    boost::interprocess::unique_ptr<Desktop, Deleter<Desktop> > curDesktop;  /// aktueller Desktop
    boost::interprocess::unique_ptr<Desktop, Deleter<Desktop> > nextdesktop; /// der nächste Desktop
    bool disable_mouse; /// Mausdeaktivator, zum beheben des "Switch-Anschließend-Drück-Bug"s

    IgwList windows; /// Fensterliste
    /// Windows that will be shown after desktop switch
    /// Otherwise the window will not be shown, if it was added after a switch request
    std::vector<IngameWindow*> nextWnds;
    Position lastMousePos;
    std::string curTooltip;
    Extent screenSize; /// last valid screen size

    // Für Doppelklick merken:
    unsigned lastLeftClickTime; /// Zeit des letzten Links-Klicks
    Position lastLeftClickPos;  /// Position beim letzten Links-Klick
};

#define WINDOWMANAGER WindowManager::inst()

#endif // !WINDOWMANAGER_H_INCLUDED
