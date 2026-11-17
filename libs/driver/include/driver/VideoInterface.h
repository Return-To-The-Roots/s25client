// Copyright (C) 2005 - 2026 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "GuiScale.h"
#include "KeyEvent.h"
#include "Point.h"
#include "VideoMode.h"
#include "exportImport.h"
#include <string>
#include <type_traits>
#include <vector>

/// Function type for loading OpenGL methods
using OpenGL_Loader_Proc = void* (*)(const char*);

class BOOST_SYMBOL_VISIBLE IVideoDriver
{
public:
    virtual ~IVideoDriver() = 0;

    /// Funktion zum Auslesen des Treibernamens.
    virtual const char* getName() const = 0;

    virtual bool initialize() = 0;

    /// Erstellt das Fenster mit entsprechenden Werten.
    virtual bool createScreen(const std::string& title, VideoMode newSize, DisplayMode displayMode) = 0;

    virtual bool resizeScreen(VideoMode newSize, DisplayMode displayMode) = 0;

    /// Schliesst das Fenster.
    virtual void destroyScreen() = 0;

    /// Wechselt die OpenGL-Puffer.
    virtual bool swapBuffers() = 0;

    /// Die Nachrichtenschleife.
    virtual bool messageLoop() = 0;

    /// Return the current tick count (time since epoch in ms)
    virtual unsigned long getTickCount() const = 0;

    /// Funktion zum Holen einer Subfunktion.
    virtual OpenGL_Loader_Proc getLoaderFunction() const = 0;

    virtual std::vector<VideoMode> listVideoModes() const = 0;

    /// Funktion zum Auslesen der Mauskoordinaten.
    virtual Position getMousePos() const = 0;

    /// Funktion zum Setzen der Mauskoordinaten.
    virtual void setMousePos(Position pos) = 0;

    /// Return true when left mouse button is pressed
    virtual bool getMouseStateL() const = 0;
    /// Return true when right mouse button is pressed
    virtual bool getMouseStateR() const = 0;
    /// Return true if at least 1 finger is on screen
    virtual bool isTouchEvent() const = 0;

    /// Get the size of the window in screen coordinates
    virtual VideoMode getWindowSize() const = 0;
    /// Get the size of the render region in pixels
    virtual Extent getRenderSize() const = 0;
    virtual DisplayMode getDisplayMode() const = 0;

    /// Get the factor required to scale "normal" DPI to the display DPI
    virtual float getDpiScale() const = 0;

    /// Get the scale applied to the user interface
    virtual const GuiScale& getGuiScale() const = 0;

    /// Set the scale applied to the user interface in percent
    virtual void setGuiScalePercent(unsigned percent) = 0;

    /// Get minimum, maximum, and recommended GUI scale percentages for the current window and render size
    virtual GuiScaleRange getGuiScaleRange() const = 0;

    /// Get state of the modifier keys
    virtual KeyEvent getModKeyState() const = 0;

    /// Get pointer to window (device-dependent!), HWND unter Windows
    virtual void* getMapPointer() const = 0;

    virtual bool isInitialized() const = 0;
    /// Shall we support OpenGL? (Disabled for tests)
    virtual bool isOpenGL() const = 0;

    // Display the problem to the gamer
    virtual void showErrorMessage(const std::string& title, const std::string& message) = 0;
};

class VideoDriverLoaderInterface;

/// Instanzierungsfunktion der Treiber.
RTTR_DECL IVideoDriver* CreateVideoInstance(VideoDriverLoaderInterface* CallBack);
RTTR_DECL void FreeVideoInstance(IVideoDriver* driver);

using CreateVideoInstance_t = decltype(CreateVideoInstance);
using FreeVideoInstance_t = decltype(FreeVideoInstance);
