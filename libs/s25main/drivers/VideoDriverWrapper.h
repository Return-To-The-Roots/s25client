// Copyright (C) 2005 - 2026 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "DriverWrapper.h"
#include "Point.h"
#include "driver/GuiScale.h"
#include "driver/KeyEvent.h"
#include "driver/VideoInterface.h"
#include "driver/VideoMode.h"
#include "s25util/Singleton.h"
#include <memory>
#include <set>
#include <string>
#include <vector>

class IVideoDriver;
class IRenderer;
class FrameCounter;
class FrameLimiter;

///////////////////////////////////////////////////////////////////////////////
// DriverWrapper
class VideoDriverWrapper : public Singleton<VideoDriverWrapper, SingletonPolicies::WithLongevity>
{
public:
    static constexpr unsigned Longevity = 30;

    VideoDriverWrapper();
    ~VideoDriverWrapper();

    /// Loads a new driver.
    /// Do not use this class unless this returned true!
    bool loadDriver(IVideoDriver* existingDriver);
    bool loadDriver(std::string& preference);
    void unloadDriver();
    IVideoDriver* getDriver() const { return videodriver.get(); }

    /// Erstellt das Fenster.
    bool createScreen(VideoMode size, DisplayMode displayMode);
    /// Verändert Auflösung, Fenster/Fullscreen
    bool resizeScreen(VideoMode size, DisplayMode displayMode);
    /// Viewport (neu) setzen
    void renewViewport();
    /// zerstört das Fenster.
    bool destroyScreen();
    /// räumt die Texturen auf
    void cleanUp();
    /// erstellt eine Textur
    unsigned generateTexture();
    void bindTexture(unsigned t);
    void deleteTexture(unsigned t);

    IRenderer* getRenderer() { return renderer_.get(); }

    /// Swapped den Buffer
    void swapBuffers();
    /// Clears the screen (glClear)
    void clearScreen();
    // Should only be used to draw the mouse. For everything else use the events
    Position getMousePos() const;

    /// Get supported resolutions
    std::set<VideoMode> listVideoModes() const;
    std::set<VideoMode> getDefaultWindowSizes() const;
    bool hasVSync() const;

    /// Gibt Pointer auf ein Fenster zurück (device-dependent!), HWND unter Windows
    void* getMapPointer() const;
    /// Get the size/resolution of the window in screen coordinates
    VideoMode getWindowSize() const;
    /// Get the renderer size in pixels
    Extent getRenderSize() const;
    DisplayMode getDisplayMode() const;

    /// Get the factor required to scale "normal" DPI to the display DPI
    float getDpiScale() const;

    /// Get the scale applied to the user interface
    const GuiScale& getGuiScale() const;

    /// Set the scale applied to the user interface in percent
    void setGuiScalePercent(unsigned percent);

    /// Get minimum, maximum, and recommended GUI scale percentages for the current window and render size
    GuiScaleRange getGuiScaleRange() const;

    bool isLeftDown();
    bool isRightDown();
    bool isTouch();
    // setzt den Mausstatus
    void setMousePos(const Position& newPos);
    void setMouseWarping(bool enabled) { enableMouseWarping = enabled; }
    /// Get state of the modifier keys
    KeyEvent getModKeyState() const;

    // Nachrichtenschleife
    bool run();

    void showErrorMessage(const std::string& title, const std::string& message);

    unsigned getTickCount();
    /// Set framerate target (FPS)
    /// negative for unlimited, 0 for hardware VSync
    void setTargetFramerate(int target);
    unsigned getFPS() const;

    std::string getName() const;
    bool isLoaded() const { return videodriver != nullptr; }

    /// Calculate the size of the texture which is optimal for the driver and at least minSize
    Extent calcPreferredTextureSize(const Extent& minSize) const;

    static constexpr VideoMode MinWindowSize{800, 600};

private:
    bool initialize();
    bool setHwVSync(bool enabled);

    // lädt eine Funktion aus den Extensions
    void* loadExtension(const std::string& extension);

    // Alle (im Programm benutzen) Extensions laden
    bool loadAllExtensions();

    using Handle = std::unique_ptr<IVideoDriver, void (*)(IVideoDriver*)>;

    drivers::DriverWrapper driver_wrapper;
    Handle videodriver;
    std::unique_ptr<IRenderer> renderer_;
    std::unique_ptr<FrameCounter> frameCtr_;
    std::unique_ptr<FrameLimiter> frameLimiter_;
    bool enableMouseWarping;

    std::vector<unsigned> texture_list;
    unsigned texture_current;
};

#define VIDEODRIVER VideoDriverWrapper::inst()
