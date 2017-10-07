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
#ifndef VIDEOINTERFACE_H_INCLUDED
#define VIDEOINTERFACE_H_INCLUDED

#pragma once

#include "KeyEvent.h"
#include "VideoMode.h"
#include <string>
#include <vector>

class IVideoDriver
{
public:
    virtual ~IVideoDriver() = 0;

    /// Funktion zum Auslesen des Treibernamens.
    virtual const char* GetName() const = 0;

    virtual bool Initialize() = 0;

    virtual void CleanUp() = 0;

    /// Erstellt das Fenster mit entsprechenden Werten.
    virtual bool CreateScreen(const std::string& title, unsigned short width, unsigned short height, const bool fullscreen) = 0;

    virtual bool ResizeScreen(unsigned short width, unsigned short height, const bool fullscreen) = 0;

    /// Schliesst das Fenster.
    virtual void DestroyScreen() = 0;

    /// Wechselt die OpenGL-Puffer.
    virtual bool SwapBuffers() = 0;

    /// Die Nachrichtenschleife.
    virtual bool MessageLoop() = 0;

    /// Funktion zum Auslesen des TickCounts.
    virtual unsigned long GetTickCount() const = 0;

    /// Funktion zum Holen einer Subfunktion.
    virtual void* GetFunction(const char* function) const = 0;

    virtual void ListVideoModes(std::vector<VideoMode>& video_modes) const = 0;

    /// Funktion zum Auslesen der Mauskoordinaten.
    virtual void GetMousePos(int& x, int& y) const = 0;

    /// Funktion zum Setzen der Mauskoordinaten.
    virtual void SetMousePos(int x, int y) = 0;

    /// Return true when left mouse button is pressed
    virtual bool GetMouseStateL() const = 0;

    /// Return true when right mouse button is pressed
    virtual bool GetMouseStateR() const = 0;

    virtual unsigned short GetScreenWidth() const = 0;
    virtual unsigned short GetScreenHeight() const = 0;
    virtual bool IsFullscreen() const = 0;

    /// Get state of the modifier keys
    virtual KeyEvent GetModKeyState() const = 0;

    /// Get pointer to window (device-dependent!), HWND unter Windows
    virtual void* GetMapPointer() const = 0;

    virtual bool IsInitialized() = 0;
    /// Shall we support OpenGL? (Disabled for tests)
    virtual bool IsOpenGL() = 0;
};

class VideoDriverLoaderInterface;

///////////////////////////////////////////////////////////////////////////////
// Makros / Defines
#undef DRIVERDLLAPI
#ifdef _WIN32
#ifdef BUILD_DLL
#define DRIVERDLLAPI extern "C" __declspec(dllexport)
#else
#define DRIVERDLLAPI extern "C" __declspec(dllimport)
#endif // !_USRDLL
#else
#define DRIVERDLLAPI extern "C"
#endif // !_WIN32

/// Instanzierungsfunktion der Treiber.
DRIVERDLLAPI IVideoDriver* CreateVideoInstance(VideoDriverLoaderInterface* CallBack);
DRIVERDLLAPI void FreeVideoInstance(IVideoDriver* driver);

///
typedef IVideoDriver* (*PDRIVER_CREATEVIDEOINSTANCE)(VideoDriverLoaderInterface*);
typedef void (*PDRIVER_FREEVIDEOINSTANCE)(IVideoDriver*);

#endif // !VIDEOINTERFACE_H_INCLUDED
