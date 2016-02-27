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
        /// Konstruktor von @p Videotreiber.
        VideoDriver(VideoDriverLoaderInterface* CallBack);

        /// Destruktor von @p Videotreiber.
        virtual ~VideoDriver(void){}

        /// Funktion zum Auslesen der Mauskoordinaten.
        virtual void GetMousePos(int& x, int& y) const;

        /// Funktion zum Auslesen der X-Koordinate der Maus.
        virtual int GetMousePosX() const;

        /// Funktion zum Auslesen der Y-Koordinate der Maus.
        virtual int GetMousePosY() const;

        /// Funktion zum Auslesen ob die Linke Maustaste gedrückt ist.
        virtual bool GetMouseStateL(void) const;

        /// Funktion zum Auslesen ob die Rechte Maustaste gedrückt ist.
        virtual bool GetMouseStateR(void) const;

        //
        virtual unsigned short GetScreenWidth()  const { return screenWidth;  }
        virtual unsigned short GetScreenHeight() const { return screenHeight; }
        virtual bool IsFullscreen() const { return isFullscreen_; }

        /// prüft auf Initialisierung.
        virtual bool IsInitialized() { return initialized; }

    protected:
        VideoDriverLoaderInterface* CallBack;  ///< Das DriverCallback für Rückmeldungen.
        bool initialized;            ///< Initialisierungsstatus.
        MouseCoords mouse_xy;        ///< Status der Maus.
        boost::array<bool, 512> keyboard; ///< Status der Tastatur;
        unsigned short screenWidth;  ///< aktuelle Bildschirm-/Fensterbreite
        unsigned short screenHeight; ///< aktuelle Bildschirm-/Fensterhöhe
        bool isFullscreen_;             ///< Vollbild an/aus?
};
#endif // !VIDEODRIVER_H_INCLUDED
