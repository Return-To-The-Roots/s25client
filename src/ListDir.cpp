// $Id: ListDir.cpp 9357 2014-04-25 15:35:25Z FloSoft $
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
#include "ListDir.h"

///////////////////////////////////////////////////////////////////////////////
// Makros / Defines
#if defined _WIN32 && defined _DEBUG && defined _MSC_VER
#define new new(_NORMAL_BLOCK, THIS_FILE, __LINE__)
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


///////////////////////////////////////////////////////////////////////////////
// lists the files of a directory
//void ListDir(const std::string& path, void (*CallBack)(const std::string& filename, void * param), void *param, StringList *liste)

void ListDir(const std::string& path, bool directories, void (*CallBack)(const std::string& filename, void* param), void* param, std::list<std::string> *liste)
{
    // Pfad zum Ordner, wo die Dateien gesucht werden sollen
    std::string rpath = path.substr(0, path.find_last_of('/') + 1);

    // Pfad in Kleinbuchstaben umwandeln
    std::string filen(path);
    std::transform(path.begin(), path.end(), filen.begin(), tolower);

    //LOG.lprintf("%s\n", filen.c_str());
    // Dateiendung merken
    size_t pos = path.find_last_of('.');
    if(pos == std::string::npos)
        return;

    std::string endung = path.substr(pos + 1);

#ifdef _WIN32
    HANDLE hFile;
    WIN32_FIND_DATAA wfd;

    hFile = FindFirstFileA(path.c_str(), &wfd);
    if(hFile != INVALID_HANDLE_VALUE)
    {
        do
        {
            std::string whole_path = rpath + wfd.cFileName;

            bool push = true;
            if(!directories && ( (wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == FILE_ATTRIBUTE_DIRECTORY) )
                push = false;

            if(push)
            {
                if(CallBack)
                    CallBack(whole_path.c_str(), param);
                if(liste)
                    liste->push_back(whole_path);
            }
        }
        while(FindNextFileA(hFile, &wfd));

        FindClose(hFile);
    }
#else
    DIR* dir_d;
    dirent* dir = NULL;
    if ((dir_d = opendir(rpath.c_str())) != NULL)
    {
        while( (dir = readdir(dir_d)) != NULL)
        {
            struct stat file_stat;
            std::string whole_path = rpath + dir->d_name;

            stat(whole_path.c_str(), &file_stat);

            bool push = true;
            if(!directories && S_ISDIR(file_stat.st_mode))
                push = false;

            if(push)
            {
                std::string ende = dir->d_name;
                size_t pos = ende.find_last_of('.');
                if(pos == std::string::npos)
                    continue;

                ende = ende.substr(pos + 1);

                if(endung != ende)
                    std::transform(ende.begin(), ende.end(), ende.begin(), tolower);

                //LOG.lprintf("%s == %s\n", endung.c_str(), ende.c_str());

                if(endung != ende)
                    continue;

                if(CallBack)
                    CallBack(whole_path, param);
                if(liste)
                    liste->push_back(whole_path);
            }
        }
        closedir(dir_d);
        if(liste)
            liste->sort();
    }
#endif // _WIN32

}
