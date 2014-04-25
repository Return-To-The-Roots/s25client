// $Id: prebuild-mutex.cpp 9357 2014-04-25 15:35:25Z FloSoft $
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
#include <windows.h>

#include <ctime>

#include <string>
#include <vector>
#include <iostream>
#include <algorithm>
#include <sys/stat.h>

using namespace std;

#define CONTRIBDIR "contrib"
#define RELEASEDIR "win32\\bin"
#define DORELEASE

///////////////////////////////////////////////////////////////////////////////
/**
 *  prüft ob die angegebene Datei existiert.
 *
 *  @author FloSoft
 */
bool existfile(string file)
{
    FILE* f = fopen(file.c_str(), "r");
    if(!f)
        return false;

    fclose(f);
    return true;
}

bool existDir(string dir)
{
    DWORD dwAttrib = GetFileAttributesA(dir.c_str());

    return (dwAttrib != INVALID_FILE_ATTRIBUTES && (dwAttrib & FILE_ATTRIBUTE_DIRECTORY));
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  führt einen angegebenen (shell)befehl aus
 *
 *  @author FloSoft
 */
void exec(string cmd)
{
    //cout << "executing \"" << cmd << "\"" << endl;
    system(cmd.c_str());
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  kopiert eine Datei von A nach B.
 *
 *  @author FloSoft
 */
void copyfile(string file, string from, string to, string tofile = "")
{
    if(tofile.empty())
        tofile = file;

    cout << "copying file \"" << file << "\" from \"" << from << "\" to file \"" << tofile << "\" in \"" << to << "\": ";
    from += file;
    to += tofile;
    cout << (CopyFileA(from.c_str(), to.c_str(), FALSE) ? "ok" : "failed") << endl;
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  usage: argv[0] $uniqueid $workingdir $binarydir
 *  example:
 *
 *  "$(OutDir)prebuild-mutex.exe" "prebuild" "$(SolutionName)" "$(ProjectName)" "$(ProjectDir)..\.." "$(OutDir)"
 *  "$(OutDir)prebuild-mutex.exe" "postbuild" "$(SolutionName)" "$(ProjectName)" "$(ProjectDir)..\.." "$(OutDir)" "$(SolutionDir)..\..\contrib"
 *
 *  @author FloSoft
 */
int main(int argc, char* argv[])
{
    srand((unsigned int)time(NULL));

    if(argc < 6)
    {
        cout << "Usage: " << argv[0] << " pre-/postbuild $SolutionName $ProjectName $workingdir $binarydir" << endl;
        return 1;
    }

    vector<string> args;
    for(int i = 0; i < argc; ++i)
        args.push_back(argv[i]);

    bool prebuild = false;

    if(args.at(1) == "prebuild")
        prebuild = true;

    string mutexname = (prebuild ? "prebuild" : "postbuild") + args.at(2);
    string project = args.at(3);
    string working = args.at(4);
    string binary = args.at(5);

    replace(working.begin(), working.end(), '"', '\\');
    replace(binary.begin(), binary.end(), '"', '\\');

    if(working.at(working.length() - 1) != '\\')
        working += "\\";
    if(binary.at(binary.length() - 1) != '\\')
        binary += "\\";

    string contribDir = working + CONTRIBDIR;
    string releaseDir = working + RELEASEDIR;

    if(contribDir.at(contribDir.length() - 1) != '\\')
        contribDir += "\\";
    if(releaseDir.at(releaseDir.length() - 1) != '\\')
        releaseDir += "\\";

    cout << (prebuild ? "prebuild" : "postbuild") << "-mutex for " << project << " started" << endl;
    cout << "working dir is " << working << endl;
    cout << "binary dir is " << binary << endl;

    SetCurrentDirectoryA(working.c_str());

    HANDLE mHandle;

    bool exist = false;
    bool hadtowait = false;
    int timeout = 0;
    do
    {
        mHandle = CreateMutexA(NULL, true, mutexname.c_str());

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

                        string cmd;

                        cmd = "\"" + binary + "version.exe\"";
                        exec(cmd);

                        if(project == "s25client")
                        {
                            cout << "creating language files" << endl;

                            vector<string> langs;

                            HANDLE hFile;
                            WIN32_FIND_DATAA wfd;

                            hFile = FindFirstFileA("RTTR\\languages\\*.po", &wfd);
                            if(hFile != INVALID_HANDLE_VALUE)
                            {
                                do
                                {
                                    string lang = wfd.cFileName;
                                    lang = lang.substr(0, lang.find_last_of('.'));
                                    langs.push_back(lang);
                                }
                                while(FindNextFileA(hFile, &wfd));

                                FindClose(hFile);
                            }

                            for(vector<string>::iterator it = langs.begin(); it != langs.end(); ++it)
                            {
                                cout << "creating language " << (*it) << endl;

                                cmd = "msgmerge --sort-output --no-wrap --quiet --update --backup=none -s RTTR/languages/" + (*it) + ".po RTTR/languages/rttr.pot";
                                exec(cmd);
                                cmd = "msgfmt -o RTTR/languages/" + (*it) + ".mo RTTR/languages/" + (*it) + ".po";
                                exec(cmd);
                            }
                        }
                    }
                    else
                    {
                        if(project == "s25client")
                        {
                            const int DLLCOUNT = 8;
                            char* dlls[DLLCOUNT] = {"libiconv2.dll", "libogg-0.dll", "libvorbis-0.dll", "libvorbisfile-3.dll", "mikmod.dll", "SDL.dll", "SDL_mixer.dll" , "smpeg.dll" };
                            copyfile("sound-convert.exe", binary, working + "RTTR\\");
                            copyfile("s-c_resample.exe", binary, working + "RTTR\\");
                            copyfile("s25update.exe", binary, working + "RTTR\\");
                            string contribDirSub = contribDir + "full-contrib-msvc2010\\bin\\";
                            if(existDir(contribDir))
                            {
                                copyfile( "lua52.dll", contribDir + "lua\\win32\\", binary);
                                if(existDir(contribDirSub))
                                {
                                    for(int i = 0; i < DLLCOUNT; i++)
                                        copyfile( dlls[i], contribDirSub, binary);
#ifdef _DEBUG
                                    copyfile("libcurld.dll", contribDirSub, working + "RTTR\\");
#else
                                    copyfile("libcurl.dll", contribDirSub, working + "RTTR\\");
#endif
                                }
                            }
#ifdef DORELEASE
                            if(existDir(releaseDir))
                                exec("rd /S /Q \"" + releaseDir + "\"");
                            exec("mkdir \"" + releaseDir + "\"");
                            exec("mkdir \"" + releaseDir + "driver\"");
                            exec("mkdir \"" + releaseDir + "driver\\audio\"");
                            exec("mkdir \"" + releaseDir + "driver\\video\"");
                            exec("xcopy /E /C /I /Y /Q \"" + working + "RTTR\" \"" + releaseDir + "RTTR\"");
                            copyfile("s25client.exe", binary, releaseDir);
                            exec("copy \"" + working + "driver\\audio\\*.dll\" \"" + releaseDir + "driver\\audio\\\"");
                            exec("copy \"" + working + "driver\\video\\*.dll\" \"" + releaseDir + "driver\\video\\\"");
                            if(existDir(contribDir))
                            {
                                copyfile( "lua52.dll", contribDir + "lua\\win32\\", releaseDir);
                                if(existDir(contribDirSub))
                                {
                                    for(int i = 0; i < DLLCOUNT; i++)
                                        copyfile( dlls[i], contribDirSub, releaseDir);
                                }
                            }
#endif
                        }
                    }
                }

                cout << (prebuild ? "prebuild" : "postbuild") << "-mutex for " << project << " finished" << endl;
                cout << endl;

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