// $Id: main.cpp 7358 2011-08-10 09:50:48Z FloSoft $
//
// Copyright (c) 2005 - 2010 Settlers Freaks (sf-team at siedler25.org)
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
#include <stdafx.h>
#include "main.h"

#include "GlobalVars.h"
#include "signale.h"
#include "Socket.h"

#include "GameManager.h"

#ifdef __APPLE__
#	include <SDL_main.h>
#endif // __APPLE__

#ifdef _WIN32
#	include "../win32/resource.h"
#endif

#if defined _WIN32 && defined _DEBUG && defined _MSC_VER && !defined NOHWETRANS
#	include <windows.h>
#	include <eh.h>
#endif

///////////////////////////////////////////////////////////////////////////////
// Makros / Defines
#if defined _WIN32 && defined _DEBUG && defined _MSC_VER
	#define new new(_NORMAL_BLOCK, THIS_FILE, __LINE__)
	#undef THIS_FILE
	static char THIS_FILE[] = __FILE__;
#endif

///////////////////////////////////////////////////////////////////////////////
/**
 *  Exit-Handler, wird bei @p exit ausgeführt.
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

int mkdir_p(const std::string dir)
{
	if(IsDir(dir))
		return 0;

	if (
#ifdef _WIN32
		!CreateDirectory(dir.c_str(), NULL)
#else
		mkdir(dir.c_str(), 0750) < 0
#endif
	)
	{
		size_t slash = dir.rfind('/');
		if (slash != std::string::npos) 
		{
			std::string prefix = dir.substr(0, slash);
			if(mkdir_p(prefix) == 0)
			{
				return (
#ifdef _WIN32
					CreateDirectory(dir.c_str(), NULL) ? 0 : -1
#else
					mkdir(dir.c_str(), 0750)
#endif
				);
			}
		}
		return -1;
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
int main(int argc, char *argv[])
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
#else
	struct sigaction sa;
	sa.sa_handler = HandlerRoutine;
	sa.sa_flags = 0; //SA_RESTART would not allow to interrupt connect call;
	sigemptyset(&sa.sa_mask);

	sigaction(SIGINT, &sa, NULL);
	sigaction(SIGPIPE, &sa, NULL);
	sigaction(SIGALRM, &sa, NULL);
#endif // _WIN32

	// diverse dirs anlegen
	const unsigned int dir_count = 7;
	unsigned int dirs[dir_count] = { 94, 47, 48, 51, 85, 98, 99 }; // settingsdir muss zuerst angelegt werden (94)
	
#ifdef _WIN32
	if(IsDir(GetFilePath("~/Siedler II.5 RttR")))
		MoveFile(GetFilePath("~/Siedler II.5 RttR"), GetFilePath(FILE_PATHS[94]));
#endif

#ifdef __APPLE__
	if(IsDir(GetFilePath("~/.s25rttr")))
		rename(GetFilePath("~/.s25rttr"), GetFilePath(FILE_PATHS[94]));
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

	// Zufallsgenerator initialisieren (Achtung: nur für Animationens-Offsets interessant, für alles andere (spielentscheidende) wird unser Generator verwendet)
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
