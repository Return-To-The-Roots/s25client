// Copyright (c) 2005 - 2015 Settlers Freaks (sf-team at siedler25.org)
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
#include "defines.h" // IWYU pragma: keep
#include "RTTR_AssertError.h"

#ifdef _WIN32
#   include <windows.h>
#endif

#ifndef _MSC_VER
#   include <csignal>
#endif

#include "signale.h"

#include "GameManager.h"

#include "Settings.h"
#include "Log.h"
#include "error.h"
#include "files.h"

// This is for catching crashes and reporting bugs, it does not slow down anything.
#include "Debug.h"

#include "QuickStartGame.h"
#include "GlobalVars.h"
#include "fileFuncs.h"

#include "ogl/glAllocator.h"
#include "libsiedler2/src/libsiedler2.h"
#include "mygettext/src/mygettext.h"

#ifdef _WIN32
#   include "../win32/resource.h"
#   include "drivers/VideoDriverWrapper.h"
#endif

#include <boost/filesystem.hpp>
#include <boost/program_options.hpp>

#ifdef __APPLE__
#   include <SDL_main.h>
#endif // __APPLE__

#if defined _WIN32 && defined _DEBUG && defined _MSC_VER && !defined NOHWETRANS
#   include <eh.h>
#endif

//#include <vld.h> 

#include <ctime>
#include <iostream>
#include <limits>
#include <cstdlib>

// Include last!
#include "DebugNew.h" // IWYU pragma: keep

namespace po = boost::program_options;

void WaitForEnter()
{
    static bool waited = false;
    if(waited)
        return;
    waited = true;
    std::cout << "\n\nPress ENTER to close this window . . ." << std::endl;
    std::cin.clear();
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    std::cin.get();
}

///////////////////////////////////////////////////////////////////////////////


#if defined _WIN32 && defined _DEBUG && defined _MSC_VER && !defined NOHWETRANS
///////////////////////////////////////////////////////////////////////////////
/**
 *  Exception-Handler, wird bei einer C-Exception ausgeführt, falls
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
    if(GLOBALVARS.isTest)
    {
        std::cerr << std::endl << "ERROR: Test failed!" << std::endl;
        _exit(1);

#ifdef _MSC_VER
        return(EXCEPTION_EXECUTE_HANDLER);
#endif
    }

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
void LinExceptionHandler(int  /*sig*/)
{
    if(GLOBALVARS.isTest)
    {
        std::cerr << std::endl << "ERROR: Test failed!" << std::endl;
        abort();
    }

    if (SETTINGS.global.submit_debug_data == 1)
    {
        DebugInfo di;

        di.SendReplay();
        di.SendStackTrace();
    }

    abort();
}
#endif

void InstallSignalHandlers()
{
#ifdef _WIN32
    SetConsoleCtrlHandler(HandlerRoutine, TRUE);

#   ifndef _MSC_VER
    signal(SIGSEGV, WinExceptionHandler);
#   else
    SetUnhandledExceptionFilter(WinExceptionHandler);
#   endif
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
}

void UninstallSignalHandlers()
{
#ifdef _WIN32
    SetConsoleCtrlHandler(HandlerRoutine, FALSE);

#   ifndef _MSC_VER
    signal(SIGSEGV, SIG_DFL);
#   else
    SetUnhandledExceptionFilter(NULL);
#   endif
#else
    struct sigaction sa;
    sa.sa_handler = SIG_DFL;
    sa.sa_flags = 0; //SA_RESTART would not allow to interrupt connect call;
    sigemptyset(&sa.sa_mask);

    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGPIPE, &sa, NULL);
    sigaction(SIGALRM, &sa, NULL);

    signal(SIGSEGV, SIG_DFL);
#endif // _WIN32
}

//////////////////////////////////////////////////////////////////////////
/**
 *  Exit-Handler, wird bei @p exit ausgeführt.
 *
 *  @author FloSoft
 */
void ExitHandler()
{
    Socket::Shutdown();
    UninstallSignalHandlers();

#ifdef _DEBUG
    WaitForEnter();
#endif
}

bool InitProgram()
{
    // Check and set locale (avoids errors caused by invalid locales later like #420)
    try{
        // Check for errors and use classic locale to avoid e.g. thousand separator in int2string conversions via streams
        std::locale::global(std::locale::classic());
    }catch(std::exception& e){
        std::cerr << "Error initializing your locale setting. ";
#ifdef _WIN32
        std::cerr << "Check your system language configuration!";
#else
        char* lcAll = getenv("LC_ALL");
        char* lcLang = getenv("LC_LANG");
        std::cerr << "Check your environment for invalid settings (e.g. LC_ALL";
        if(lcAll)
            std::cerr << "=" << lcAll;
        std::cerr << " or LC_LANG";
        if(lcLang)
            std::cerr << "=" << lcLang;
        std::cerr << ")";
#endif
        std::cerr << std::endl;
        std::cerr << e.what() << std::endl;
        return false;
    }

#if defined _WIN32 && defined _DEBUG && defined _MSC_VER && !defined NOHWETRANS
    _set_se_translator(ExceptionHandler);
#endif // _WIN32 && _DEBUG && !NOHWETRANS

#if defined _WIN32 && defined _DEBUG && defined _MSC_VER && !defined NOCRTDBG
    // Enable Memory-Leak-Detection
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF /*| _CRTDBG_CHECK_EVERY_1024_DF*/);
#endif // _WIN32 && _DEBUG && !NOCRTDBG

#ifdef _WIN32
    // set console window icon
    SendMessage(GetConsoleWindow(), WM_SETICON, (WPARAM)TRUE, (LPARAM)LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_SYMBOL)));

    // Set UTF-8 console charset
    SetConsoleOutputCP(CP_UTF8);
#endif // _WIN32

    InstallSignalHandlers();

    return true;
}

bool InitDirectories()
{
    // diverse dirs anlegen
    const unsigned int dir_count = 7;
    unsigned int dirs[dir_count] = { 94, 47, 48, 51, 85, 98, 99 }; // settingsdir muss zuerst angelegt werden (94)

    std::string oldSettingsDir;
    std::string newSettingsDir = GetFilePath(FILE_PATHS[94]);

#ifdef _WIN32
    oldSettingsDir = GetFilePath("~/Siedler II.5 RttR");
#elif defined(__APPLE__)
    oldSettingsDir = GetFilePath("~/.s25rttr");
#endif
    if(!oldSettingsDir.empty() && boost::filesystem::is_directory(oldSettingsDir))
        boost::filesystem::rename(oldSettingsDir, newSettingsDir);

    for(unsigned int i = 0; i < dir_count; ++i)
    {
        std::string dir = GetFilePath(FILE_PATHS[dirs[i]]);
        boost::system::error_code ec;
        boost::filesystem::create_directories(dir, ec);
        if(ec != boost::system::errc::success)
        {
            error("Directory %s could not be created: ", dir.c_str());
            error("Failed to start the game");
            WaitForEnter();
            return false;
        }
    }
    return true;
}

bool InitGame()
{
    libsiedler2::setTextureFormat(libsiedler2::FORMAT_RGBA);
    libsiedler2::setAllocator(new GlAllocator());

    // Socketzeug initialisieren
    if(!Socket::Initialize())
    {
        error("Could not init sockets!");
        error("Failed to start the game");
        WaitForEnter();
        return false;
    }

    // Spiel starten
    if(!GAMEMANAGER.Start())
    {
        error("Failed to start the game");
        WaitForEnter();
        return false;
    }
    return true;
}

int RunProgram(po::variables_map& options)
{
    if(!InitProgram())
        return 1;
    if(!InitDirectories())
        return 1;

    // Zufallsgenerator initialisieren (Achtung: nur für Animations-Offsets interessant, für alles andere (spielentscheidende) wird unser Generator verwendet)
    srand(static_cast<unsigned int>(std::time(NULL)));

    // Exit-Handler initialisieren
    atexit(&ExitHandler);

    try{
        if(!InitGame())
            return 2;

        if(options.count("map"))
            QuickStartGame(options["map"].as<std::string>());

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
        libsiedler2::setAllocator(NULL);
    } catch(RTTR_AssertError& error)
    {
        // Write to log file, but don't throw any errors if this fails too
        try{
            LOG.write(error.what());
        } catch(...){} //-V565
        return 42;
    }
    return 0;
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  Hauptfunktion von Siedler II.5 Return to the Roots
 *
 *  @param[in] argc Anzahl übergebener Argumente
 *  @param[in] argv Array der übergebenen Argumente
 *
 *  @return Exit Status, 0 bei Erfolg, > 0 bei Fehler
 *
 *  @author FloSoft
 *  @author OLiver
 */
int main(int argc, char** argv)
{
    po::options_description desc("Allowed options");
    desc.add_options()
        ("help,h", "Show help")
        ("map,m", po::value<std::string>(), "Map to load")
        ("test", "Run in test mode (shows errors during run)")
        ;
    po::positional_options_description positionalOptions;
    positionalOptions.add("map", 1);

    po::variables_map options;
    po::store(po::command_line_parser(argc, argv).options(desc)
        .positional(positionalOptions).run(),
        options);
    po::notify(options);

    if(options.count("help")) {
        std::cout << desc << "\n";
        return 1;
    }

    GLOBALVARS.isTest = options.count("test") > 0;
    GLOBALVARS.errorOccured = false;

    int result = RunProgram(options);
    if(GLOBALVARS.isTest)
    {
        if(result || GLOBALVARS.errorOccured)
        {
            std::cerr << std::endl << std::endl << "ERROR: Test failed!" << std::endl;
            if(!result)
                result = 1;
        } else
            std::cout << std::endl << std::endl << "Test passed!" << std::endl;
    }    

    return result;
}
