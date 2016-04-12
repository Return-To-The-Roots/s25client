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
#include "DriverWrapper.h"

#include "ListDir.h"
#include "files.h"
#include "fileFuncs.h"
#include "../driver/src/Interface.h"
#include "../driver/src/AudioInterface.h"
#include "../driver/src/VideoInterface.h"
#include "../driver/src/DriverInterfaceVersion.h"
#include "Log.h"
#include "error.h"

#ifndef _WIN32
#	include <dlfcn.h>
#endif // _WIN32

// Include last!
#include "DebugNew.h" // IWYU pragma: keep

DriverWrapper::DriverWrapper() :  dll(0)
{
}

DriverWrapper::~DriverWrapper()
{
    Unload();
}

void DriverWrapper::Unload()
{
    if(dll)
    {
        FreeLibrary(dll);
        dll = 0;
    }
}

bool DriverWrapper::Load(const DriverType dt, std::string& preference)
{
    // ggf. aufräumen vorher
    Unload();

    /// Verfügbare Treiber auflisten
    const std::string DIRECTORY[2] = { "video", "audio" };

    std::vector<DriverItem> drivers = LoadDriverList(dt);

    LOG.lprintf("%u %s drivers found!\n", unsigned(drivers.size()), DIRECTORY[dt].c_str());

    // Welche gefunden?
    if(drivers.empty())
        return false;

    /// Suche, ob der Treiber dabei ist, den wir wünschen
    for(std::vector<DriverItem>::iterator it = drivers.begin(); it != drivers.end(); ++it)
    {
        if(it->GetName() == preference)
        {
            // Dann den gleich nehmen
            dll = LoadLibraryA(it->GetFile().c_str());
            break;
        }
    }

    // ersten Treiber laden
    if(!dll)
    {
        dll = LoadLibraryA(drivers.begin()->GetFile().c_str());

        // Standardwert zuweisen
        preference = drivers.begin()->GetName();
    }

    if(!dll)
    {
        fatal_error("kritischer Treiberfehler: DLL konnte nicht geladen werden\n");
        return false;
    }

    return true;
}

void* GetDLLFunction2(HINSTANCE dll, const std::string& name)
{
#ifdef _WIN32
    union{
        FARPROC proc;
        void* ptr;
    };
    proc = GetProcAddress(dll, name.c_str());
    return ptr;
#else
    return GetProcAddress(dll, name.c_str());
#endif // _WIN32
}

void* DriverWrapper::GetDLLFunction(const std::string& name)
{
    return GetDLLFunction2(dll, name);
}

std::vector<DriverWrapper::DriverItem> DriverWrapper::LoadDriverList(const DriverType dt)
{
    std::vector<DriverItem> driver_list;

    const std::string DIRECTORY[2] = { "video", "audio" };

    std::string path = GetFilePath(FILE_PATHS[46]) + DIRECTORY[dt];
    std::string extension = 
#ifdef _WIN32
                       "dll";
#else
#   ifdef __APPLE__
                       "dylib";
#   else
                       "so";
#   endif // !__APPLE__
#endif // !_WIN32

    LOG.lprintf("searching for drivers in %s\n", path.c_str());
    std::vector<std::string> driver_files = ListDir(path, extension, false);

    HINSTANCE dll;
    for(std::vector<std::string>::iterator it = driver_files.begin(); it != driver_files.end(); ++it)
    {
        std::string path(*it);

#ifdef _WIN32
        // check filename to "rls_*" / "dbg_*", to allow not specialized drivers (for cygwin builds)
        size_t filepos = path.find_last_of("/\\");
        if(filepos != std::string::npos)
        {
            std::string file = path.substr(filepos + 1);
#ifdef _DEBUG
            if(file.substr(0, 4) == "rls_" || file.substr(0, 8) == "Release_")
#else
            if(file.substr(0, 4) == "dbg_" || file.substr(0, 6) == "Debug_")
#endif
                continue;
        }
#endif

        if( (dll = LoadLibraryA(path.c_str())) )
        {
            PDRIVER_GETDRIVERAPIVERSION GetDriverAPIVersion = pto2ptf<PDRIVER_GETDRIVERAPIVERSION>(GetDLLFunction2(dll, "GetDriverAPIVersion"));

            if(GetDriverAPIVersion && GetDriverAPIVersion() == DRIVERAPIVERSION)
            {
                PDRIVER_GETDRIVERNAME GetDriverName = pto2ptf<PDRIVER_GETDRIVERNAME>(GetDLLFunction2(dll, "GetDriverName"));

                if(GetDriverName)
                {
                    PDRIVER_CREATEAUDIOINSTANCE CreateAudioInstance = pto2ptf<PDRIVER_CREATEAUDIOINSTANCE>(GetDLLFunction2(dll, "CreateAudioInstance"));
                    PDRIVER_CREATEVIDEOINSTANCE CreateVideoInstance = pto2ptf<PDRIVER_CREATEVIDEOINSTANCE>(GetDLLFunction2(dll, "CreateVideoInstance"));

                    if((dt == DT_VIDEO && CreateVideoInstance) || (dt == DT_AUDIO && CreateAudioInstance))
                    {
                        DriverItem di(*it, GetDriverName());
                        driver_list.push_back(di);
                    }
                }
            }

            FreeLibrary(dll);
            dll = 0;
        }
    }
    return driver_list;
}

