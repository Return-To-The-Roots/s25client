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

#include "Window.h"
#include <array>
#include <vector>

class glArchivItem_Bitmap;
class MouseCoords;
template<typename T>
struct Point;

class IngameWindow : public Window
{
    /// For each id we save the last position of the window
    static std::vector<DrawPoint> last_pos;

public:
    /// Special position that gets translated to the last know position or screen center when passed to the ctor
    static const DrawPoint posLastOrCenter;
    /// Special position that gets translated to the screen center when passed to the ctor
    static const DrawPoint posCenter;
    /// Special position that gets translated to the mouse position when passed to the ctor
    static const DrawPoint posAtMouse;

    static const Extent borderSize;

    IngameWindow(unsigned id, const DrawPoint& pos, const Extent& size, std::string title, glArchivItem_Bitmap* background,
                 bool modal = false, bool closeOnRightClick = true, Window* parent = nullptr);
    ~IngameWindow() override;

    /// setzt den Hintergrund.
    void SetBackground(glArchivItem_Bitmap* background) { this->background = background; }
    /// liefert den Hintergrund.
    glArchivItem_Bitmap* GetBackground() const { return background; }

    /// setzt den Fenstertitel.
    void SetTitle(const std::string& title) { this->title_ = title; }
    /// liefert den Fenstertitel.
    const std::string& GetTitle() const { return title_; }

    void Resize(const Extent& newSize) override;
    /// Set the size of the (expanded) content area
    void SetIwSize(const Extent& newSize);
    /// Get the size of the (expanded) content area
    Extent GetIwSize() const;
    /// Get the current lower right corner of the content area
    DrawPoint GetRightBottomBoundary();

    /// merkt das Fenster zum Schließen vor.
    void Close();
    /// soll das Fenster geschlossen werden.
    bool ShouldBeClosed() const { return closeme; }

    /// minimiert das Fenster.
    void SetMinimized(bool minimized = true);
    /// ist das Fenster minimiert?
    bool IsMinimized() const { return isMinimized_; }

    /// Can we close the window on right-click?
    /// If this is false and the window is modal the close button at the title bar will be hidden
    bool GetCloseOnRightClick() const { return closeOnRightClick_; }

    /// ist das Fenster ein modales Fenster?
    bool IsModal() const { return isModal_; }

    void MouseLeftDown(const MouseCoords& mc);
    void MouseLeftUp(const MouseCoords& mc);
    void MouseMove(const MouseCoords& mc);

protected:
    void Draw_() override;

    /// Verschiebt Fenster in die Bildschirmmitte
    void MoveToCenter();
    /// Verschiebt Fenster neben die Maus
    void MoveNextToMouse();

    /// Weiterleitung von Nachrichten erlaubt oder nicht?
    bool IsMessageRelayAllowed() const override;

protected:
    unsigned short iwHeight;
    std::string title_;
    glArchivItem_Bitmap* background;
    DrawPoint lastMousePos;
    bool last_down;
    bool last_down2;
    std::array<ButtonState, 2> button_state;

    /// Offset from left and top to actual content
    Extent contentOffset;
    /// Offset from content to right and bottom boundary
    Extent contentOffsetEnd;

    Rect GetLeftButtonRect() const;
    Rect GetRightButtonRect() const;

private:
    bool isModal_;
    bool closeme;
    bool isMinimized_;
    bool isMoving;
    bool closeOnRightClick_;
};
