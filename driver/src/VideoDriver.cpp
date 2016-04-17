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

#include "main.h" // IWYU pragma: keep
#include "VideoDriver.h"
#include <algorithm>

// Include last!
#include "DebugNew.h" // IWYU pragma: keep

// Do not inline! That would break DLL compatibility: http://stackoverflow.com/questions/32444520/how-to-handle-destructors-in-dll-exported-interfaces
IVideoDriver::~IVideoDriver(){}

///////////////////////////////////////////////////////////////////////////////
/** @var typedef void (*VideoDriverLoaderInterface *)(unsigned int msg, void *param)
 *
 *  Definition des DriverCallback-Zeigertyps.
 *
 *  @author FloSoft
 */

///////////////////////////////////////////////////////////////////////////////
/** @class VideoDriver
 *
 *  Basisklasse für einen Videotreiber.
 *
 *  @author FloSoft
 */

///////////////////////////////////////////////////////////////////////////////
/** @var VideoGLFW::CallBack
 *
 *  Das DriverCallback für Rückmeldungen.
 *
 *  @author FloSoft
 */

///////////////////////////////////////////////////////////////////////////////
/** @var VideoGLFW::initialized
 *
 *  Initialisierungsstatus.
 *
 *  @author FloSoft
 */

///////////////////////////////////////////////////////////////////////////////
/** @var VideoGLFW::mouse_xy
 *
 *  Status der Maus.
 *
 *  @author FloSoft
 */

///////////////////////////////////////////////////////////////////////////////
/** @var VideoGLFW::keyboard
 *
 *  Status der Tastatur;
 *
 *  @author FloSoft
 */

///////////////////////////////////////////////////////////////////////////////
/** @var VideoGLFW::fullscreen
 *
 *  Vollbild an/aus?
 *
 *  @author FloSoft
 */

///////////////////////////////////////////////////////////////////////////////
/**
 *  Konstruktor von @p VideoDriver.
 *
 *  @param[in] CallBack DriverCallback für Rückmeldungen.
 *
 *  @author FloSoft
 */
VideoDriver::VideoDriver(VideoDriverLoaderInterface* CallBack) : CallBack(CallBack), initialized(false), screenWidth(0), screenHeight(0), isFullscreen_(false)
{
    std::fill(keyboard.begin(), keyboard.end(), false);
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  Funktion zum Auslesen der Mauskoordinaten.
 *
 *  @param[out] x X-Koordinate
 *  @param[out] y Y-Koordinate
 *
 *  @author FloSoft
 */
void VideoDriver::GetMousePos(int& x, int& y) const
{
    x = mouse_xy.x;
    y = mouse_xy.y;
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  Funktion zum Auslesen der X-Koordinate der Maus.
 *
 *  @return liefert die X-Koordinate
 *
 *  @author FloSoft
 */
int VideoDriver::GetMousePosX() const
{
    return mouse_xy.x;
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  Funktion zum Auslesen der Y-Koordinate der Maus.
 *
 *  @return liefert die Y-Koordinate
 *
 *  @author FloSoft
 */
int VideoDriver::GetMousePosY() const
{
    return mouse_xy.y;
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  Funktion zum Auslesen ob die Linke Maustaste gedrückt ist.
 *
 *  @return @p true bei Gedrückt, @p false bei Losgelassen
 *
 *  @author FloSoft
 */
bool VideoDriver::GetMouseStateL() const
{
    return mouse_xy.ldown;
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  Funktion zum Auslesen ob die Rechte Maustaste gedrückt ist.
 *
 *  @return @p true bei Gedrückt, @p false bei Losgelassen
 *
 *  @author FloSoft
 */
bool VideoDriver::GetMouseStateR() const
{
    return mouse_xy.rdown;
}
