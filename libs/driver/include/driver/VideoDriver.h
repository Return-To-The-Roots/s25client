// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
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
    Position GetMousePos() const override;

    /// Funktion zum Auslesen ob die Linke Maustaste gedrückt ist.
    bool GetMouseStateL() const override;

    /// Funktion zum Auslesen ob die Rechte Maustaste gedrückt ist.
    bool GetMouseStateR() const override;

    VideoMode GetWindowSize() const override final { return windowSize_; }
    Extent GetRenderSize() const override final { return scaledRenderSize_; }
    bool IsFullscreen() const override final { return isFullscreen_; }

    const GuiScale& getGuiScale() const override final { return guiScale_; }

    GuiScaleRange getGuiScaleRange() const override;

    /// prüft auf Initialisierung.
    bool IsInitialized() const override final { return initialized; }
    bool IsOpenGL() const override { return true; }

protected:
    VideoMode FindClosestVideoMode(const VideoMode& mode) const;
    void SetNewSize(VideoMode windowSize, Extent renderSize);

    /// Set the GUI scale in percent and return true if changed
    bool setGuiScaleInternal(unsigned percent);

    VideoDriverLoaderInterface* CallBack; /// Das DriverCallback für Rückmeldungen.
    bool initialized;                     /// Initialisierungsstatus.
    MouseCoords mouse_xy;                 /// Status der Maus.
    std::array<bool, 512> keyboard;       /// Status der Tastatur;
    bool isFullscreen_;                   /// Vollbild an/aus?
private:
    // cached as possibly used often
    VideoMode windowSize_;    ///< Size of the window or fullscreen resolution
    Extent renderSize_;       ///< Size of the renderable surface
    Extent scaledRenderSize_; ///< Size of the projection clipping area

    GuiScale guiScale_; ///< Scale factor applied to the user interface
};
