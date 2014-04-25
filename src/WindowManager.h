// $Id: WindowManager.h 9357 2014-04-25 15:35:25Z FloSoft $
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
#ifndef WINDOWMANAGER_H_INCLUDED
#define WINDOWMANAGER_H_INCLUDED

#pragma once

#include "Singleton.h"
#include "MouseAndKeys.h"
#include "VideoDriverLoaderInterface.h"

class Window;
class Desktop;
class IngameWindow;

/// Verwaltet alle (offenen) Fenster bzw Desktops samt ihren Controls und Messages
class WindowManager : public Singleton<WindowManager>, public VideoDriverLoaderInterface
{
        typedef list<IngameWindow*> IngameWindowList;                   ///< Fensterlistentyp
        typedef list<IngameWindow*>::iterator IngameWindowListIterator; ///< Fensterlistentypiterator

    public:
        /// Konstruktor von @p WindowManager.
        WindowManager(void);
        /// Destruktor von @p WindowManager.
        ~WindowManager(void);
        void CleanUp();

        /// Zeichnet Desktop und alle Fenster.
        void Draw(void);
        /// liefert ob der aktuelle Desktop den Focus besitzt oder nicht.
        bool IsDesktopActive(void);

        /// schickt eine Nachricht an das aktive Fenster bzw den aktiven Desktop.
        /// Sendet eine Tastaturnachricht an die Steuerelemente.
        void RelayKeyboardMessage(bool (Window::*msg)(const KeyEvent&), const KeyEvent& ke);
        /// Sendet eine Mausnachricht weiter an alle Steuerelemente
        void RelayMouseMessage(bool (Window::*msg)(const MouseCoords&), const MouseCoords& mc);

        /// öffnet ein IngameWindow und fügt es zur Fensterliste hinzu.
        void Show(IngameWindow* window, bool mouse = false);
        /// schliesst ein IngameWindow und entfernt es aus der Fensterliste.
        void Close(IngameWindow* window);
        /// Sucht ein Fenster mit der entsprechenden Fenster-ID und schließt es (falls es so eins gibt)
        void Close(unsigned int id);
        /// merkt einen Desktop zum Wechsel vor.
        void Switch(Desktop* desktop, void* data = NULL, bool mouse = false);
        /// Verarbeitung des Drückens der Linken Maustaste.
        void Msg_LeftDown(MouseCoords mc);
        /// Verarbeitung des Loslassens der Linken Maustaste.
        void Msg_LeftUp(const MouseCoords& mc);
        /// Verarbeitung des Drückens der Rechten Maustaste.
        void Msg_RightUp(const MouseCoords& mc);
        /// Verarbeitung des Loslassens der Rechten Maustaste.
        void Msg_RightDown(const MouseCoords& mc);
        /// Verarbeitung des Drückens des Rad hoch.
        void Msg_WheelUp(const MouseCoords& mc);
        /// Verarbeitung Rad runter.
        void Msg_WheelDown(const MouseCoords& mc);
        /// Verarbeitung des Verschiebens der Maus.
        void Msg_MouseMove(const MouseCoords& mc);
        /// Verarbeitung Keyboard-Event
        void Msg_KeyDown(const KeyEvent& ke);
        // setzt den Tooltip
        void SetToolTip(Window* ttw, const std::string& tooltip);

        /// Verarbeitung Spielfenstergröße verändert (vom Betriebssystem aus)
        void ScreenResized(unsigned short width, unsigned short height);
        /// Verarbeitung Spielfenstergröße verändert (vom Spiel aus)
        // Achtung: nicht dieselbe Nachricht, die die Window-Klasse empfängt
        void Msg_ScreenResize(unsigned short width, unsigned short height);

    protected:
        void DrawToolTip();

    private:
        /// schliesst ein IngameWindow und entfernt es aus der Fensterliste.
        void Close(IngameWindowListIterator& it);
        /// wechselt einen Desktop
        void Switch(void);

    private:
        Desktop* desktop;        ///< aktueller Desktop
        Desktop* nextdesktop;    ///< der nächste Desktop
        void* nextdesktop_data;  ///< Daten für den nächsten Desktop, welche dann MSG_SWITCH übergeben werden
        bool disable_mouse;      ///< Mausdeaktivator, zum beheben des "Switch-Anschließend-Drück-Bug"s

        IngameWindowList windows; ///< Fensterliste
        const MouseCoords* mc;
        std::string tooltip;
        unsigned short screenWidth;  /// letzte gültige Bildschirm-/Fensterbreite
        unsigned short screenHeight; /// letzte gültige Bildschirm-/Fensterhöhe

        // Für Doppelklick merken:
        unsigned last_left_click_time; /// Zeit des letzten Links-Klicks
        Point<int> last_left_click_point; /// Position beim letzten Links-Klick

        // um Schleifen abzufangen, die entstehen, weil wir mindestens 800x600 haben wollen.
//  unsigned short lastScreenWidthSignal;
//  unsigned short lastScreenHeightSignal;
//  unsigned short lastScreenSignalCount;
};

#endif // !WINDOWMANAGER_H_INCLUDED
