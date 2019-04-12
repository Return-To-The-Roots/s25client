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
#include "helpers/containerUtils.h"
#include "mygettext/mygettext.h"
#include "libutil/Log.h"
#include "libutil/error.h"
#include <boost/dll/shared_library.hpp>
#include <memory>

namespace {
/// Definition des GetDriverAPIVersion-Zeigers
using GetDriverAPIVersion_t = decltype(GetDriverAPIVersion);
/// Definition des GetDriverName
using GetDriverName_t = decltype(GetDriverName);

std::string getName(drivers::DriverType type)
{
    return (type == drivers::DriverType::Audio) ? "audio" : "video";
}
auto tryLoadLibrary(const bfs::path& filepath)
{
    boost::system::error_code ec;
    auto result = std::make_unique<boost::dll::shared_library>(filepath, ec, boost::dll::load_mode::rtld_lazy);
    if(ec)
        result.reset();
    return result;
}
} // namespace

namespace drivers {
DriverWrapper::DriverWrapper() = default;
DriverWrapper::~DriverWrapper() = default;

void DriverWrapper::Unload()
{
    dll.reset();
}

bool DriverWrapper::Load(const DriverType dt, std::string& preference)
{
    // ggf. aufräumen vorher
    Unload();

    /// Verfügbare Treiber auflisten
    std::vector<DriverItem> drivers = LoadDriverList(dt);

    LOG.write("%u %s drivers found!\n") % drivers.size() % getName(dt);

    // Welche gefunden?
    if(drivers.empty())
        return false;

    /// Suche, ob der Treiber dabei ist, den wir wünschen
    const auto it = helpers::findPred(drivers, [preference](const auto& it) { return it.GetName() == preference; });
    if(it != drivers.end())
    {
        // Dann den gleich nehmen
        dll = tryLoadLibrary(it->GetFile());
    }

    // ersten Treiber laden
    if(!dll)
    {
        dll = tryLoadLibrary(drivers.front().GetFile());
        // Standardwert zuweisen
        preference = drivers.front().GetName();
    }

    if(!dll)
    {
        s25util::fatal_error("Could not load driver library\n");
        return false;
    }

    return true;
}

void* DriverWrapper::GetDLLFunction(const std::string& name)
{
    return !dll ? nullptr : &dll->get<void*>(name);
}

bool DriverWrapper::CheckLibrary(const bfs::path& path, DriverType dt, std::string& nameOrError)
{
    auto dll = tryLoadLibrary(path);

    if(!dll)
    {
        nameOrError = _("Failed to load library. Check dependencies!");
        return false;
    }

    auto GetDriverAPIVersion = dll->get<GetDriverAPIVersion_t>("GetDriverAPIVersion");
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

    auto GetDriverName = dll->get<GetDriverName_t>("GetDriverName");
    std::string createName, freeName;
    if(dt == DriverType::Video)
    {
        createName = "CreateVideoInstance";
        freeName = "FreeVideoInstance";
    } else
    {
        createName = "CreateAudioInstance";
        freeName = "FreeAudioInstance";
    }

    if(!GetDriverName || !dll->has(createName) || !dll->has(freeName))
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

    std::string path = RTTRCONFIG.ExpandPath(FILE_PATHS[46]) + "/" + getName(dt);
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

    for(auto path : driver_files)
    {
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
} // namespace drivers
