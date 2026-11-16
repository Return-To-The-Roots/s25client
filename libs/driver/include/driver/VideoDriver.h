// Copyright (C) 2005 - 2026 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "MouseCoords.h"
#include "VideoInterface.h"
#include <array>

class VideoDriverLoaderInterface;

/// Basisklasse für einen Videotreiber.
class VideoDriver : public IVideoDriver
{
public:
    VideoDriver(VideoDriverLoaderInterface* CallBack);

    ~VideoDriver() override = default;

    /// Funktion zum Auslesen der Mauskoordinaten.
    Position getMousePos() const override;

    /// Funktion zum Auslesen ob die Linke Maustaste gedrückt ist.
    bool getMouseStateL() const override;

    /// Funktion zum Auslesen ob die Rechte Maustaste gedrückt ist.
    bool getMouseStateR() const override;

    /// Function to check if at least 1 finger is on screen
    bool isTouchEvent() const override;

    VideoMode getWindowSize() const override final { return windowSize_; }
    Extent getRenderSize() const override final { return scaledRenderSize_; }
    DisplayMode getDisplayMode() const override final { return displayMode_; }

    float getDpiScale() const override final { return dpiScale_; }

    const GuiScale& getGuiScale() const override final { return guiScale_; }
    void setGuiScalePercent(unsigned percent) override;

    GuiScaleRange getGuiScaleRange() const override;

    /// prüft auf Initialisierung.
    bool isInitialized() const override final { return initialized; }
    bool isOpenGL() const override { return true; }

protected:
    VideoMode findClosestVideoMode(VideoMode mode) const;
    void setNewSize(VideoMode windowSize, Extent renderSize);

    VideoDriverLoaderInterface* CallBack; /// DriverCallback to notify on player input
    bool initialized;
    MouseCoords mouse_xy;           /// Mouse state
    std::array<bool, 512> keyboard; /// Keyboard state
    DisplayMode displayMode_;       /// Fullscreen/resizable?

private:
    // cached as possibly used often
    VideoMode windowSize_;    ///< Size of the window or fullscreen resolution
    Extent renderSize_;       ///< Size of the renderable surface
    Extent scaledRenderSize_; ///< Size of the projection clipping area

    float dpiScale_;    ///< Scale factor required to convert "normal" DPI to the display DPI
    GuiScale guiScale_; ///< Scale factor applied to the user interface
    bool autoGuiScale_; ///< Whether the recommended GUI scale is used
};
