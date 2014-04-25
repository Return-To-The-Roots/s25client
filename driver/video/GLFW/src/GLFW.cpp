// $Id: GLFW.cpp 9357 2014-04-25 15:35:25Z FloSoft $
//
// Copyright (c) 2005-2009 Settlers Freaks (sf-team at siedler25.org)
//
// This file is part of Siedler II.5 RTTR.
//
// Siedler II.5 RTTR is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 2 of the License, or
// (at your option) any later version.
//
// Siedler II.5 RTTR is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with Siedler II.5 RTTR. If not, see <http://www.gnu.org/licenses/>.

///////////////////////////////////////////////////////////////////////////////
// Header
#include "main.h"
#include "GLFW.h"
#include <VideoInterface.h>
#include <build_version.h>

///////////////////////////////////////////////////////////////////////////////
// Makros / Defines
#if defined _WIN32 && defined _DEBUG && defined _MSC_VER
#define new new(_NORMAL_BLOCK, THIS_FILE, __LINE__)
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

///////////////////////////////////////////////////////////////////////////////
/**
 *  Instanzierungsfunktion von @p VideoGLFW.
 *
 *  @param[in] CallBack DriverCallback für Rückmeldungen.
 *
 *  @return liefert eine Instanz des jeweiligen Treibers
 *
 *  @author FloSoft
 */
DRIVERDLLAPI VideoDriver* CreateVideoInstance(VideoDriverLoaderInterface* CallBack)
{
    return new VideoGLFW(CallBack);
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  Namensfunktion von @p VideoGLFW.
 *
 *  @return liefert den Namen des Treibers.
 *
 *  @author OLiver
 */
DRIVERDLLAPI const char* GetDriverName(void)
{
    return "(GLFW) OpenGL via GL-FrameWork-Library";
}

///////////////////////////////////////////////////////////////////////////////
/** @class VideoGLFW
 *
 *  Klasse für den GL-Framework Videotreiber.
 *
 *  @author FloSoft
 */

///////////////////////////////////////////////////////////////////////////////
/**
 *  Zeiger auf die aktuelle Instanz.
 *
 *  @author FloSoft
 */
static VideoGLFW* pVideoGLFW = NULL;

///////////////////////////////////////////////////////////////////////////////
/**
 *  Konstruktor von @p VideoGLFW.
 *
 *  @param[in] CallBack DriverCallback für Rückmeldungen.
 *
 *  @author FloSoft
 */
VideoGLFW::VideoGLFW(VideoDriverLoaderInterface* CallBack) : VideoDriver(CallBack), mouse_l(false), mouse_r(false), libGL(NULL)
{
    pVideoGLFW = this;
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  Destruktor von @p VideoGLFW.
 *
 *  @author FloSoft
 */
VideoGLFW::~VideoGLFW(void)
{
    pVideoGLFW = NULL;
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  Treiberinitialisierungsfunktion.
 *
 *  @return @p true bei Erfolg, @p false bei Fehler
 *
 *  @author FloSoft
 */
bool VideoGLFW::Initialize(void)
{
    if(CallBack == NULL)
        return false;

    // GL-Framework initialisieren
    initialized = glfwInit();

    // automatisches Eventabrufen deaktivieren
    glfwDisable(GLFW_AUTO_POLL_EVENTS);

    libGL = dlopen("/usr/lib/libGL.so", RTLD_LAZY);
    if(libGL == NULL)
        libGL = dlopen("/usr/lib/libGL.so.1", RTLD_LAZY);

    return initialized;
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  Treiberaufräumfunktion.
 *
 *  @author FloSoft
 */
void VideoGLFW::CleanUp(void)
{
    // Fenster zerstören
    DestroyScreen();

    // GL-Framework aufräumen
    glfwTerminate();

    // nun sind wir nicht mehr initalisiert
    initialized = false;

    if(libGL != NULL)
        dlclose(libGL);
    libGL = NULL;
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
 *  @bug Hardwarecursor ist bei Fenstermodus sichtbar,
 *       Cursor deaktivieren ist fehlerhaft
 *
 *  @author FloSoft
 */
bool VideoGLFW::CreateScreen(unsigned short width, unsigned short height, bool fullscreen)
{
    char title[512];

    if(!initialized)
        return false;

    // Fenster erstellen
    if(!glfwOpenWindow(width, height, 8, 8, 8, 8, 32, 0, fullscreen ? GLFW_FULLSCREEN : GLFW_WINDOW))
        return false;

    // Titel setzen
    sprintf(title, "%s - v%s-%s", GetWindowTitle(), GetWindowVersion(), GetWindowRevision());
    glfwSetWindowTitle(title);

    // Callbacks setzen
    glfwSetMousePosCallback(OnMouseMove);
    glfwSetMouseButtonCallback(OnMouseButton);
    glfwSetKeyCallback(OnKeyAction);

    memset(keyboard, false, sizeof(bool) * 512);
    this->screenWidth  = width;
    this->screenHeight = height;
    this->fullscreen = fullscreen;

    // buggy im Fenstermodus
    if(fullscreen)
        glfwDisable(GLFW_MOUSE_CURSOR);

    glfwEnable(GLFW_KEY_REPEAT);
    glfwEnable(GLFW_SYSTEM_KEYS);

    return true;
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
 *  @todo Vollbildmodus ggf. wechseln
 *
 *  @author FloSoft
 */
bool VideoGLFW::ResizeScreen(unsigned short* width, unsigned short* height, bool fullscreen)
{
    int w, h;

    if(!initialized)
        return false;

    // Falls fenster nicht existiert, neu erstellen
    if(glfwGetWindowParam(GLFW_OPENED) == GL_FALSE)
        return CreateScreen(*width, *height, fullscreen);

    // Fenstergröße setzen
    glfwSetWindowSize(*width, *height);

    glfwPollEvents();

    // und abrufen
    glfwGetWindowSize(&w, &h);
    *width = w;
    *height = h;


    this->screenWidth  = *width;
    this->screenHeight = *height;
    this->fullscreen = fullscreen;

    return true;
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  Schliesst das Fenster.
 *
 *  @author FloSoft
 */
void VideoGLFW::DestroyScreen(void)
{
    // Fenster schliessen
    glfwCloseWindow();

    fullscreen = false;
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  Wechselt die OpenGL-Puffer.
 *
 *  @return @p true bei Erfolg, @p false bei Fehler
 *
 *  @author FloSoft
 */
bool VideoGLFW::SwapBuffers(void)
{
    // Puffer wechseln
    glfwSwapBuffers();

    return true;
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  Die Nachrichtenschleife.
 *
 *  @return @p true bei Erfolg, @p false bei Fehler
 *
 *  @author FloSoft
 */
bool VideoGLFW::MessageLoop(void)
{
    // Events abrufen
    glfwPollEvents();

    // Wenn das Fenster geschlossen wurde, beenden
    return (glfwGetWindowParam(GLFW_OPENED) == GL_TRUE);
}


///////////////////////////////////////////////////////////////////////////////
/**
 *  Funktion zum Auslesen des TickCounts.
 *
 *  @return liefert den TickCount
 *
 *  @author FloSoft
 */
unsigned long VideoGLFW::GetTickCount(void) const
{
    return (unsigned long)(glfwGetTime() * 1000.0);
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
void* VideoGLFW::GetFunction(const char* function) const
{
    if(libGL == NULL)
        return NULL;
    return dlsym(libGL, function);
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
void VideoGLFW::SetMousePos(int x, int y)
{
    mouse_xy.x = x;
    mouse_xy.y = y;
    glfwSetMousePos(x, y);
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  Funktion zum Setzen der X-Koordinate der Maus.
 *
 *  @param[in] x X-Koordinate
 *
 *  @author FloSoft
 */
void VideoGLFW::SetMousePosX(int x)
{
    SetMousePos(x, mouse_xy.y);
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  Funktion zum Setzen der Y-Koordinate der Maus.
 *
 *  @param[in] y Y-Koordinate
 *
 *  @author FloSoft
 */
void VideoGLFW::SetMousePosY(int y)
{
    SetMousePos(mouse_xy.x, y);
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  Callbackfunktion des GL-Frameworks um Mausbewegungen abzufangen.
 *
 *  @param[in] x X-Koordinate
 *  @param[in] y Y-Koordinate
 *
 *  @author FloSoft
 */
void GLFWCALL VideoGLFW::OnMouseMove(int x, int y)
{
    if(pVideoGLFW == NULL)
        return;

    pVideoGLFW->mouse_xy.x = x;
    pVideoGLFW->mouse_xy.y = y;
    pVideoGLFW->CallBack->Msg_MouseMove(pVideoGLFW->mouse_xy);
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  Callbackfunktion des GL-Frameworks um Mausklicks abzufangen.
 *
 *  @param[in] button Maustaste
 *  @param[in] action Aktion der Taste
 *
 *  @author FloSoft
 */
void GLFWCALL VideoGLFW::OnMouseButton(int button, int action)
{
    if(pVideoGLFW == NULL)
        return;

    switch(button)
    {
        case GLFW_MOUSE_BUTTON_LEFT:    {   pVideoGLFW->mouse_l = (action == GLFW_PRESS);   } break;
        case GLFW_MOUSE_BUTTON_RIGHT:   {   pVideoGLFW->mouse_r = (action == GLFW_PRESS);   } break;
    }

    if(pVideoGLFW->mouse_l && !pVideoGLFW->mouse_xy.ldown)
    {
        pVideoGLFW->mouse_xy.ldown = pVideoGLFW->mouse_l;
        pVideoGLFW->CallBack->Msg_LeftDown(pVideoGLFW->mouse_xy);
    }
    else if(!pVideoGLFW->mouse_l && pVideoGLFW->mouse_xy.ldown)
    {
        pVideoGLFW->mouse_xy.ldown = pVideoGLFW->mouse_l;
        pVideoGLFW->CallBack->Msg_LeftUp(pVideoGLFW->mouse_xy);
    }

    if(pVideoGLFW->mouse_r && !pVideoGLFW->mouse_xy.rdown)
    {
        pVideoGLFW->mouse_xy.rdown = pVideoGLFW->mouse_r;
        pVideoGLFW->CallBack->Msg_RightDown(pVideoGLFW->mouse_xy);
    }
    else if(!pVideoGLFW->mouse_r && pVideoGLFW->mouse_xy.rdown)
    {
        pVideoGLFW->mouse_xy.rdown = pVideoGLFW->mouse_r;
        pVideoGLFW->CallBack->Msg_RightUp(pVideoGLFW->mouse_xy);
    }
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  Tastenwandlungsfunktion.
 *
 *  @param[in] key    Taste
 *
 *  @return ggf. veränderten @p key
 *
 *  @author FloSoft
 */
unsigned char VideoGLFW::TranslateKey(unsigned char key)
{
    if(keyboard[GLFW_KEY_LSHIFT] == true || keyboard[GLFW_KEY_RSHIFT] == true)
    {
        key = toupper(key);
        //printf("u %d, %c\n", key, key);
        switch(key)
        {
            case '1': key = '!'; break;
            case '2': key = '\"'; break;
            case '3': key = 21; break; // paragraph
            case '4': key = '$'; break;
            case '5': key = '%'; break;
            case '6': key = '&'; break;
            case '7': key = '/'; break;
            case '8': key = '('; break;
            case '9': key = ')'; break;
            case '0': key = '='; break;
            case  44: key = ';'; break; // komma
            case  46: key = ':'; break; // punkt
            case 223: key = '?'; break; // scharfes s
        }
        /// @todo: fehlende Zeichen einsetzen
    }
    else
    {
        key = tolower(key);
        //printf("l %d, %c\n", key, key);
        switch(key)
        {
            case 196: key = 'ä'; break;
            case 214: key = 'ö'; break;
            case 220: key = 'ü'; break;
        }
        /// @todo: fehlende Zeichen einsetzen
    }

    char c[2] = {key, 0};
    AnsiToOem(c, c);
    key = c[0];

    /// @todo: restliche "mods"

    return key;
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  Callbackfunktion des GL-Frameworks um Tastatureingaben abzufangen.
 *
 *  @param[in] key    Taste
 *  @param[in] action Aktion der Taste
 *
 *  @author FloSoft
 */
void GLFWCALL VideoGLFW::OnKeyAction(int key, int action)
{
    if(pVideoGLFW == NULL)
        return;

    pVideoGLFW->keyboard[key] = (action == GLFW_PRESS);

    KeyEvent ke = {KT_INVALID, 0, false, false, false };

    if(action == GLFW_PRESS)
    {
        switch(key)
        {
            default:
            {
                // Die 12 F-Tasten
                if(key >= GLFW_KEY_F1 && key <= GLFW_KEY_F12)
                    ke.kt = static_cast<KeyType>(KT_F1 + key - GLFW_KEY_F1);
            } break;
            case GLFW_KEY_LSHIFT:    ke.kt = KT_SHIFT;     break;
            case GLFW_KEY_RSHIFT:    ke.kt = KT_SHIFT;     break;
            case GLFW_KEY_SPACE:     ke.kt = KT_SPACE;     break;
            case GLFW_KEY_ENTER:     ke.kt = KT_RETURN;    break;
            case GLFW_KEY_LEFT:      ke.kt = KT_LEFT;      break;
            case GLFW_KEY_UP:        ke.kt = KT_UP;        break;
            case GLFW_KEY_RIGHT:     ke.kt = KT_RIGHT;     break;
            case GLFW_KEY_DOWN:      ke.kt = KT_DOWN;      break;
            case GLFW_KEY_BACKSPACE: ke.kt = KT_BACKSPACE; break;
            case GLFW_KEY_DEL:       ke.kt = KT_DELETE;    break;
            case GLFW_KEY_TAB:       ke.kt = KT_TAB;       break;
            case GLFW_KEY_END:       ke.kt = KT_END;       break;
        }

        if(ke.kt == KT_INVALID || ke.kt == KT_SPACE)
        {
            ke.kt = KT_CHAR;
            ke.c = pVideoGLFW->TranslateKey(key);
        }

        if(glfwGetKey(GLFW_KEY_LCTRL)  | glfwGetKey(GLFW_KEY_RCTRL))  ke.ctrl  = true;
        if(glfwGetKey(GLFW_KEY_LSHIFT) | glfwGetKey(GLFW_KEY_RSHIFT)) ke.shift = true;
        if(glfwGetKey(GLFW_KEY_LALT)   | glfwGetKey(GLFW_KEY_RALT))   ke.alt   = true;

        pVideoGLFW->CallBack->Msg_KeyDown(ke);
    }
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  Get state of the modifier keys
 *
 *  @author Divan
 */
KeyEvent VideoGLFW::GetModKeyState(void) const
{
    const KeyEvent ke = { KT_INVALID, 0,
                          (glfwGetKey(GLFW_KEY_LCTRL)  | glfwGetKey(GLFW_KEY_RCTRL)  ) ? true : false,
                          (glfwGetKey(GLFW_KEY_LSHIFT) | glfwGetKey(GLFW_KEY_RSHIFT) ) ? true : false,
                          (glfwGetKey(GLFW_KEY_LALT)   | glfwGetKey(GLFW_KEY_RALT)   ) ? true : false
                        };
    return ke;
}

void* VideoGLFW::GetWindowPointer() const { return NULL; }
