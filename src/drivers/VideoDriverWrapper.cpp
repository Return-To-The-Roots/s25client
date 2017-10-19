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

#include "defines.h" // IWYU pragma: keep
#include "VideoDriverWrapper.h"
#include "ExtensionList.h"
#include "GlobalVars.h"
#include "RTTR_Version.h"
#include "Settings.h"
#include "WindowManager.h"
#include "driver/VideoInterface.h"
#include "helpers/roundToNextPow2.h"
#include "libutil/Log.h"
#include "libutil/error.h"
#include <algorithm>
#include <ctime>
#include <sstream>
#if !defined(NDEBUG) && defined(HAVE_MEMCHECK_H)
#include <valgrind/memcheck.h>
#endif

VideoDriverWrapper::VideoDriverWrapper() : videodriver(NULL), loadedFromDll(false), isOglEnabled_(false), texture_pos(0), texture_current(0)
{
    std::fill(texture_list.begin(), texture_list.end(), 0);
}

VideoDriverWrapper::~VideoDriverWrapper()
{
    CleanUp();
    UnloadDriver();
}

/**
 *  Wählt und lädt einen Displaytreiber.
 *
 *  @param[in] second @p true wenn 2te Chance aktiv, @p false wenn 2te Chance ausgeführt werden soll.
 *
 *  @return liefert @p true bei Erfolg, @p false bei Fehler
 */
bool VideoDriverWrapper::LoadDriver(IVideoDriver* existingDriver /*= NULL*/)
{
    loadedFromDll = existingDriver == NULL;
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

    isOglEnabled_ = videodriver->IsOpenGL();

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
    videodriver = NULL;
}

/**
 *  Erstellt das Fenster.
 *
 *  @param[in] width  Breite des Fensters
 *  @param[in] height Höhe des Fensters
 *
 *  @return Bei Erfolg @p true ansonsten @p false
 */
bool VideoDriverWrapper::CreateScreen(const unsigned short screen_width, const unsigned short screen_height, const bool fullscreen)
{
    if(!videodriver)
    {
        s25Util::fatal_error("Kein Videotreiber ausgewaehlt!\n");
        return false;
    }

    std::stringstream title;
    title << RTTR_Version::GetTitle() << " - v" << RTTR_Version::GetVersion() << "-" << RTTR_Version::GetShortRevision();

// Fenster erstellen
// On Windows it is necessary to open a windowed mode window at first and then resize it
#ifdef _WIN32
    // We need this doubled up here
    // - With WinAPI in the windowed case, otherwise the GL Viewport is set wrong (or something related, seems to be a bug in our WinAPI
    // implementation) - With SDL in the fullscreen case
    if(!videodriver->CreateScreen(title.str(), screen_width, screen_height, false))
    {
        s25Util::fatal_error("Erstellen des Fensters fehlgeschlagen!\n");
        return false;
    }
    if(!videodriver->ResizeScreen(screen_width, screen_height, fullscreen))
    {
        s25Util::fatal_error("Erstellen des Fensters fehlgeschlagen!\n");
        return false;
    }
    // Set this, as there is no message sent for the resize by the driver
    SETTINGS.video.fullscreen = VIDEODRIVER.IsFullscreen();
#else
    if(!videodriver->CreateScreen(title.str(), screen_width, screen_height, fullscreen))
    {
        s25Util::fatal_error("Erstellen des Fensters fehlgeschlagen!\n");
        return false;
    }
#endif

    // DriverWrapper Initialisieren
    if(!Initialize())
    {
        s25Util::fatal_error("Initialisieren des OpenGL-Kontexts fehlgeschlagen!\n");
        return false;
    }

    // WindowManager informieren
    WINDOWMANAGER.Msg_ScreenResize(Extent(screen_width, screen_height));

    // VSYNC ggf abschalten/einschalten
    if(GLOBALVARS.ext_swapcontrol)
        wglSwapIntervalEXT((SETTINGS.video.vsync == 1));

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
bool VideoDriverWrapper::ResizeScreen(const unsigned short screenWidth, const unsigned short screenHeight, const bool fullscreen)
{
    if(!videodriver)
    {
        s25Util::fatal_error("Kein Videotreiber ausgewaehlt!\n");
        return false;
    }

    const bool result = videodriver->ResizeScreen(screenWidth, screenHeight, fullscreen);
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

    RenewViewport();

    WINDOWMANAGER.Msg_ScreenResize(GetScreenSize());

    return result;
}

/**
 *  Zerstört den DriverWrapper-Bildschirm.
 */
bool VideoDriverWrapper::DestroyScreen()
{
    if(!videodriver)
    {
        s25Util::fatal_error("Kein Videotreiber ausgewaehlt!\n");
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
    if(isOglEnabled_)
        glDeleteTextures(texture_pos, (const GLuint*)&texture_list.front());

    std::fill(texture_list.begin(), texture_list.end(), 0);
    texture_pos = 0;
}

unsigned VideoDriverWrapper::GenerateTexture()
{
    if(!isOglEnabled_)
        return 0;

    if(texture_pos >= texture_list.size())
    {
        s25Util::fatal_error("texture-limit reached!!!!\n");
        return 0;
    }

    GLuint newTexture = 0;
    glGenTextures(1, &newTexture);
#if !defined(NDEBUG) && defined(HAVE_MEMCHECK_H)
    VALGRIND_MAKE_MEM_DEFINED(&newTexture, sizeof(newTexture));
#endif

    texture_list[texture_pos] = newTexture;

    return texture_list[texture_pos++];
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

bool VideoDriverWrapper::SwapBuffers()
{
    if(!videodriver)
    {
        s25Util::fatal_error("Kein Videotreiber ausgewaehlt!\n");
        return false;
    }

    return videodriver->SwapBuffers();
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
        s25Util::fatal_error("Kein Videotreiber ausgewaehlt!\n");
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

    RenewViewport();

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

    // Extensions laden
    if(!LoadAllExtensions())
        return false;

    ClearScreen();

    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);

    // Buffer swappen um den leeren Buffer darzustellen
    SwapBuffers();

    return true;
}

/**
 *  Viewport (neu) setzen
 */
void VideoDriverWrapper::RenewViewport()
{
    if(!videodriver->IsOpenGL())
        return;

    const unsigned short width = videodriver->GetScreenWidth();
    const unsigned short height = videodriver->GetScreenHeight();

    // Viewport mit widthxheight setzen
    glViewport(0, 0, width, height);
    glScissor(0, 0, width, height);

    // Orthogonale Matrix erstellen
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

    // 0,0 should be top left corner
    glOrtho(0, width, height, 0, -100, 100);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    ClearScreen();
}

/**
 *  lädt die driverwrapper-extensions.
 */
bool VideoDriverWrapper::LoadAllExtensions()
{
// auf VSync-Extension testen
#ifdef _WIN32
    if((GLOBALVARS.ext_swapcontrol = hasExtension("WGL_EXT_swap_control")))
    {
        if((wglSwapIntervalEXT = pto2ptf<PFNWGLSWAPINTERVALFARPROC>(loadExtension("wglSwapIntervalEXT"))) == NULL)
            GLOBALVARS.ext_swapcontrol = false;
    }
#else
    /*if((GLOBALVARS.ext_swapcontrol = hasExtension("GLX_SGI_swap_control")))
    {*/
    // fix for buggy video driver...
    GLOBALVARS.ext_swapcontrol = true;
    if((wglSwapIntervalEXT = pto2ptf<PFNWGLSWAPINTERVALFARPROC>(loadExtension("glXSwapIntervalSGI"))) == NULL)
        GLOBALVARS.ext_swapcontrol = false;
//}
#endif

    // auf VertexBufferObject-Extension testen
    if((GLOBALVARS.ext_vbo = hasExtension("GL_ARB_vertex_buffer_object")))
    {
#ifndef __APPLE__
        if((glBindBufferARB = pto2ptf<PFNGLBINDBUFFERARBPROC>(loadExtension("glBindBufferARB"))) == NULL)
            GLOBALVARS.ext_vbo = false;
        else if((glDeleteBuffersARB = pto2ptf<PFNGLDELETEBUFFERSARBPROC>(loadExtension("glDeleteBuffersARB"))) == NULL)
            GLOBALVARS.ext_vbo = false;
        else if((glGenBuffersARB = pto2ptf<PFNGLGENBUFFERSARBPROC>(loadExtension("glGenBuffersARB"))) == NULL)
            GLOBALVARS.ext_vbo = false;
        else if((glBufferDataARB = pto2ptf<PFNGLBUFFERDATAARBPROC>(loadExtension("glBufferDataARB"))) == NULL)
            GLOBALVARS.ext_vbo = false;
        else if((glBufferSubDataARB = pto2ptf<PFNGLBUFFERSUBDATAARBPROC>(loadExtension("glBufferSubDataARB"))) == NULL)
            GLOBALVARS.ext_vbo = false;
#endif // ! __APPLE__
    }

    return true;
}

unsigned VideoDriverWrapper::GetTickCount()
{
    if(!videodriver)
        return (unsigned)time(NULL);

    return (unsigned)videodriver->GetTickCount();
}

std::string VideoDriverWrapper::GetName() const
{
    return (videodriver) ? videodriver->GetName() : "";
}

/**
 *  lädt eine bestimmte DriverWrapper Extension-Funktion.
 *
 *  @param[in] extension Die Extension-Funktion
 *
 *  @return @p NULL bei Fehler, Adresse der gewünschten Funktion bei Erfolg.
 */
void* VideoDriverWrapper::loadExtension(const std::string& extension)
{
    if(!videodriver)
    {
        s25Util::fatal_error("Kein Videotreiber ausgewaehlt!\n");
        return (NULL);
    }

    return videodriver->GetFunction(extension.c_str());
}

int VideoDriverWrapper::GetMouseX() const
{
    return GetMousePos().x;
}

int VideoDriverWrapper::GetMouseY() const
{
    return GetMousePos().y;
}

Point<int> VideoDriverWrapper::GetMousePos() const
{
    if(!videodriver)
        return Point<int>::Invalid();
    Point<int> result;
    videodriver->GetMousePos(result.x, result.y);
    return result;
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

/**
 *  setzt die Mausposition
 */
void VideoDriverWrapper::SetMousePos(const int x, const int y)
{
    SetMousePos(Point<int>(x, y));
}

void VideoDriverWrapper::SetMousePos(const Point<int>& newPos)
{
    if(!videodriver || !SETTINGS.global.smartCursor)
        return;

    videodriver->SetMousePos(newPos.x, newPos.y);
}

/**
 *  Listet verfügbare Videomodi auf.
 */
void VideoDriverWrapper::ListVideoModes(std::vector<VideoMode>& video_modes) const
{
    if(!videodriver)
        return;

    // Standard-Modi hinzufügen
    video_modes.push_back(VideoMode(800, 600));
    video_modes.push_back(VideoMode(1024, 768));

    videodriver->ListVideoModes(video_modes);
}

/**
 *  Gibt Pointer auf ein Fenster zurück (device-dependent!), HWND unter Windows.
 */
void* VideoDriverWrapper::GetMapPointer() const
{
    if(!videodriver)
        return NULL;

    return videodriver->GetMapPointer();
}

unsigned short VideoDriverWrapper::GetScreenWidth() const
{
    return std::max<unsigned>(800u, videodriver->GetScreenWidth());
}

unsigned short VideoDriverWrapper::GetScreenHeight() const
{
    return std::max<unsigned>(600u, videodriver->GetScreenHeight());
}

Extent VideoDriverWrapper::GetScreenSize() const
{
    return Extent(GetScreenWidth(), GetScreenHeight());
}

bool VideoDriverWrapper::IsFullscreen() const
{
    return videodriver->IsFullscreen();
}
