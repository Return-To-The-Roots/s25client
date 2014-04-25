// $Id: VideoDriver.cpp 9357 2014-04-25 15:35:25Z FloSoft $
//
// Copyright (c) 2005 - 2011 Settlers Freaks (sf-team at siedler25.org)
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

///////////////////////////////////////////////////////////////////////////////
// Header
#include "main.h"
#include "VideoDriver.h"

///////////////////////////////////////////////////////////////////////////////
// Makros / Defines
#if defined _WIN32 && defined _DEBUG && defined _MSC_VER
#define new new(_NORMAL_BLOCK, THIS_FILE, __LINE__)
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

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
VideoDriver::VideoDriver(VideoDriverLoaderInterface* CallBack) : CallBack(CallBack), initialized(false), screenWidth(0), screenHeight(0), fullscreen(false)
{
    memset(keyboard, 0, 512);
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  Destruktor von @p VideoDriver.
 *
 *  @author FloSoft
 */
VideoDriver::~VideoDriver(void)
{
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  Funktion zum Auslesen des Treibernamens.
 *
 *  @return liefert den Treibernamen zurück
 *
 *  @author FloSoft
 */
const char* VideoDriver::GetName(void) const
{
    return NULL;
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  Treiberinitialisierungsfunktion.
 *
 *  @return @p true bei Erfolg, @p false bei Fehler
 *
 *  @author FloSoft
 */
bool VideoDriver::Initialize(void)
{
    return false;
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  Treiberaufräumfunktion.
 *
 *  @author FloSoft
 */
void VideoDriver::CleanUp(void)
{
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  Erstellt das Fenster mit entsprechenden Werten.
 *
 *  @param[in] width      Breite des Fensters
 *  @param[in] height     Höhe des Fensters
 *  @param[in] fullscreen Vollbildmodus ja oder nein
 *
 *  @return @p true bei Erfolg, @p false bei Fehler
 *
 *  @author FloSoft
 */
bool VideoDriver::CreateScreen(unsigned short width, unsigned short height, const bool fullscreen)
{
    return false;
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  Erstellt oder verändert das Fenster mit entsprechenden Werten.
 *
 *  @param[in] width      Breite des Fensters
 *  @param[in] height     Höhe des Fensters
 *  @param[in] fullscreen Vollbildmodus ja oder nein
 *
 *  @return @p true bei Erfolg, @p false bei Fehler
 *
 *  @author FloSoft
 */
bool VideoDriver::ResizeScreen(unsigned short width, unsigned short height, const bool fullscreen)
{
    return false;
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  Schliesst das Fenster.
 *
 *  @author FloSoft
 */
void VideoDriver::DestroyScreen(void)
{
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  Wechselt die OpenGL-Puffer.
 *
 *  @return @p true bei Erfolg, @p false bei Fehler
 *
 *  @author FloSoft
 */
bool VideoDriver::SwapBuffers(void)
{
    return false;
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  Die Nachrichtenschleife.
 *
 *  @return @p true bei Erfolg, @p false bei Fehler
 *
 *  @author FloSoft
 */
bool VideoDriver::MessageLoop(void)
{
    return false;
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  Funktion zum Auslesen des TickCounts.
 *
 *  @return liefert den TickCount
 *
 *  @author FloSoft
 */
unsigned long VideoDriver::GetTickCount(void) const
{
    return 0;
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  Funktion zum Holen einer Subfunktion.
 *
 *  @param[in] function Name der Funktion welche geholt werden soll.
 *
 *  @return Adresse der Funktion bei Erfolg, @p NULL bei Fehler
 *
 *  @author FloSoft
 */
void* VideoDriver::GetFunction(const char* function) const
{
    return NULL;
}

/// Listet verfügbare Videomodi auf
void VideoDriver::ListVideoModes(std::vector<VideoMode>& video_modes) const
{
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
 *  Funktion zum Setzen der Mauskoordinaten.
 *
 *  @param[in] x X-Koordinate
 *  @param[in] y Y-Koordinate
 *
 *  @author FloSoft
 */
void VideoDriver::SetMousePos(int x, int y)
{
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
 *  Funktion zum Setzen der X-Koordinate der Maus.
 *
 *  @param[in] x X-Koordinate
 *
 *  @author FloSoft
 */
void VideoDriver::SetMousePosX(int x)
{
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
 *  Funktion zum Setzen der Y-Koordinate der Maus.
 *
 *  @param[in] y Y-Koordinate
 *
 *  @author FloSoft
 */
void VideoDriver::SetMousePosY(int y)
{
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  Funktion zum Auslesen ob die Linke Maustaste gedrückt ist.
 *
 *  @return @p true bei Gedrückt, @p false bei Losgelassen
 *
 *  @author FloSoft
 */
bool VideoDriver::GetMouseStateL(void) const
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
bool VideoDriver::GetMouseStateR(void) const
{
    return mouse_xy.rdown;
}
