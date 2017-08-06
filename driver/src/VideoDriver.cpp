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

#include "driverDefines.h" // IWYU pragma: keep
#include "VideoDriver.h"
#include <algorithm>

// Do not inline! That would break DLL compatibility:
// http://stackoverflow.com/questions/32444520/how-to-handle-destructors-in-dll-exported-interfaces
IVideoDriver::~IVideoDriver()
{
}

/** @var typedef void (*VideoDriverLoaderInterface *)(unsigned msg, void *param)
 *
 *  Definition des DriverCallback-Zeigertyps.
 */

/** @class VideoDriver
 *
 *  Basisklasse für einen Videotreiber.
 */

/** @var VideoGLFW::CallBack
 *
 *  Das DriverCallback für Rückmeldungen.
 */

/** @var VideoGLFW::initialized
 *
 *  Initialisierungsstatus.
 */

/** @var VideoGLFW::mouse_xy
 *
 *  Status der Maus.
 */

/** @var VideoGLFW::keyboard
 *
 *  Status der Tastatur;
 */

/** @var VideoGLFW::fullscreen
 *
 *  Vollbild an/aus?
 */

/**
 *  Konstruktor von @p VideoDriver.
 *
 *  @param[in] CallBack DriverCallback für Rückmeldungen.
 */
VideoDriver::VideoDriver(VideoDriverLoaderInterface* CallBack)
    : CallBack(CallBack), initialized(false), screenWidth(0), screenHeight(0), isFullscreen_(false)
{
    std::fill(keyboard.begin(), keyboard.end(), false);
}

/**
 *  Funktion zum Auslesen der Mauskoordinaten.
 *
 *  @param[out] x X-Koordinate
 *  @param[out] y Y-Koordinate
 */
void VideoDriver::GetMousePos(int& x, int& y) const
{
    x = mouse_xy.x;
    y = mouse_xy.y;
}

/**
 *  Funktion zum Auslesen ob die Linke Maustaste gedrückt ist.
 *
 *  @return @p true bei Gedrückt, @p false bei Losgelassen
 */
bool VideoDriver::GetMouseStateL() const
{
    return mouse_xy.ldown;
}

/**
 *  Funktion zum Auslesen ob die Rechte Maustaste gedrückt ist.
 *
 *  @return @p true bei Gedrückt, @p false bei Losgelassen
 */
bool VideoDriver::GetMouseStateR() const
{
    return mouse_xy.rdown;
}
