// Copyright (c) 2005 - 2015 Settlers Freaks (sf-team at siedler25.org)
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
#ifndef VIDEODRIVER_H_INCLUDED
#define VIDEODRIVER_H_INCLUDED

#pragma once

///////////////////////////////////////////////////////////////////////////////
// Header
#include "VideoInterface.h"
#include "MouseCoords.h"
#include <boost/array.hpp>

class VideoDriverLoaderInterface;

/// Basisklasse für einen Videotreiber.
class VideoDriver: public IVideoDriver
{
    public:
        VideoDriver(VideoDriverLoaderInterface* CallBack);

        ~VideoDriver() override{}

        /// Funktion zum Auslesen der Mauskoordinaten.
        void GetMousePos(int& x, int& y) const override;

        /// Funktion zum Auslesen der X-Koordinate der Maus.
        int GetMousePosX() const override;

        /// Funktion zum Auslesen der Y-Koordinate der Maus.
        int GetMousePosY() const override;

        /// Funktion zum Auslesen ob die Linke Maustaste gedrückt ist.
        bool GetMouseStateL() const override;

        /// Funktion zum Auslesen ob die Rechte Maustaste gedrückt ist.
        bool GetMouseStateR() const override;

        //
        unsigned short GetScreenWidth()  const override { return screenWidth;  }
        unsigned short GetScreenHeight() const override { return screenHeight; }
        bool IsFullscreen() const override { return isFullscreen_; }

        /// prüft auf Initialisierung.
        bool IsInitialized() override { return initialized; }

    protected:
        VideoDriverLoaderInterface* CallBack;  /// Das DriverCallback für Rückmeldungen.
        bool initialized;            /// Initialisierungsstatus.
        MouseCoords mouse_xy;        /// Status der Maus.
        boost::array<bool, 512> keyboard; /// Status der Tastatur;
        unsigned short screenWidth;  /// aktuelle Bildschirm-/Fensterbreite
        unsigned short screenHeight; /// aktuelle Bildschirm-/Fensterhöhe
        bool isFullscreen_;             /// Vollbild an/aus?
};
#endif // !VIDEODRIVER_H_INCLUDED
