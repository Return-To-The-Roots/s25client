// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

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

    IngameWindow(unsigned id, const DrawPoint& pos, const Extent& size, std::string title,
                 glArchivItem_Bitmap* background, bool modal = false, bool closeOnRightClick = true,
                 Window* parent = nullptr);
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

    /// Set the position for the window after adjusting newPos so the window is in the visible area
    void SetPos(DrawPoint newPos);

    /// merkt das Fenster zum Schließen vor.
    virtual void Close();
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
