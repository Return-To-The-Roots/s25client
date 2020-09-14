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
    Extent GetRenderSize() const override final { return renderSize_; }
    bool IsFullscreen() const override final { return isFullscreen_; }

    /// prüft auf Initialisierung.
    bool IsInitialized() const override final { return initialized; }
    bool IsOpenGL() const override { return true; }

protected:
    VideoMode FindClosestVideoMode(const VideoMode& mode) const;
    void SetNewSize(VideoMode windowSize, Extent renderSize);

    VideoDriverLoaderInterface* CallBack; /// Das DriverCallback für Rückmeldungen.
    bool initialized;                     /// Initialisierungsstatus.
    MouseCoords mouse_xy;                 /// Status der Maus.
    std::array<bool, 512> keyboard;       /// Status der Tastatur;
    bool isFullscreen_;                   /// Vollbild an/aus?
private:
    // cached as possibly used often
    VideoMode windowSize_;
    Extent renderSize_;
};
