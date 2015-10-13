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
#ifndef VIDEOINTERFACE_H_INCLUDED
#define VIDEOINTERFACE_H_INCLUDED

#pragma once

#include "Interface.h"
#include "KeyEvent.h"
#include <vector>

/// Listet verfügbare Videomodi auf
struct VideoMode
{
    unsigned short width;
    unsigned short height;

    VideoMode(): width(0), height(0){}
    VideoMode(unsigned short width, unsigned short height): width(width), height(height){}
    bool operator==(const VideoMode& o) const { return (width == o.width && height == o.height); }
    bool operator!=(const VideoMode& o) const { return !(*this==o); }
};

class IVideoDriver
{
public:
    virtual ~IVideoDriver() = 0;

    /// Funktion zum Auslesen des Treibernamens.
    virtual const char* GetName(void) const = 0;

    /// Treiberinitialisierungsfunktion.
    virtual bool Initialize(void) = 0;

    /// Treiberaufräumfunktion.
    virtual void CleanUp(void) = 0;

    /// Erstellt das Fenster mit entsprechenden Werten.
    virtual bool CreateScreen(unsigned short width, unsigned short height, const bool fullscreen) = 0;

    /// Erstellt oder verändert das Fenster mit entsprechenden Werten.
    virtual bool ResizeScreen(unsigned short width, unsigned short height, const bool fullscreen) = 0;

    /// Schliesst das Fenster.
    virtual void DestroyScreen(void) = 0;

    /// Wechselt die OpenGL-Puffer.
    virtual bool SwapBuffers(void) = 0;

    /// Die Nachrichtenschleife.
    virtual bool MessageLoop(void) = 0;

    /// Funktion zum Auslesen des TickCounts.
    virtual unsigned long GetTickCount(void) const = 0;

    /// Funktion zum Holen einer Subfunktion.
    virtual void* GetFunction(const char* function) const = 0;

    virtual void ListVideoModes(std::vector<VideoMode>& video_modes) const = 0;

    /// Funktion zum Auslesen der Mauskoordinaten.
    virtual void GetMousePos(int& x, int& y) const = 0;

    /// Funktion zum Setzen der Mauskoordinaten.
    virtual void SetMousePos(int x, int y) = 0;

    /// Funktion zum Auslesen der X-Koordinate der Maus.
    virtual int GetMousePosX() const = 0;

    /// Funktion zum Setzen der X-Koordinate der Maus.
    virtual void SetMousePosX(int x) = 0;

    /// Funktion zum Auslesen der Y-Koordinate der Maus.
    virtual int GetMousePosY() const = 0;

    /// Funktion zum Setzen der Y-Koordinate der Maus.
    virtual void SetMousePosY(int y) = 0;

    /// Funktion zum Auslesen ob die Linke Maustaste gedrückt ist.
    virtual bool GetMouseStateL(void) const = 0;

    /// Funktion zum Auslesen ob die Rechte Maustaste gedrückt ist.
    virtual bool GetMouseStateR(void) const = 0;

    //
    virtual unsigned short GetScreenWidth()  const = 0;
    virtual unsigned short GetScreenHeight() const = 0;
    virtual bool IsFullscreen() const = 0;

    /// Get state of the modifier keys
    virtual KeyEvent GetModKeyState(void) const = 0;

    /// Gibt Pointer auf ein Fenster zurück (device-dependent!), HWND unter Windows
    virtual void* GetMapPointer() const = 0;

    /// prüft auf Initialisierung.
    virtual bool IsInitialized() = 0;
};

class VideoDriverLoaderInterface;

///////////////////////////////////////////////////////////////////////////////
// Makros / Defines
#undef DRIVERDLLAPI
#ifdef _WIN32
#   if defined _USRDLL || defined _LIB || defined BUILD_DLL
#       define DRIVERDLLAPI extern "C" __declspec(dllexport)
#   else
#       define DRIVERDLLAPI extern "C" __declspec(dllimport)
#   endif // !_USRDLL
#else
#   define DRIVERDLLAPI extern "C"
#endif // !_WIN32

/// Instanzierungsfunktion der Treiber.
DRIVERDLLAPI IVideoDriver* CreateVideoInstance(VideoDriverLoaderInterface* CallBack);
DRIVERDLLAPI void FreeVideoInstance(IVideoDriver* driver);

///
typedef IVideoDriver* (*PDRIVER_CREATEVIDEOINSTANCE)(VideoDriverLoaderInterface*);
typedef void (*PDRIVER_FREEVIDEOINSTANCE)(IVideoDriver*);


#endif // !VIDEOINTERFACE_H_INCLUDED
