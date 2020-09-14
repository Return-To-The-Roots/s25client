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

#pragma once

#include "Point.h"
#include "driver/VideoDriverLoaderInterface.h"
#include "s25util/Singleton.h"
#include <list>
#include <memory>
#include <string>
#include <vector>

class Window;
class Desktop;
class IngameWindow;
class MouseCoords;
struct KeyEvent;
class ctrlBaseTooltip;

// Cursor types with values equal to indices in resource.idx
enum class Cursor : unsigned
{
    None,
    Hand = 30,
    Scroll = 32,
    Moon = 33,
    Remove = 34
};

/// Verwaltet alle (offenen) Fenster bzw Desktops samt ihren Controls und Messages
class WindowManager : public Singleton<WindowManager>, public VideoDriverLoaderInterface
{
public:
    using KeyboardMsgHandler = bool (Window::*)(const KeyEvent&);
    using MouseMsgHandler = bool (Window::*)(const MouseCoords&);

    WindowManager();
    ~WindowManager();
    void CleanUp();

    /// Zeichnet Desktop und alle Fenster.
    void Draw();
    /// liefert ob der aktuelle Desktop den Focus besitzt oder nicht.
    bool IsDesktopActive();

    /// schickt eine Nachricht an das aktive Fenster bzw den aktiven Desktop.
    /// Sendet eine Tastaturnachricht an die Steuerelemente.
    void RelayKeyboardMessage(KeyboardMsgHandler msg, const KeyEvent& ke);
    /// Sendet eine Mausnachricht weiter an alle Steuerelemente
    void RelayMouseMessage(MouseMsgHandler msg, const MouseCoords& mc);

    /// Öffnet ein IngameWindow und fügt es zur Fensterliste hinzu.
    IngameWindow& DoShow(std::unique_ptr<IngameWindow> window, bool mouse = false);
    template<typename T>
    T& Show(std::unique_ptr<T> window, bool mouse = false)
    {
        return static_cast<T&>(DoShow(std::move(window), mouse));
    }
    template<typename T>
    T& ReplaceWindow(std::unique_ptr<T> window)
    {
        auto* oldWnd = FindNonModalWindow(window->GetID());
        if(oldWnd)
            oldWnd->Close();
        return Show(std::move(window));
    }
    template<typename T>
    T* ToggleWindow(std::unique_ptr<T> window)
    {
        auto* oldWnd = FindNonModalWindow(window->GetID());
        if(oldWnd)
        {
            oldWnd->Close();
            return nullptr;
        } else
            return &Show(std::move(window));
    }
    /// Registers a window to be shown after a desktop switch
    IngameWindow* ShowAfterSwitch(std::unique_ptr<IngameWindow> window);
    /// schliesst ein IngameWindow und entfernt es aus der Fensterliste.
    void Close(const IngameWindow* window);
    /// Sucht ein Fenster mit der entsprechenden Fenster-ID und schließt es (falls es so eins gibt)
    void Close(unsigned id);
    /// merkt einen Desktop zum Wechsel vor.
    Desktop* Switch(std::unique_ptr<Desktop> desktop);
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
    void WindowResized() override;
    /// Verarbeitung Spielfenstergröße verändert (vom Spiel aus)
    // Achtung: nicht dieselbe Nachricht, die die Window-Klasse empfängt
    void Msg_ScreenResize(const Extent& newSize);

    /// Return the window currently on the top (probably active)
    const IngameWindow* GetTopMostWindow() const;
    IngameWindow* FindWindowAtPos(const Position& pos) const;
    IngameWindow* FindNonModalWindow(unsigned id) const;

    Desktop* GetCurrentDesktop() { return curDesktop.get(); }
    /// Makes the given window (desktop or ingame window) active and all others inactive
    void SetActiveWindow(Window&);

    void SetCursor(Cursor cursor = Cursor::Hand);
    Cursor GetCursor() const { return cursor_; }

private:
    class Tooltip;

    void DrawCursor();
    void DrawToolTip();

    void TakeScreenshot();
    /// wechselt einen Desktop
    void DoDesktopSwitch();
    /// Actually close all ingame windows marked for closing
    void CloseMarkedIngameWnds();

    Cursor cursor_;
    std::unique_ptr<Desktop> curDesktop;  /// aktueller Desktop
    std::unique_ptr<Desktop> nextdesktop; /// der nächste Desktop
    bool disable_mouse;                   /// Mausdeaktivator, zum beheben des "Switch-Anschließend-Drück-Bug"s

    std::list<std::unique_ptr<IngameWindow>> windows; /// Fensterliste
    /// Windows that will be shown after desktop switch
    /// Otherwise the window will not be shown, if it was added after a switch request
    std::vector<std::unique_ptr<IngameWindow>> nextWnds;
    Position lastMousePos;
    std::unique_ptr<Tooltip> curTooltip;
    Extent curRenderSize; /// current render size

    // Für Doppelklick merken:
    unsigned lastLeftClickTime; /// Zeit des letzten Links-Klicks
    Position lastLeftClickPos;  /// Position beim letzten Links-Klick
};

#define WINDOWMANAGER WindowManager::inst()
