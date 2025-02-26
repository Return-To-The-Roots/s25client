// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
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
    CleanUp();
    UnloadDriver();
}

bool VideoDriverWrapper::Initialize()
{
    if(!videodriver || !videodriver->Initialize())
    {
        UnloadDriver();
        return false;
    }

    LOG.write(_("Loaded video driver \"%1%\"\n")) % GetName();

    frameCtr_ = std::make_unique<FrameCounter>();
    frameLimiter_ = std::make_unique<FrameLimiter>();

    return true;
}

bool VideoDriverWrapper::LoadDriver(IVideoDriver* existingDriver)
{
    UnloadDriver();
    videodriver = Handle(existingDriver, [](IVideoDriver* p) { delete p; });
    return Initialize();
}

bool VideoDriverWrapper::LoadDriver(std::string& preference)
{
    UnloadDriver();
    // DLL laden
    if(!driver_wrapper.Load(drivers::DriverType::Video, preference))
        return false;

    auto createVideoInstance = driver_wrapper.GetFunction<CreateVideoInstance_t>("CreateVideoInstance");
    auto freeVideoInstance = driver_wrapper.GetFunction<FreeVideoInstance_t>("FreeVideoInstance");
    RTTR_Assert(createVideoInstance && freeVideoInstance);

    videodriver = Handle(createVideoInstance(&WINDOWMANAGER), freeVideoInstance);
    return Initialize();
}

void VideoDriverWrapper::UnloadDriver()
{
    videodriver.reset();
    driver_wrapper.Unload();
    renderer_.reset();
}

/**
 *  Erstellt das Fenster.
 *
 *  @param[in] width  Breite des Fensters
 *  @param[in] height Höhe des Fensters
 *
 *  @return Bei Erfolg @p true ansonsten @p false
 */
bool VideoDriverWrapper::CreateScreen(const VideoMode size, const bool fullscreen)
{
    if(!videodriver)
    {
        s25util::fatal_error("No video driver selected!");
        return false;
    }

    if(!videodriver->CreateScreen(rttr::version::GetTitle(), size, fullscreen))
    {
        s25util::fatal_error("Could not create window!");
        return false;
    }

    // DriverWrapper Initialisieren
    // Extensions laden
    if(!LoadAllExtensions())
    {
        s25util::fatal_error("Failed to initialize the OpenGL context!");
        return false;
    }

    RenewViewport();

    // Buffer swappen um den leeren Buffer darzustellen
    SwapBuffers();

    // WindowManager informieren
    WINDOWMANAGER.Msg_ScreenResize(GetRenderSize());

    return true;
}

/**
 *  Verändert Auflösung, Fenster/Fullscreen
 *
 *  @param[in] screenWidth neue Breite des Fensters
 *  @param[in] screenHeight neue Höhe des Fensters
 *  @param[in] fullscreen Vollbild oder nicht
 *
 *  @return Bei Erfolg @p true ansonsten @p false
 */
bool VideoDriverWrapper::ResizeScreen(const VideoMode size, const bool fullscreen)
{
    if(!videodriver)
    {
        s25util::fatal_error("No video driver selected!");
        return false;
    }

    const bool result = videodriver->ResizeScreen(size, fullscreen);
#ifdef _WIN32
    if(!videodriver->IsFullscreen())
    {
        // We cannot change the size of a maximized window. So restore it here
        WINDOWPLACEMENT wp;
        wp.length = sizeof(WINDOWPLACEMENT);

        if(GetWindowPlacement((HWND)GetMapPointer(), &wp) && ((wp.showCmd & SW_MAXIMIZE) == SW_MAXIMIZE))
            ShowWindow((HWND)GetMapPointer(), SW_RESTORE);
    }
#endif
    WINDOWMANAGER.WindowResized();
    return result;
}

/**
 *  Zerstört den DriverWrapper-Bildschirm.
 */
bool VideoDriverWrapper::DestroyScreen()
{
    if(!videodriver)
    {
        s25util::fatal_error("No video driver selected!");
        return false;
    }

    LOG.write("Clearing textures: ");
    unsigned ladezeit = GetTickCount();
    CleanUp();
    LOG.write("Finished in %dms\n") % (GetTickCount() - ladezeit);

    // Videotreiber zurücksetzen
    videodriver->DestroyScreen();

    return true;
}

void VideoDriverWrapper::ShowErrorMessage(const std::string& title, const std::string& message)
{
    if(videodriver)
        videodriver->ShowErrorMessage(title, message);
}

void VideoDriverWrapper::setTargetFramerate(int target)
{
    frameLimiter_->setTargetFramerate(target);
    if(!setHwVSync(target == 0) && target == 0) // Fallback if no HW vsync but was requested
        frameLimiter_->setTargetFramerate(60);
}

unsigned VideoDriverWrapper::GetFPS() const
{
    return frameCtr_->getFrameRate();
}

/**
 *  Löscht alle herausgegebenen Texturen aus dem Speicher.
 */
void VideoDriverWrapper::CleanUp()
{
    if(!texture_list.empty())
    {
        glDeleteTextures(texture_list.size(), static_cast<const GLuint*>(texture_list.data()));
        texture_list.clear();
    }
}

unsigned VideoDriverWrapper::GenerateTexture()
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

void VideoDriverWrapper::BindTexture(unsigned t)
{
    if(t != texture_current)
    {
        texture_current = t;
        glBindTexture(GL_TEXTURE_2D, t);
    }
}

void VideoDriverWrapper::DeleteTexture(unsigned t)
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

KeyEvent VideoDriverWrapper::GetModKeyState() const
{
    if(videodriver)
        return videodriver->GetModKeyState();
    const KeyEvent ke = {KeyType::Invalid, 0, false, false, false};
    return ke;
}

void VideoDriverWrapper::SwapBuffers()
{
    if(!videodriver)
    {
        s25util::fatal_error("No video driver selected!");
        return;
    }
    frameLimiter_->sleepTillNextFrame(FrameCounter::clock::now());
    videodriver->SwapBuffers();
    FrameCounter::clock::time_point now = FrameCounter::clock::now();
    frameLimiter_->update(now);
    frameCtr_->update(now);
}

void VideoDriverWrapper::ClearScreen()
{
    glClear(GL_COLOR_BUFFER_BIT);
}

bool VideoDriverWrapper::Run()
{
    if(!videodriver)
    {
        s25util::fatal_error("No video driver selected!");
        return false;
    }

    return videodriver->MessageLoop();
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

/**
 *  Viewport (neu) setzen
 */
void VideoDriverWrapper::RenewViewport()
{
    if(!videodriver->IsOpenGL() || !renderer_)
        return;

    const Extent renderSize = getGuiScale().viewToScreen<Extent>(videodriver->GetRenderSize());
    const VideoMode windowSize = videodriver->GetWindowSize();

    // Set the viewport and scissor area to the entire window
    glViewport(0, 0, renderSize.x, renderSize.y);
    glScissor(0, 0, renderSize.x, renderSize.y);

    // Orthogonale Matrix erstellen
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

    // 0,0 should be top left corner
    glOrtho(0, windowSize.width, windowSize.height, 0, -100, 100);

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

    ClearScreen();
}

/**
 *  lädt die driverwrapper-extensions.
 */
bool VideoDriverWrapper::LoadAllExtensions()
{
    if(videodriver->IsOpenGL())
        renderer_ = std::make_unique<OpenGLRenderer>();
    else
        renderer_ = std::make_unique<DummyRenderer>();
    if(!renderer_->initOpenGL(videodriver->GetLoaderFunction()))
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

unsigned VideoDriverWrapper::GetTickCount()
{
    if(!videodriver)
        return (unsigned)time(nullptr);

    return (unsigned)videodriver->GetTickCount();
}

std::string VideoDriverWrapper::GetName() const
{
    const char* name = (videodriver) ? videodriver->GetName() : nullptr;
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

    return videodriver->GetLoaderFunction()(extension.c_str());
}

Position VideoDriverWrapper::GetMousePos() const
{
    if(!videodriver)
        return Position::Invalid();
    return videodriver->GetMousePos();
}

bool VideoDriverWrapper::IsLeftDown()
{
    if(!videodriver)
        return false;

    return videodriver->GetMouseStateL();
}

bool VideoDriverWrapper::IsRightDown()
{
    if(!videodriver)
        return false;

    return videodriver->GetMouseStateR();
}

void VideoDriverWrapper::SetMousePos(const Position& newPos)
{
    if(!videodriver || !enableMouseWarping)
        return;

    videodriver->SetMousePos(newPos);
}

/**
 *  Listet verfügbare Videomodi auf.
 */
void VideoDriverWrapper::ListVideoModes(std::vector<VideoMode>& video_modes) const
{
    if(!videodriver)
        return;

    videodriver->ListVideoModes(video_modes);
}

bool VideoDriverWrapper::HasVSync() const
{
    return wglSwapIntervalEXT != nullptr;
}

/**
 *  Gibt Pointer auf ein Fenster zurück (device-dependent!), HWND unter Windows.
 */
void* VideoDriverWrapper::GetMapPointer() const
{
    if(!videodriver)
        return nullptr;

    return videodriver->GetMapPointer();
}

VideoMode VideoDriverWrapper::GetWindowSize() const
{
    // Always return at least 800x600 even if real window is smaller
    VideoMode windowSize = videodriver->GetWindowSize();
    windowSize.width = std::max<unsigned>(800, windowSize.width);
    windowSize.height = std::max<unsigned>(600, windowSize.height);
    return windowSize;
}

Extent VideoDriverWrapper::GetRenderSize() const
{
    return videodriver->GetRenderSize();
}

bool VideoDriverWrapper::IsFullscreen() const
{
    return videodriver->IsFullscreen();
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
