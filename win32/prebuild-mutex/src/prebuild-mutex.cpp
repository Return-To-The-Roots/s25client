// $Id: prebuild-mutex.cpp 7500 2011-09-07 12:42:25Z FloSoft $
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
#include <windows.h>

#include <ctime>

#include <string>
#include <vector>
#include <iostream>

using namespace std;

///////////////////////////////////////////////////////////////////////////////
/**
 *  prüft ob die angegebene Datei existiert.
 *
 *  @author FloSoft
 */
bool existfile(std::string file)
{
	FILE *f = fopen(file.c_str(), "r");
	if(!f)
		return false;

	fclose(f);
	return true;
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  führt einen angegebenen (shell)befehl aus
 *
 *  @author FloSoft
 */
void exec(std::string cmd)
{
	//std::cout << "executing \"" << cmd << "\"" << std::endl;
	system(cmd.c_str());
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  kopiert eine Datei von A nach B.
 *
 *  @author FloSoft
 */
void copyfile(std::string file, std::string from, std::string to, std::string tofile = "")
{
	if(tofile.empty())
		tofile = file;

	std::cout << "copying file \"" << file << "\" from \"" << from << "\" to file \"" << tofile << "\" in \"" << to << "\": ";
	from += file;
	to += tofile;
	std::cout << (CopyFile(from.c_str(), to.c_str(), FALSE) ? "ok" : "failed") << std::endl;
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  usage: argv[0] $uniqueid $prebuild-script $params
 *  example: prebuild-mutex projectname prebuild.bat a b c
 *
 *  @author FloSoft
 */
int main(int argc, char *argv[])
{
	srand((unsigned int)std::time(NULL));

	if(argc < 4)
	{
		std::cout << "Usage: " << argv[0] << " pre-/postbuild $workingdir $binarydir" << std::endl;
		return 1;
	}

	vector<string> args;
	for(int i = 0; i < argc; ++i)
		args.push_back(argv[i]);

	bool prebuild = false;

	if(args.at(1) == "prebuild")
		prebuild = true;

	string mutexname = (prebuild ? "prebuild" : "postbuild") + args.at(2);
	string working = args.at(3);
	string binary = args.at(4);

	if(working.at(working.length()-1) != '\\')
		working += "\\";
	if(binary.at(binary.length()-1) != '\\')
		binary += "\\";

	std::cout << (prebuild ? "prebuild" : "postbuild") << "-mutex started" << std::endl;
	//std::cout << "working in " << working << std::endl;
	//std::cout << "binary dir is " << binary << std::endl;

	SetCurrentDirectory(working.c_str());

	HANDLE mHandle; 

	bool exist = false;
	bool hadtowait = false;
	int timeout = 0;
	do
	{
		mHandle = CreateMutex(NULL, true, mutexname.c_str()); 

		if(mHandle != NULL)
		{ 
			exist = (GetLastError() == ERROR_ALREADY_EXISTS);

			if(!exist)
			{
				// do it only if we havent run yet
				if(!hadtowait)
				{
					// Pre/Postbuild-Ereignis mit Parametern starten
					if(prebuild)
					{
						if(!existfile(working + "build_version.h"))
							copyfile("build_version.h.in", working, working, "build_version.h");
						if(!existfile(working + "build_paths.h"))
							copyfile("build_paths.h.in", working, working, "build_paths.h");

						std::string cmd;

						cmd = "\"" + binary + "version.exe\"";
						exec(cmd);

						std::cout << "creating language files" << std::endl;

						std::vector<std::string> langs;

						HANDLE hFile;
						WIN32_FIND_DATA wfd;

						hFile = FindFirstFile("RTTR\\languages\\*.po", &wfd);
						if(hFile != INVALID_HANDLE_VALUE)
						{
							do
							{
								std::string lang = wfd.cFileName;
								lang = lang.substr(0, lang.find_last_of('.'));
								langs.push_back(lang);
							} while(FindNextFile(hFile, &wfd));

							FindClose(hFile);
						}

						for(std::vector<std::string>::iterator it = langs.begin(); it != langs.end(); ++it)
						{
							std::cout << "creating language " << (*it) << std::endl;

							cmd = "msgmerge --sort-output --no-wrap --quiet --update --backup=none -s RTTR/languages/" + (*it) + ".po RTTR/languages/rttr.pot";
							exec(cmd);
							cmd = "msgfmt -o RTTR/languages/" + (*it) + ".mo RTTR/languages/" + (*it) + ".po";
							exec(cmd);
						}
					}
					else
					{
						copyfile("sound-convert.exe", binary, working + "RTTR\\");
						copyfile("s-c_resample.exe", binary, working + "RTTR\\");
						copyfile("libsiedler2.dll", binary, working + "RTTR\\");
					}
				}

				ReleaseMutex(mHandle);
				return 0;
			}
		}
		Sleep(250 + rand() % 100);
		++timeout;

		hadtowait = true;
		return 0;
	}
	while(exist && timeout < 30);

	ReleaseMutex(mHandle);
	return 0;
}