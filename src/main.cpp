﻿// $Id: main.cpp 9357 2014-04-25 15:35:25Z FloSoft $
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
#include "defines.h"


#ifdef _WIN32
#   include <windows.h>
#   define chdir !SetCurrentDirectoryA
#   ifndef __CYGWIN__
#       include <conio.h>
#   endif
#else
#   include <unistd.h>
#endif

#ifndef _MSC_VER
#   include <csignal>
#endif

#include "GlobalVars.h"
#include "signale.h"
#include "Socket.h"

#include "GameManager.h"

#include "GameClient.h"
#include "Settings.h"
#include "Log.h"
#include "error.h"
#include "files.h"

#include "../libsiedler2/src/types.h"
#include "ogl/glAllocator.h"

// This is for catching crashes and reporting bugs, it does not slow down anything.
#include "Debug.h"

#ifndef NDEBUG
    #include "GameWorld.h"
    #include "GameServer.h"
    #include "ingameWindows/iwDirectIPCreate.h"
    #include "WindowManager.h"
    #include "desktops/dskGameLoader.h"
    #include "desktops/dskSelectMap.h"
    #include "ingameWindows/iwPleaseWait.h"
#endif

#include "fileFuncs.h"

#ifdef __APPLE__
#   include <SDL_main.h>
#endif // __APPLE__

#ifdef _WIN32
#   include "../win32/resource.h"
#   include "drivers/VideoDriverWrapper.h"
#   ifdef _MSC_VER
#        define getch _getch
#   endif
#endif

#if defined _WIN32 && defined _DEBUG && defined _MSC_VER && !defined NOHWETRANS
#   include <eh.h>
#endif

#include <ctime>

///////////////////////////////////////////////////////////////////////////////
// Makros / Defines
#if defined _WIN32 && defined _DEBUG && defined _MSC_VER
#define new new(_NORMAL_BLOCK, THIS_FILE, __LINE__)
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

///////////////////////////////////////////////////////////////////////////////
/**
 *  Exit-Handler, wird bei @p exit ausgefÃ¼hrt.
 *
 *  @author FloSoft
 */
void ExitHandler(void)
{
    Socket::Shutdown();

#if defined _WIN32 && !defined __CYGWIN__
    LOG.lprintf("\n\nDr%ccken Sie eine beliebige Taste . . .\n", 129);
    getch();
#endif
}

#if defined _WIN32 && defined _DEBUG && defined _MSC_VER && !defined NOHWETRANS
///////////////////////////////////////////////////////////////////////////////
/**
 *  Exception-Handler, wird bei einer C-Exception ausgefÃ¼hrt, falls
 *  dies in der build_paths.h mit deaktiviertem NOHWETRANS und
 *  im Projekt mit den Compilerflags (/EHa) aktiviert ist.
 *
 *  @param[in] exception_type    Typ der Exception (siehe GetExceptionCode)
 *  @param[in] exception_pointer Genaue Beschreibung der Exception (siehe GetExceptionInformation)
 *
 *  @author OLiver
 */
void ExceptionHandler (unsigned int exception_type, _EXCEPTION_POINTERS* exception_pointer)
{
    fatal_error("C-Exception caught\n");
}
#endif // _WIN32 && _DEBUG && !NOHWETRANS

#ifdef _WIN32
#ifdef _MSC_VER
LONG WINAPI
#else
void
#endif
WinExceptionHandler(
#ifdef _MSC_VER
    LPEXCEPTION_POINTERS info
#else
    int sig
#endif
)
{
    if ((SETTINGS.global.submit_debug_data == 1) ||
            MessageBoxA(NULL,
                        _("RttR crashed. Would you like to send debug information to RttR to help us avoiding this crash in the future? Thank you very much!"),
                        _("Error"), MB_YESNO | MB_ICONERROR | MB_TASKMODAL | MB_SETFOREGROUND) == IDYES)
    {
        VIDEODRIVER.DestroyScreen();

        DebugInfo di;

        di.SendReplay();
        di.SendStackTrace(
#ifdef _MSC_VER
            info->ContextRecord
#endif
        );
    }

    if(SETTINGS.global.submit_debug_data == 0)
        MessageBoxA(NULL, _("RttR crashed. Please restart the application!"), _("Error"), MB_OK | MB_ICONERROR | MB_TASKMODAL | MB_SETFOREGROUND);

    _exit(1);

#ifdef _MSC_VER
    return(EXCEPTION_EXECUTE_HANDLER);
#endif
}
#else
void LinExceptionHandler(int sig)
{
    if (SETTINGS.global.submit_debug_data == 1)
    {
        DebugInfo di;

        di.SendReplay();
        di.SendStackTrace();
    }

    abort();
}
#endif

///////////////////////////////////////////////////////////////////////////////
/**
 *  Hauptfunktion von Siedler II.5 Return to the Roots
 *
 *  @param[in] argc Anzahl Ã¼bergebener Argumente
 *  @param[in] argv Array der Ã¼bergebenen Argumente
 *
 *  @return Exit Status, 0 bei Erfolg, > 0 bei Fehler
 *
 *  @author FloSoft
 *  @author OLiver
 */
int main(int argc, char* argv[])
{
#if defined _WIN32 && defined _DEBUG && defined _MSC_VER && !defined NOHWETRANS
    _set_se_translator(ExceptionHandler);
#endif // _WIN32 && _DEBUG && !NOHWETRANS

#if defined _WIN32 && defined _DEBUG && defined _MSC_VER && !defined NOCRTDBG
    // Enable Memory-Leak-Detection
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_CHECK_ALWAYS_DF | _CRTDBG_LEAK_CHECK_DF /*| _CRTDBG_CHECK_CRT_DF*/);
#endif // _WIN32 && _DEBUG && !NOCRTDBG

    // Signal-Handler setzen
#ifdef _WIN32
    SetConsoleCtrlHandler(HandlerRoutine, TRUE);

    // set console window icon
    SendMessage(GetConsoleWindow(), WM_SETICON, (WPARAM)TRUE, (LPARAM)LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_SYMBOL)));

    // Set UTF-8 console charset
    SetConsoleOutputCP(CP_UTF8);

#ifndef _MSC_VER
    signal(SIGSEGV, WinExceptionHandler);
#else
    SetUnhandledExceptionFilter(WinExceptionHandler);
#endif
    //AddVectoredExceptionHandler(1, WinExceptionHandler);
#else
    struct sigaction sa;
    sa.sa_handler = HandlerRoutine;
    sa.sa_flags = 0; //SA_RESTART would not allow to interrupt connect call;
    sigemptyset(&sa.sa_mask);

    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGPIPE, &sa, NULL);
    sigaction(SIGALRM, &sa, NULL);

    signal(SIGSEGV, LinExceptionHandler);
#endif // _WIN32


	// We need to make the exe's path to the current dir
	std::string exePath = argv[0];
	// get '/' or '\\' depending on unix/mac or windows.
#if defined(_WIN32) || defined(WIN32)
	size_t pos = exePath.rfind('\\');
#else
	size_t pos = exePath.rfind('/');
#endif
	if(pos != std::string::npos){
		exePath = exePath.substr(0, pos);
		if(chdir(exePath.c_str()))
			error("Could not change dir to %s", exePath.c_str());
	}

    // diverse dirs anlegen
    const unsigned int dir_count = 7;
    unsigned int dirs[dir_count] = { 94, 47, 48, 51, 85, 98, 99 }; // settingsdir muss zuerst angelegt werden (94)

#ifdef _WIN32
    if(IsDir(GetFilePath("~/Siedler II.5 RttR")))
        MoveFileA(GetFilePath("~/Siedler II.5 RttR").c_str(), GetFilePath(FILE_PATHS[94]).c_str());
#endif

#ifdef __APPLE__
    if(IsDir(GetFilePath("~/.s25rttr")))
        rename(GetFilePath("~/.s25rttr").c_str(), GetFilePath(FILE_PATHS[94]).c_str());
#endif

    for(unsigned int i = 0; i < dir_count; ++i)
    {
        std::string dir = GetFilePath(FILE_PATHS[dirs[i]]);

        if(mkdir_p(dir) < 0)
        {
            error("Verzeichnis %s konnte nicht erstellt werden: ", dir.c_str());
            error("Das Spiel konnte nicht gestartet werden");
            return 1;
        }
    }

    libsiedler2::setTextureFormat(libsiedler2::FORMAT_RGBA);
    libsiedler2::setAllocator(glAllocator);

    // Zufallsgenerator initialisieren (Achtung: nur fÃ¼r Animationens-Offsets interessant, fÃ¼r alles andere (spielentscheidende) wird unser Generator verwendet)
    srand(static_cast<unsigned int>(std::time(NULL)));

    // Exit-Handler initialisieren
    atexit(&ExitHandler);

    // Socketzeug initialisieren
    if(!Socket::Initialize())
    {
        error("Konnte Sockets nicht initialisieren!");
        return 1;
    }

    // Spiel starten
    if(!GAMEMANAGER.Start())
    {
        error("Das Spiel konnte nicht gestartet werden");
        return 1;
    }

#ifndef NDEBUG
    if (argc > 1)
    {
        CreateServerInfo csi;
        csi.gamename = _("Unlimited Play");
        csi.password = "localgame";
        csi.port = 3665;
        csi.type = NP_LOCAL;
        csi.ipv6 = false;
        csi.use_upnp = false;

        printf("loading game!\n");

        WINDOWMANAGER.Switch(new dskSelectMap(csi));

        if(!GAMESERVER.TryToStart(csi, argv[1], MAPTYPE_SAVEGAME))
        {
            if(!GAMESERVER.TryToStart(csi, argv[1], MAPTYPE_OLDMAP))
            {
                GameWorldViewer* gwv;
                unsigned int error = GAMECLIENT.StartReplay(argv[1], gwv);

                std::string replay_errors[] =
                {
                    _("Error while playing replay!"),
                    _("Error while opening file!"),
                    _("Invalid Replay!"),
                    _("Error: Replay is too old!"),
                    _("Program version is too old to play that replay!"),
                    "",
                    _("Temporary map file was not found!")
                };

                if (error)
                {
                    printf("ERROR: %s\n", replay_errors[error].c_str());
                }
                else
                {
                    WINDOWMANAGER.Switch(new dskGameLoader(gwv));
                }
            }
            else
            {
                WINDOWMANAGER.Draw();
                WINDOWMANAGER.Show(new iwPleaseWait);
            }
        }
        else
        {
            WINDOWMANAGER.Draw();
            WINDOWMANAGER.Show(new iwPleaseWait);
        }
    }
#endif

    // Hauptschleife
    while(GAMEMANAGER.Run())
    {
#ifndef _WIN32
        extern bool killme;
        killme = false;
#endif // !_WIN32
    }

    // Spiel beenden
    GAMEMANAGER.Stop();

    return 0;
}
