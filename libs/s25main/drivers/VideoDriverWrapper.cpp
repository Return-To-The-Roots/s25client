// Copyright (C) 2005 - 2026 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "VideoDriverWrapper.h"
#include "FrameCounter.h"
#include "RTTR_Version.h"
#include "WindowManager.h"
#include "driver/VideoInterface.h"
#include "helpers/containerUtils.h"
#include "helpers/roundToNextPow2.h"
#include "mygettext/mygettext.h"
#include "ogl/DummyRenderer.h"
#include "ogl/OpenGLRenderer.h"
#include "openglCfg.hpp"
#include "s25util/Log.h"
#include "s25util/error.h"
#include <glad/glad.h>
#include <array>
#include <ctime>
#if !defined(NDEBUG) && defined(HAVE_MEMCHECK_H)
#    include <valgrind/memcheck.h>
#endif

#ifdef _WIN32
using SwapIntervalExt_t = BOOL APIENTRY(int);
#else
using SwapIntervalExt_t = int(int);
#endif

SwapIntervalExt_t* wglSwapIntervalEXT = nullptr;

VideoDriverWrapper::VideoDriverWrapper()
    : videodriver(nullptr, nullptr), renderer_(nullptr), enableMouseWarping(true), texture_current(0)
{}

VideoDriverWrapper::~VideoDriverWrapper()
{
    cleanUp();
    unloadDriver();
}

bool VideoDriverWrapper::initialize()
{
    if(!videodriver || !videodriver->initialize())
    {
        unloadDriver();
        return false;
    }

    LOG.write(_("Loaded video driver \"%1%\"\n")) % getName();

    frameCtr_ = std::make_unique<FrameCounter>();
    frameLimiter_ = std::make_unique<FrameLimiter>();

    return true;
}

bool VideoDriverWrapper::loadDriver(IVideoDriver* existingDriver)
{
    unloadDriver();
    videodriver = Handle(existingDriver, [](IVideoDriver* p) { delete p; });
    return initialize();
}

bool VideoDriverWrapper::loadDriver(std::string& preference)
{
    unloadDriver();
    // DLL laden
    if(!driver_wrapper.Load(drivers::DriverType::Video, preference))
        return false;

    auto createVideoInstance = driver_wrapper.getFunction<CreateVideoInstance_t>("CreateVideoInstance");
    auto freeVideoInstance = driver_wrapper.getFunction<FreeVideoInstance_t>("FreeVideoInstance");
    RTTR_Assert(createVideoInstance && freeVideoInstance);

    videodriver = Handle(createVideoInstance(&WINDOWMANAGER), freeVideoInstance);
    return initialize();
}

void VideoDriverWrapper::unloadDriver()
{
    videodriver.reset();
    driver_wrapper.unload();
    renderer_.reset();
}

bool VideoDriverWrapper::createScreen(const VideoMode size, const DisplayMode displayMode)
{
    if(!videodriver)
    {
        s25util::fatal_error("No video driver selected!");
        return false;
    }

    if(!videodriver->createScreen(rttr::version::getTitle(), size, displayMode))
    {
        s25util::fatal_error("Could not create window!");
        return false;
    }

    // DriverWrapper Initialisieren
    // Extensions laden
    if(!loadAllExtensions())
    {
        s25util::fatal_error("Failed to initialize the OpenGL context!");
        return false;
    }

    renewViewport();

    // Buffer swappen um den leeren Buffer darzustellen
    swapBuffers();

    // WindowManager informieren
    WINDOWMANAGER.msgScreenResize(getRenderSize());

    return true;
}

bool VideoDriverWrapper::resizeScreen(const VideoMode size, const DisplayMode displayMode)
{
    if(!videodriver)
    {
        s25util::fatal_error("No video driver selected!");
        return false;
    }

    const bool result = videodriver->resizeScreen(size, displayMode);
#ifdef _WIN32
    if(videodriver->getDisplayMode() != DisplayMode::Fullscreen)
    {
        // We cannot change the size of a maximized window. So restore it here
        WINDOWPLACEMENT wp;
        wp.length = sizeof(WINDOWPLACEMENT);

        if(GetWindowPlacement((HWND)getMapPointer(), &wp) && ((wp.showCmd & SW_MAXIMIZE) == SW_MAXIMIZE))
            ShowWindow((HWND)getMapPointer(), SW_RESTORE);
    }
#endif
    WINDOWMANAGER.windowResized();
    return result;
}

bool VideoDriverWrapper::destroyScreen()
{
    if(!videodriver)
    {
        s25util::fatal_error("No video driver selected!");
        return false;
    }

    LOG.write("Clearing textures: ");
    unsigned ladezeit = getTickCount();
    cleanUp();
    LOG.write("Finished in %dms\n") % (getTickCount() - ladezeit);

    // Videotreiber zurücksetzen
    videodriver->destroyScreen();

    return true;
}

void VideoDriverWrapper::showErrorMessage(const std::string& title, const std::string& message)
{
    if(videodriver)
        videodriver->showErrorMessage(title, message);
}

void VideoDriverWrapper::setTargetFramerate(int target)
{
    frameLimiter_->setTargetFramerate(target);
    if(!setHwVSync(target == 0) && target == 0) // Fallback if no HW vsync but was requested
        frameLimiter_->setTargetFramerate(60);
}

unsigned VideoDriverWrapper::getFPS() const
{
    return frameCtr_->getFrameRate();
}

void VideoDriverWrapper::cleanUp()
{
    if(!texture_list.empty())
    {
        glDeleteTextures(texture_list.size(), static_cast<const GLuint*>(texture_list.data()));
        texture_list.clear();
    }
}

unsigned VideoDriverWrapper::generateTexture()
{
    GLuint newTexture = 0;
    glGenTextures(1, &newTexture);
#if !defined(NDEBUG) && defined(HAVE_MEMCHECK_H)
    VALGRIND_MAKE_MEM_DEFINED(&newTexture, sizeof(newTexture));
#endif

    static_assert(sizeof(newTexture) == sizeof(texture_list[0]), "Unexpected texture size");
    if(newTexture)
        texture_list.push_back(newTexture);
    return newTexture;
}

void VideoDriverWrapper::bindTexture(unsigned t)
{
    if(t != texture_current)
    {
        texture_current = t;
        glBindTexture(GL_TEXTURE_2D, t);
    }
}

void VideoDriverWrapper::deleteTexture(unsigned t)
{
    if(!t)
        return;
    if(t == texture_current)
        texture_current = 0;
    auto it = helpers::find(texture_list, t);
    if(it != texture_list.end())
    {
        glDeleteTextures(1, &t);
        texture_list.erase(it);
    }
}

KeyEvent VideoDriverWrapper::getModKeyState() const
{
    if(videodriver)
        return videodriver->getModKeyState();
    return KeyEvent();
}

void VideoDriverWrapper::swapBuffers()
{
    if(!videodriver)
    {
        s25util::fatal_error("No video driver selected!");
        return;
    }
    frameLimiter_->sleepTillNextFrame(FrameCounter::clock::now());
    videodriver->swapBuffers();
    FrameCounter::clock::time_point now = FrameCounter::clock::now();
    frameLimiter_->update(now);
    frameCtr_->update(now);
}

void VideoDriverWrapper::clearScreen()
{
    glClear(GL_COLOR_BUFFER_BIT);
}

bool VideoDriverWrapper::run()
{
    if(!videodriver)
    {
        s25util::fatal_error("No video driver selected!");
        return false;
    }

    return videodriver->messageLoop();
}

Extent VideoDriverWrapper::calcPreferredTextureSize(const Extent& minSize) const
{
    return Extent(helpers::roundToNextPowerOfTwo(minSize.x), helpers::roundToNextPowerOfTwo(minSize.y));
}

bool VideoDriverWrapper::setHwVSync(bool enabled)
{
    if(!wglSwapIntervalEXT)
        return false;
    return wglSwapIntervalEXT(enabled ? 1 : 0) != 0;
}

void VideoDriverWrapper::renewViewport()
{
    if(!videodriver->isOpenGL() || !renderer_)
        return;

    const Extent renderSize = videodriver->getRenderSize();
    const VideoMode windowSize = videodriver->getWindowSize();

    // Set the viewport and scissor area to the entire window
    glViewport(0, 0, windowSize.width, windowSize.height);
    glScissor(0, 0, windowSize.width, windowSize.height);

    // Orthogonale Matrix erstellen
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

    // 0,0 should be top left corner
    glOrtho(0, renderSize.x, renderSize.y, 0, -100, 100);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    // Depthbuffer und Colorbuffer einstellen
    glClearColor(0.0, 0.0, 0.0, 1.0);

    // Smooth - Shading aktivieren
    glShadeModel(GL_SMOOTH);

    glEnable(GL_ALPHA_TEST);
    glAlphaFunc(GL_GREATER, 0.0f);

    // Alphablending an
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_BLEND);

    // Depthbuffer abschalten
    glDisable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);

    // Texturen anstellen
    glEnable(GL_TEXTURE_2D);

    // Dither abstellen
    glDisable(GL_DITHER);

    // Scissoring aktivieren
    glEnable(GL_SCISSOR_TEST);

    // Nur obere Seite von Dreiecke rendern --> Performance
    glEnable(GL_CULL_FACE);

    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);

    clearScreen();
}

bool VideoDriverWrapper::loadAllExtensions()
{
    if(videodriver->isOpenGL())
        renderer_ = std::make_unique<OpenGLRenderer>();
    else
        renderer_ = std::make_unique<DummyRenderer>();
    if(!renderer_->initOpenGL(videodriver->getLoaderFunction()))
        return false;
    LOG.write(_("OpenGL %1%.%2% supported\n")) % GLVersion.major % GLVersion.minor;
    if(GLVersion.major < RTTR_OGL_MAJOR || (GLVersion.major == RTTR_OGL_MAJOR && GLVersion.minor < RTTR_OGL_MINOR))
    {
        LOG.write(_("OpenGL %1% %2%.%3% is not supported. Try updating your GPU drivers or hardware!"))
          % ((RTTR_OGL_ES) ? "ES" : "") % RTTR_OGL_MAJOR % RTTR_OGL_MINOR;
        return false;
    }

// auf VSync-Extension testen
#ifdef _WIN32
    wglSwapIntervalEXT = reinterpret_cast<SwapIntervalExt_t*>(loadExtension("wglSwapIntervalEXT"));
#else
    wglSwapIntervalEXT = reinterpret_cast<SwapIntervalExt_t*>(loadExtension("glXSwapIntervalSGI"));
#endif

    return true;
}

unsigned VideoDriverWrapper::getTickCount()
{
    if(!videodriver)
        return (unsigned)time(nullptr);

    return (unsigned)videodriver->getTickCount();
}

std::string VideoDriverWrapper::getName() const
{
    const char* name = (videodriver) ? videodriver->getName() : nullptr;
    return (name) ? name : "";
}

/**
 *  lädt eine bestimmte DriverWrapper Extension-Funktion.
 *
 *  @param[in] extension Die Extension-Funktion
 *
 *  @return @p nullptr bei Fehler, Adresse der gewünschten Funktion bei Erfolg.
 */
void* VideoDriverWrapper::loadExtension(const std::string& extension)
{
    if(!videodriver)
    {
        s25util::fatal_error("No video driver selected!");
        return (nullptr);
    }

    return videodriver->getLoaderFunction()(extension.c_str());
}

Position VideoDriverWrapper::getMousePos() const
{
    if(!videodriver)
        return Position::invalid();
    return videodriver->getMousePos();
}

bool VideoDriverWrapper::isLeftDown()
{
    if(!videodriver)
        return false;

    return videodriver->getMouseStateL();
}

bool VideoDriverWrapper::isRightDown()
{
    if(!videodriver)
        return false;

    return videodriver->getMouseStateR();
}

bool VideoDriverWrapper::isTouch()
{
    if(!videodriver)
        return false;

    return videodriver->isTouchEvent();
}

void VideoDriverWrapper::setMousePos(const Position& newPos)
{
    if(!videodriver || !enableMouseWarping)
        return;

    videodriver->setMousePos(newPos);
}

std::set<VideoMode> VideoDriverWrapper::listVideoModes() const
{
    if(!videodriver)
        return {};

    const auto videoModes = videodriver->listVideoModes();
    std::set<VideoMode> result;
    // Keep only modes larger than the minimum
    std::copy_if(videoModes.begin(), videoModes.end(), std::inserter(result, result.end()),
                 [](const auto& m) { return m.width >= MinWindowSize.width || m.height >= MinWindowSize.height; });
    return result;
}

std::set<VideoMode> VideoDriverWrapper::getDefaultWindowSizes() const
{
    // clang-format off
    std::array defaultWindowSizes = {
      VideoMode(800, 600),
      VideoMode(1024, 768),
      VideoMode(1152, 648),
      VideoMode(1280, 720),
      VideoMode(1280, 800),
      VideoMode(1366, 768),
      VideoMode(1440, 810),
      VideoMode(1600, 900),
      VideoMode(1680, 1050),
      VideoMode(1920, 1080)
    };
    // clang-format on
    std::set<VideoMode> windowSizes = listVideoModes();
    windowSizes.insert(defaultWindowSizes.begin(), defaultWindowSizes.end());
    return windowSizes;
}

bool VideoDriverWrapper::hasVSync() const
{
    return wglSwapIntervalEXT != nullptr;
}

void* VideoDriverWrapper::getMapPointer() const
{
    if(!videodriver)
        return nullptr;

    return videodriver->getMapPointer();
}

VideoMode VideoDriverWrapper::getWindowSize() const
{
    // Always return at least MinWindowSize even if real window is smaller
    VideoMode windowSize = videodriver->getWindowSize();
    windowSize.width = std::max(MinWindowSize.width, windowSize.width);
    windowSize.height = std::max(MinWindowSize.height, windowSize.height);
    return windowSize;
}

Extent VideoDriverWrapper::getRenderSize() const
{
    return videodriver->getRenderSize();
}

DisplayMode VideoDriverWrapper::getDisplayMode() const
{
    return videodriver->getDisplayMode();
}

float VideoDriverWrapper::getDpiScale() const
{
    return videodriver->getDpiScale();
}

const GuiScale& VideoDriverWrapper::getGuiScale() const
{
    return videodriver->getGuiScale();
}

void VideoDriverWrapper::setGuiScalePercent(unsigned percent)
{
    videodriver->setGuiScalePercent(percent);
}

GuiScaleRange VideoDriverWrapper::getGuiScaleRange() const
{
    return videodriver->getGuiScaleRange();
}
