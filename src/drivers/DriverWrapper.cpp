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

#include "rttrDefines.h" // IWYU pragma: keep
#include "DriverWrapper.h"
#include "ListDir.h"
#include "RttrConfig.h"
#include "driver/DriverInterfaceVersion.h"
#include "driver/Interface.h"
#include "files.h"
#include "mygettext/mygettext.h"
#include "libutil/Log.h"
#include "libutil/error.h"
#include <boost/interprocess/smart_ptr/unique_ptr.hpp>

#ifndef _WIN32
#include <dlfcn.h>
#endif // _WIN32

namespace {
struct DeleterFreeLib
{
    typedef HINSTANCE pointer;
    void operator()(HINSTANCE dll) const { FreeLibrary(dll); }
};
} // namespace

DriverWrapper::DriverWrapper() : dll(0) {}

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
    const std::string DIRECTORY[2] = {"video", "audio"};

    std::vector<DriverItem> drivers = LoadDriverList(dt);

    LOG.write("%u %s drivers found!\n") % unsigned(drivers.size()) % DIRECTORY[dt];

    // Welche gefunden?
    if(drivers.empty())
        return false;

    /// Suche, ob der Treiber dabei ist, den wir wünschen
    for(std::vector<DriverItem>::iterator it = drivers.begin(); it != drivers.end(); ++it)
    {
        if(it->GetName() == preference)
        {
            // Dann den gleich nehmen
            dll = LoadLibraryW(it->GetFile().c_str());
            break;
        }
    }

    // ersten Treiber laden
    if(!dll)
    {
        dll = LoadLibraryW(drivers.begin()->GetFile().c_str());

        // Standardwert zuweisen
        preference = drivers.begin()->GetName();
    }

    if(!dll)
    {
        s25util::fatal_error("Could not load driver library\n");
        return false;
    }

    return true;
}

void* GetDLLFunction2(HINSTANCE dll, const std::string& name)
{
    if(!dll)
        return NULL;
#ifdef _WIN32
    union
    {
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

bool DriverWrapper::CheckLibrary(const bfs::path& path, DriverType dt, std::string& nameOrError)
{
    boost::interprocess::unique_ptr<HINSTANCE, DeleterFreeLib> dll(LoadLibraryW(path.c_str()));

    if(!dll)
    {
        nameOrError = _("Failed to load library. Check dependencies!");
        return false;
    }

    PDRIVER_GETDRIVERAPIVERSION GetDriverAPIVersion =
      pto2ptf<PDRIVER_GETDRIVERAPIVERSION>(GetDLLFunction2(dll.get(), "GetDriverAPIVersion"));
    if(!GetDriverAPIVersion)
    {
        nameOrError = _("Not a RTTR driver library!");
        return false;
    }
    if(GetDriverAPIVersion() != DRIVERAPIVERSION)
    {
        nameOrError = _("Invalid API version!");
        return false;
    }

    PDRIVER_GETDRIVERNAME GetDriverName = pto2ptf<PDRIVER_GETDRIVERNAME>(GetDLLFunction2(dll.get(), "GetDriverName"));
    std::string createName, freeName;
    if(dt == DT_VIDEO)
    {
        createName = "CreateVideoInstance";
        freeName = "FreeVideoInstance";
    } else
    {
        createName = "CreateAudioInstance";
        freeName = "FreeAudioInstance";
    }

    if(!GetDriverName || !GetDLLFunction2(dll.get(), createName) || !GetDLLFunction2(dll.get(), freeName))
    {
        nameOrError = _("Missing required API function");
        return false;
    }

    nameOrError = GetDriverName();
    return true;
}

std::vector<DriverWrapper::DriverItem> DriverWrapper::LoadDriverList(const DriverType dt)
{
    std::vector<DriverItem> driver_list;

    const std::string DIRECTORY[2] = {"/video", "/audio"};

    std::string path = RTTRCONFIG.ExpandPath(FILE_PATHS[46]) + DIRECTORY[dt];
    std::string extension =
#ifdef _WIN32
      "dll";
#else
#ifdef __APPLE__
      "dylib";
#else
      "so";
#endif // !__APPLE__
#endif // !_WIN32

    LOG.write(_("Searching for drivers in %s\n")) % path;
    std::vector<std::string> driver_files = ListDir(path, extension, false);

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
        std::string nameOrError;
        if(!CheckLibrary(path, dt, nameOrError))
            LOG.write(_("Skipping %s: %s\n")) % path % nameOrError;
        else
            driver_list.push_back(DriverItem(path, nameOrError));
    }
    return driver_list;
}
