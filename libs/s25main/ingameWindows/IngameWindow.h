// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "DrawPoint.h"
#include "SnapOffset.h"
#include "Window.h"
#include "helpers/EnumArray.h"
#include "gameData/const_gui_ids.h"
#include <array>
#include <vector>

class glArchivItem_Bitmap;
struct MouseCoords;
struct PersistentWindowSettings;
template<typename T>
struct Point;

enum CloseBehavior
{
    /// Closeable via right-click, button, keyboard (ESC, ALT+W)
    Regular,
    /// Close behavior is managed by window, e.g. explicit button
    Custom,
    /// Same as Regular, but doesn't (auto-)close on right-click
    NoRightClick,
};

enum class IwButton
{
    Close,
    Title, /// Pseudo-button to respond to double-clicks on the title bar
    PinOrMinimize
};
constexpr auto maxEnumValue(IwButton)
{
    return IwButton::PinOrMinimize;
}

class IngameWindow : public Window
{
public:
    /// Special position that gets translated to the last know position or screen center when passed to the ctor
    static const DrawPoint posLastOrCenter;
    /// Special position that gets translated to the screen center when passed to the ctor
    static const DrawPoint posCenter;
    /// Special position that gets translated to the mouse position when passed to the ctor
    static const DrawPoint posAtMouse;

    static const Extent borderSize;

    IngameWindow(unsigned id, const DrawPoint& pos, const Extent& size, std::string title,
                 glArchivItem_Bitmap* background, bool modal = false,
                 CloseBehavior closeBehavior = CloseBehavior::Regular, Window* parent = nullptr);

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
    /// Get the full size of the window, even when minimized
    Extent GetFullSize() const;
    /// Get the current lower right corner of the content area
    DrawPoint GetRightBottomBoundary();

    /// Set the position for the window after adjusting newPos so the window is in the visible area
    void SetPos(DrawPoint newPos, bool saveRestorePos = true);

    /// merkt das Fenster zum Schließen vor.
    virtual void Close();
    /// soll das Fenster geschlossen werden.
    bool ShouldBeClosed() const { return closeme; }

    /// minimiert das Fenster.
    void SetMinimized(bool minimized = true);
    /// ist das Fenster minimiert?
    bool IsMinimized() const { return isMinimized_; }

    void SetPinned(bool pinned = true);
    bool IsPinned() const { return isPinned_; }

    CloseBehavior getCloseBehavior() const { return closeBehavior_; }

    /// Modal windows cannot be minimized, are always active and stay on top of non-modal ones
    bool IsModal() const { return isModal_; }

    void MouseLeftDown(const MouseCoords& mc);
    void MouseLeftUp(const MouseCoords& mc);
    void MouseMove(const MouseCoords& mc);

    GUI_ID GetGUIID() const { return static_cast<GUI_ID>(Window::GetID()); }

protected:
    void Draw_() final;
    /// Called when not minimized before drawing the frame
    virtual void DrawBackground();
    /// Called when not minimized after the frame and background have been drawn
    virtual void DrawContent() {}

    /// Verschiebt Fenster in die Bildschirmmitte
    void MoveToCenter();
    /// Verschiebt Fenster neben die Maus
    void MoveNextToMouse();

    /// Weiterleitung von Nachrichten erlaubt oder nicht?
    bool IsMessageRelayAllowed() const override;

    void SaveOpenStatus(bool isOpen) const;

    unsigned short iwHeight;
    std::string title_;
    glArchivItem_Bitmap* background;
    DrawPoint lastMousePos;

    /// Offset from left and top to actual content
    Extent contentOffset;
    /// Offset from content to right and bottom boundary
    Extent contentOffsetEnd;

private:
    /// Get bounds of given button
    Rect GetButtonBounds(IwButton btn) const;

    bool isModal_;
    bool closeme;
    bool isPinned_;
    bool isMinimized_;
    bool isMoving;
    SnapOffset snapOffset_;
    CloseBehavior closeBehavior_;
    helpers::EnumArray<ButtonState, IwButton> buttonStates_;
    PersistentWindowSettings* windowSettings_;
    DrawPoint restorePos_;
};
