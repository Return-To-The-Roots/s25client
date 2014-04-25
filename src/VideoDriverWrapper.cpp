// $Id: VideoDriverWrapper.cpp 9357 2014-04-25 15:35:25Z FloSoft $
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
#include "VideoDriverWrapper.h"

#include "Settings.h"
#include "GlobalVars.h"
#include "ExtensionList.h"
#include "../driver/src/VideoInterface.h"

#include "WindowManager.h"
#include "files.h"

///////////////////////////////////////////////////////////////////////////////
// Makros / Defines
#if defined _WIN32 && defined _DEBUG && defined _MSC_VER
#define new new(_NORMAL_BLOCK, THIS_FILE, __LINE__)
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

///////////////////////////////////////////////////////////////////////////////
/**
 *  Konstruktor der DriverWrapper Klasse.
 *
 *  @author FloSoft
 */
VideoDriverWrapper::VideoDriverWrapper() :  videodriver(NULL), texture_pos(0), texture_current(0)
{
    memset(texture_list, 0, sizeof(unsigned int) * 100000);
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  Destruktor der DriverWrapper Klasse.
 *
 *  @author FloSoft
 */
VideoDriverWrapper::~VideoDriverWrapper()
{
    CleanUp();
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  Wählt und lädt einen Displaytreiber.
 *
 *  @param[in] second @p true wenn 2te Chance aktiv, @p false wenn 2te Chance ausgeführt werden soll.
 *
 *  @return liefert @p true bei Erfolg, @p false bei Fehler
 *
 *  @author FloSoft
 */
bool VideoDriverWrapper::LoadDriver(void)
{
#ifdef _WIN32
    // unter Windows standardmäßig WinAPI präfarieren
    if(SETTINGS.driver.video == "")
        SETTINGS.driver.video = "(WinAPI) OpenGL via the glorious WinAPI";
#endif

    // DLL laden
    if(!driver_wrapper.Load(DriverWrapper::DT_VIDEO, SETTINGS.driver.video))
        return false;

    PDRIVER_CREATEVIDEOINSTANCE CreateVideoInstance;

    CreateVideoInstance = pto2ptf<PDRIVER_CREATEVIDEOINSTANCE>(driver_wrapper.GetDLLFunction("CreateVideoInstance"));

    // Instanz erzeugen
    if(!(videodriver = CreateVideoInstance(&WindowManager::inst())))
        return false;

    if(!videodriver->Initialize())
        return false;

    return true;
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  Erstellt das Fenster.
 *
 *  @param[in] width  Breite des Fensters
 *  @param[in] height Höhe des Fensters
 *
 *  @return Bei Erfolg @p true ansonsten @p false
 *
 *  @author FloSoft
 */
bool VideoDriverWrapper::CreateScreen(const unsigned short screen_width, const unsigned short screen_height, const bool fullscreen)
{
    if(videodriver == NULL)
    {
        fatal_error("Kein Videotreiber ausgewaehlt!\n");
        return false;
    }

    // Fenster erstellen
    // On Windows it is necessary to open a windowed mode window at first and then resize it
#ifdef _WIN32
    // We need this doubled up here
    // - With WinAPI in the windowed case, otherwise the GL Viewport is set wrong (or something related, seems to be a bug in our WinAPI implementation)
    // - With SDL in the fullscreen case
    if(!videodriver->CreateScreen(800, 600, false))
    {
        fatal_error("Erstellen des Fensters fehlgeschlagen!\n");
        return false;
    }
    if(!videodriver->ResizeScreen(screen_width, screen_height, fullscreen))
    {
        fatal_error("Erstellen des Fensters fehlgeschlagen!\n");
        return false;
    }
#else
    if(!videodriver->CreateScreen(screen_width, screen_height, fullscreen))
    {
        fatal_error("Erstellen des Fensters fehlgeschlagen!\n");
        return false;
    }
#endif

    // DriverWrapper Initialisieren
    if(!Initialize())
    {
        fatal_error("Initialisieren des OpenGL-Kontexts fehlgeschlagen!\n");
        return false;
    }



    // WindowManager informieren
    WindowManager::inst().Msg_ScreenResize(screen_width, screen_height);

    // VSYNC ggf abschalten/einschalten
    if(GLOBALVARS.ext_swapcontrol)
        wglSwapIntervalEXT((SETTINGS.video.vsync == 1));

    return true;
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  Verändert Auflösung, Fenster/Fullscreen
 *
 *  @param[in] screenWidth neue Breite des Fensters
 *  @param[in] screenHeight neue Höhe des Fensters
 *  @param[in] fullscreen Vollbild oder nicht
 *
 *  @return Bei Erfolg @p true ansonsten @p false
 *
 *  @author FloSoft
 */
bool VideoDriverWrapper::ResizeScreen(const unsigned short screenWidth, const unsigned short screenHeight, const bool fullscreen)
{
    if(videodriver == NULL)
    {
        fatal_error("Kein Videotreiber ausgewaehlt!\n");
        return false;
    }

    const bool result = videodriver->ResizeScreen(screenWidth, screenHeight, fullscreen);

    RenewViewport();

    WindowManager::inst().Msg_ScreenResize(screenWidth, screenHeight);

    return result;
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  Zerstört den DriverWrapper-Bildschirm.
 *
 *  @author FloSoft
 */
bool VideoDriverWrapper::DestroyScreen()
{
    if(videodriver == NULL)
    {
        fatal_error("Kein Videotreiber ausgewaehlt!\n");
        return false;
    }

    // Texturen aufräumen
    LOG.lprintf("Saeubere Texturespeicher: ");
    unsigned int ladezeit = GetTickCount();
    CleanUp();
    LOG.lprintf("fertig (nach %dms)\n", GetTickCount() - ladezeit);

    // Videotreiber zurücksetzen
    videodriver->DestroyScreen();

    return true;
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  prüft, ob eine bestimmte Extension existiert.
 *
 *  @param[in] extension Die zu suchende Extension
 *
 *  @return Bei Erfolg @p true ansonsten @p false
 *
 *  @author FloSoft
 */
bool VideoDriverWrapper::hasExtension(const char* extension)
{
    const unsigned char* extensions = NULL;

    const unsigned char* start;
    unsigned char* position, *ende;

    // Extension mit Leerzeichen gibts nich
    position = (unsigned char*)strchr(extension, ' ');
    if( position || *extension == '\0' )
        return false;

    // ermittle Extensions String
    extensions = glGetString( GL_EXTENSIONS );

    // such nach einer exakten Kopie des Extensions Strings
    start = extensions;
    for(;;)
    {
        position = (unsigned char*)strstr( (const char*)start, extension );
        if( !position )
            break;

        ende = position + strlen( extension );
        if( position == start || *( position - 1 ) == ' ' )
        {
            if( *ende == ' ' || *ende == '\0' )
                return true;
        }
        start = ende;
    }

    return false;
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  Löscht alle herausgegebenen Texturen aus dem Speicher.
 *
 *  @author FloSoft
 */
void VideoDriverWrapper::CleanUp()
{
    glDeleteTextures(texture_pos, (const GLuint*)texture_list);

    memset(texture_list, 0, sizeof(unsigned int)*texture_pos);
    texture_pos = 0;
}

///////////////////////////////////////////////////////////////////////////////
/**
 *
 *
 *  @author FloSoft
 */
unsigned int VideoDriverWrapper::GenerateTexture()
{
    if(texture_pos >= 100000)
    {
        fatal_error("100000 texture-limit reached!!!!\n");
        return 0;
    }

    glGenTextures(1, (GLuint*)&texture_list[texture_pos]);

    return texture_list[texture_pos++];
}

///////////////////////////////////////////////////////////////////////////////
/**
 *
 *
 *  @author FloSoft
 */
bool VideoDriverWrapper::SwapBuffers()
{
    if(videodriver == NULL)
    {
        fatal_error("Kein Videotreiber ausgewaehlt!\n");
        return false;
    }

    return videodriver->SwapBuffers();
}

///////////////////////////////////////////////////////////////////////////////
/**
 *
 *
 *  @author FloSoft
 */
bool VideoDriverWrapper::Run()
{
    if(videodriver == NULL)
    {
        fatal_error("Kein Videotreiber ausgewaehlt!\n");
        return false;
    }

    return videodriver->MessageLoop();
}

///////////////////////////////////////////////////////////////////////////////
/**
 *
 *
 *  @author FloSoft
 */
bool VideoDriverWrapper::Initialize()
{
    RenewViewport();

    // Depthbuffer und Colorbuffer einstellen
    glClearColor(0.0, 0.0, 0.0, 0.5);

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

    // Puffer leeren
    glClear(GL_COLOR_BUFFER_BIT);

    // Buffer swappen um den leeren Buffer darzustellen
    SwapBuffers();

    return true;
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  Viewport (neu) setzen
 *
 *  @author FloSoft
 */
void VideoDriverWrapper::RenewViewport(bool onlyRenew)
{
    const unsigned short width  = videodriver->GetScreenWidth();
    const unsigned short height = videodriver->GetScreenHeight();

    // Viewport mit widthxheight setzen
    glViewport(0, 0, width, height);
    glScissor(0, 0, width, height);

    // Orthogonale Matrix erstellen
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

    // ... und laden
    glOrtho(0, width, 0, height, -100, 100);

    // 0; 0 soll obere linke Ecke sein
    glRotated(180, 1, 0, 0);
    glTranslated(0, -videodriver->GetScreenHeight(), 0);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  lädt die driverwrapper-extensions.
 *
 *  @author flosoft
 */
bool VideoDriverWrapper::LoadAllExtensions()
{
    // auf VSync-Extension testen
#ifdef _WIN32
    if((GLOBALVARS.ext_swapcontrol = hasExtension("WGL_EXT_swap_control")))
    {
        if( (wglSwapIntervalEXT = pto2ptf<PFNWGLSWAPINTERVALFARPROC>(loadExtension("wglSwapIntervalEXT"))) == NULL)
            GLOBALVARS.ext_swapcontrol = false;
    }
#else
    /*if((GLOBALVARS.ext_swapcontrol = hasExtension("GLX_SGI_swap_control")))
    {*/
    // fix for buggy video driver...
    GLOBALVARS.ext_swapcontrol = true;
    if( (wglSwapIntervalEXT = pto2ptf<PFNWGLSWAPINTERVALFARPROC>(loadExtension("glXSwapIntervalSGI"))) == NULL)
        GLOBALVARS.ext_swapcontrol = false;
    //}
#endif

    // auf VertexBufferObject-Extension testen
    if((GLOBALVARS.ext_vbo = hasExtension("GL_ARB_vertex_buffer_object")))
    {
#ifndef __APPLE__
        if ( (glBindBufferARB = pto2ptf<PFNGLBINDBUFFERARBPROC>(loadExtension("glBindBufferARB"))) == NULL)
            GLOBALVARS.ext_vbo = false;
        else if ( (glDeleteBuffersARB = pto2ptf<PFNGLDELETEBUFFERSARBPROC>(loadExtension("glDeleteBuffersARB"))) == NULL)
            GLOBALVARS.ext_vbo = false;
        else if ( (glGenBuffersARB = pto2ptf<PFNGLGENBUFFERSARBPROC>(loadExtension("glGenBuffersARB"))) == NULL)
            GLOBALVARS.ext_vbo = false;
        else if ( (glBufferDataARB = pto2ptf<PFNGLBUFFERDATAARBPROC>(loadExtension("glBufferDataARB"))) == NULL)
            GLOBALVARS.ext_vbo = false;
        else if ( (glBufferSubDataARB = pto2ptf<PFNGLBUFFERSUBDATAARBPROC>(loadExtension("glBufferSubDataARB"))) == NULL)
            GLOBALVARS.ext_vbo = false;
#endif // ! __APPLE__
    }

    return true;
}

///////////////////////////////////////////////////////////////////////////////
/**
 *
 *
 *  @author FloSoft
 */
unsigned int VideoDriverWrapper::GetTickCount()
{
    if(videodriver == NULL)
        return (unsigned int)time(NULL);

    return (unsigned int)videodriver->GetTickCount();
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  lädt eine bestimmte DriverWrapper Extension-Funktion.
 *
 *  @param[in] extension Die Extension-Funktion
 *
 *  @return @p NULL bei Fehler, Adresse der gewünschten Funktion bei Erfolg.
 *
 *  @author FloSoft
 */
void* VideoDriverWrapper::loadExtension(const char* extension)
{
    if (videodriver == NULL)
    {
        fatal_error("Kein Videotreiber ausgewaehlt!\n");
        return(NULL);
    }

    return videodriver->GetFunction(extension);
}

///////////////////////////////////////////////////////////////////////////////
/**
 *
 *
 *  @author FloSoft
 */
int VideoDriverWrapper::GetMouseX()
{
    if(videodriver == NULL)
        return 0;

    return videodriver->GetMousePosX();
}

///////////////////////////////////////////////////////////////////////////////
/**
 *
 *
 *  @author FloSoft
 */
int VideoDriverWrapper::GetMouseY()
{
    if(videodriver == NULL)
        return 0;

    return videodriver->GetMousePosY();
}

///////////////////////////////////////////////////////////////////////////////
/**
 *
 *
 *  @author FloSoft
 */
bool VideoDriverWrapper::IsLeftDown()
{
    if(videodriver == NULL)
        return false;

    return videodriver->GetMouseStateL();
}

///////////////////////////////////////////////////////////////////////////////
/**
 *
 *
 *  @author FloSoft
 */
bool VideoDriverWrapper::IsRightDown()
{
    if(videodriver == NULL)
        return false;

    return videodriver->GetMouseStateR();
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  setzt die Mausposition
 *
 *  @author FloSoft
 */
void VideoDriverWrapper::SetMousePos(const int x, const int y)
{
    if(videodriver == NULL)
        return;

    videodriver->SetMousePos(x, y);
}


///////////////////////////////////////////////////////////////////////////////
/**
 *  Listet verfügbare Videomodi auf.
 *
 *  @author OLiver
 */
void VideoDriverWrapper::ListVideoModes(std::vector<VideoDriver::VideoMode>& video_modes) const
{
    if(videodriver == NULL)
        return;

    // Standard-Modi hinzufügen
    VideoDriver::VideoMode vm800  = {  800, 600 };
    VideoDriver::VideoMode vm1024 = { 1024, 768 };

    video_modes.push_back(vm800);
    video_modes.push_back(vm1024);

    videodriver->ListVideoModes(video_modes);
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  Gibt Pointer auf ein Fenster zurück (device-dependent!), HWND unter Windows.
 *
 *  @author OLiver
 */
void* VideoDriverWrapper::GetWindowPointer() const
{
    if(videodriver == NULL)
        return NULL;

    return videodriver->GetWindowPointer();
}
