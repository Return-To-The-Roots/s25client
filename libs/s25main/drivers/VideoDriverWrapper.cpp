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

#include "rttrDefines.h" // IWYU pragma: keep
#include "VideoDriverWrapper.h"
#include "FrameCounter.h"
#include "GlobalVars.h"
#include "RTTR_Version.h"
#include "Settings.h"
#include "WindowManager.h"
#include "driver/VideoInterface.h"
#include "helpers/roundToNextPow2.h"
#include "mygettext/mygettext.h"
#include "ogl/DummyRenderer.h"
#include "ogl/OpenGLRenderer.h"
#include "openglCfg.hpp"
#include "libutil/Log.h"
#include "libutil/error.h"
#include <ctime>
#include <glad/glad.h>
#include <sstream>
#if !defined(NDEBUG) && defined(HAVE_MEMCHECK_H)
#include <valgrind/memcheck.h>
#endif

// WGL_EXT_swap_control
#ifdef _WIN32
typedef BOOL(APIENTRY* PFNWGLSWAPINTERVALFARPROC)(int);
#else
typedef int (*PFNWGLSWAPINTERVALFARPROC)(int);
#endif

PFNWGLSWAPINTERVALFARPROC wglSwapIntervalEXT = nullptr;

VideoDriverWrapper::VideoDriverWrapper()
    : videodriver(nullptr), renderer_(nullptr), loadedFromDll(false), isOglEnabled_(false), texture_current(0)
{}

VideoDriverWrapper::~VideoDriverWrapper()
{
    CleanUp();
    UnloadDriver();
}

bool VideoDriverWrapper::LoadDriver(IVideoDriver* existingDriver /*= nullptr*/)
{
    loadedFromDll = existingDriver == nullptr;
    if(!existingDriver)
    {
        // DLL laden
        if(!driver_wrapper.Load(DriverWrapper::DT_VIDEO, SETTINGS.driver.video))
            return false;

        PDRIVER_CREATEVIDEOINSTANCE CreateVideoInstance =
          pto2ptf<PDRIVER_CREATEVIDEOINSTANCE>(driver_wrapper.GetDLLFunction("CreateVideoInstance"));

        // Instanz erzeugen
        videodriver = CreateVideoInstance(&WINDOWMANAGER);
        if(!videodriver)
        {
            UnloadDriver();
            return false;
        }
    } else
        videodriver = existingDriver;

    if(!videodriver->Initialize())
    {
        UnloadDriver();
        return false;
    }

    LOG.write(_("Loaded video driver \"%1%\"\n")) % GetName();

    isOglEnabled_ = videodriver->IsOpenGL();
    if(isOglEnabled_)
        renderer_ = std::make_unique<OpenGLRenderer>();
    else
        renderer_ = std::make_unique<DummyRenderer>();
    frameCtr_ = std::make_unique<FrameCounter>();
    frameLimiter_ = std::make_unique<FrameLimiter>();

    return true;
}

void VideoDriverWrapper::UnloadDriver()
{
    if(loadedFromDll)
    {
        PDRIVER_FREEVIDEOINSTANCE FreeVideoInstance =
          pto2ptf<PDRIVER_FREEVIDEOINSTANCE>(driver_wrapper.GetDLLFunction("FreeVideoInstance"));
        if(FreeVideoInstance)
            FreeVideoInstance(videodriver);
        driver_wrapper.Unload();
    } else
        delete videodriver;
    videodriver = nullptr;
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
        s25util::fatal_error("No video driver selected!\n");
        return false;
    }

    std::stringstream title;
    title << RTTR_Version::GetTitle() << " - " << RTTR_Version::GetReadableVersion();

    // Fenster erstellen
    if(!videodriver->CreateScreen(title.str(), size, fullscreen))
    {
        s25util::fatal_error("Could not create window!\n");
        return false;
    }

    // DriverWrapper Initialisieren
    if(!Initialize())
    {
        s25util::fatal_error("Failed to initialize the OpenGL context!\n");
        return false;
    }

    // WindowManager informieren
    WINDOWMANAGER.Msg_ScreenResize(GetRenderSize());

    // VSYNC ggf abschalten/einschalten
    setHwVSync(SETTINGS.video.vsync == 0);

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
        s25util::fatal_error("No video driver selected!\n");
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
        s25util::fatal_error("No video driver selected!\n");
        return false;
    }

    // Texturen aufräumen
    LOG.write("Saeubere Texturespeicher: ");
    unsigned ladezeit = GetTickCount();
    CleanUp();
    LOG.write("fertig (nach %dms)\n") % (GetTickCount() - ladezeit);

    // Videotreiber zurücksetzen
    videodriver->DestroyScreen();

    return true;
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
 *  prüft, ob eine bestimmte Extension existiert.
 *
 *  @param[in] extension Die zu suchende Extension
 *
 *  @return Bei Erfolg @p true ansonsten @p false
 */
bool VideoDriverWrapper::hasExtension(const std::string& extension)
{
    // Extension mit Leerzeichen gibts nich
    if(!isOglEnabled_ || extension.empty() || extension.find(' ') != std::string::npos)
        return false;

    // ermittle Extensions String
    const std::string extensions = reinterpret_cast<const char*>(glGetString(GL_EXTENSIONS));

    // such nach einer exakten Kopie des Extensions Strings
    size_t curOffset = 0;
    do
    {
        size_t curPos = extensions.find(extension, curOffset);
        if(curPos == std::string::npos)
            break;

        size_t endPos = curPos + extension.length();
        if(curPos == 0 || extensions[curPos - 1] == ' ')
        {
            if(endPos == extensions.length() || extensions[endPos] == ' ')
                return true;
        }
        curOffset = endPos;
    } while(curOffset < extensions.length());

    return false;
}

/**
 *  Löscht alle herausgegebenen Texturen aus dem Speicher.
 */
void VideoDriverWrapper::CleanUp()
{
    if(isOglEnabled_ && !texture_list.empty())
        glDeleteTextures(texture_list.size(), (const GLuint*)&texture_list.front());

    texture_list.clear();
}

unsigned VideoDriverWrapper::GenerateTexture()
{
    if(!isOglEnabled_)
        return 0;

    GLuint newTexture = 0;
    glGenTextures(1, &newTexture);
#if !defined(NDEBUG) && defined(HAVE_MEMCHECK_H)
    VALGRIND_MAKE_MEM_DEFINED(&newTexture, sizeof(newTexture));
#endif

    static_assert(sizeof(newTexture) == sizeof(texture_list[0]), "Unexpected texture size");
    texture_list.push_back(newTexture);
    return texture_list.back();
}

void VideoDriverWrapper::BindTexture(unsigned t)
{
    if(t != texture_current)
    {
        texture_current = t;
        if(isOglEnabled_)
            glBindTexture(GL_TEXTURE_2D, t);
    }
}

void VideoDriverWrapper::DeleteTexture(unsigned t)
{
    if(t == texture_current)
        texture_current = 0;
    if(isOglEnabled_)
        glDeleteTextures(1, &t);
}

KeyEvent VideoDriverWrapper::GetModKeyState() const
{
    if(videodriver)
        return videodriver->GetModKeyState();
    const KeyEvent ke = {KT_INVALID, 0, false, false, false};
    return ke;
}

void VideoDriverWrapper::SwapBuffers()
{
    if(!videodriver)
    {
        s25util::fatal_error("No video driver selected!\n");
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
    if(isOglEnabled_)
        glClear(GL_COLOR_BUFFER_BIT);
}

bool VideoDriverWrapper::Run()
{
    if(!videodriver)
    {
        s25util::fatal_error("No video driver selected!\n");
        return false;
    }

    return videodriver->MessageLoop();
}

Extent VideoDriverWrapper::calcPreferredTextureSize(const Extent& minSize) const
{
    return Extent(helpers::roundToNextPowerOfTwo(minSize.x), helpers::roundToNextPowerOfTwo(minSize.y));
}

bool VideoDriverWrapper::Initialize()
{
    if(!isOglEnabled_)
        return true;

    // Extensions laden
    if(!LoadAllExtensions())
        return false;

    RenewViewport();

    // Buffer swappen um den leeren Buffer darzustellen
    SwapBuffers();

    return true;
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
    if(!videodriver->IsOpenGL())
        return;

    const Extent renderSize = videodriver->GetRenderSize();

    // Viewport mit widthxheight setzen
    glViewport(0, 0, renderSize.x, renderSize.y);
    glScissor(0, 0, renderSize.x, renderSize.y);

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

    ClearScreen();
}

/**
 *  lädt die driverwrapper-extensions.
 */
bool VideoDriverWrapper::LoadAllExtensions()
{
#if RTTR_OPENGL_ES
    if(!gladLoadGLES2Loader(videodriver->GetLoaderFunction()))
    {
        return false;
    }
#else
    if(!gladLoadGLLoader(videodriver->GetLoaderFunction()))
    {
        return false;
    }
#endif
    LOG.write(_("OpenGL %1%.%2% supported\n")) % GLVersion.major % GLVersion.minor;
    if(GLVersion.major < RTTR_OGL_MAJOR || (GLVersion.major == RTTR_OGL_MAJOR && GLVersion.minor < RTTR_OGL_MINOR))
    {
        boost::format errorMsg(_("OpenGL %1% %2%.%3% is not supported. Try updating your GPU drivers or hardware!"));
        errorMsg % (RTTR_OGL_ES ? "ES" : "") % RTTR_OGL_MAJOR % RTTR_OGL_MINOR;
        s25util::fatal_error(errorMsg.str());
        return false;
    }
// auf VSync-Extension testen
#ifdef _WIN32
    if(hasExtension("WGL_EXT_swap_control"))
        wglSwapIntervalEXT = pto2ptf<PFNWGLSWAPINTERVALFARPROC>(loadExtension("wglSwapIntervalEXT"));
#else
    wglSwapIntervalEXT = pto2ptf<PFNWGLSWAPINTERVALFARPROC>(loadExtension("glXSwapIntervalSGI"));
#endif
    GLOBALVARS.hasVSync = wglSwapIntervalEXT != nullptr;

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
        s25util::fatal_error("Kein Videotreiber ausgewaehlt!\n");
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
    if(!videodriver || !SETTINGS.global.smartCursor)
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
