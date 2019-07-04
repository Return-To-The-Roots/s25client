// Copyright (c) 2005 - 2019 Settlers Freaks (sf-team at siedler25.org)
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
//
// SPDX-License-Identifier: GPL-2.0-or-later

//#include <commonDefines.h> // IWYU pragma: keep

#include "RttrConfig.h"

#include <build_paths.h>
#include <libutil/Log.h>
#include <libutil/System.h>

#include <boost/filesystem.hpp>

#include <stdexcept>

namespace bfs = boost::filesystem;

// Expected: RTTR_BINDIR, RTTR_DATADIR, RTTR_GAMEDIR, RTTR_LIBDIR, RTTR_DRIVERDIR
#if !defined(RTTR_BINDIR) || !defined(RTTR_DATADIR) || !defined(RTTR_GAMEDIR) || !defined(RTTR_LIBDIR) || !defined(RTTR_DRIVERDIR)
#   error "At least one of the RTTR_*DIR is undefined!"
#endif

#ifndef RTTR_SETTINGSDIR
#   ifdef _WIN32
#       define RTTR_SETTINGSDIR "~/Return To The Roots"
#   elif defined(__APPLE__)
#       define RTTR_SETTINGSDIR "~/Library/Application Support/Return To The Roots"
#   else
#       define RTTR_SETTINGSDIR "~/.s25rttr"
#   endif
#endif // !RTTR_SETTINGSDIR

bfs::path RttrConfig::GetPrefixPath()
{
    // Determine install prefix

    // Get path to current executable (at least for checks)
    bfs::path fullExeFilepath = System::getExecutablePath();
    
    // This should always work unless we have some missing implementation or a bad error
    if(fullExeFilepath.empty())
    {
        LOG.write("Could not get path to current executable\n", LogTarget::Stderr);
        return "";
    }
    
    if(!bfs::exists(fullExeFilepath) || !bfs::is_regular_file(fullExeFilepath))
    {
        LOG.write("Executable not at '%1%'\n", LogTarget::Stderr) % fullExeFilepath;
        return "";
    }

    bfs::path rttrBinDir(RTTR_BINDIR);

    // Allow overwrite with RTTR_PREFIX_DIR
    bfs::path prefixPath = System::getPathFromEnvVar("RTTR_PREFIX_DIR");
    if(!prefixPath.empty())
    {
        LOG.write("Note: Prefix path manually set to %1%\n", LogTarget::Stdout) % prefixPath;
    }
    else if(rttrBinDir.is_absolute())
    {
        prefixPath = RTTR_INSTALL_PREFIX;
    }
    else
    {
        // Go up one level for each entry (folder) in rttrBinDir
        prefixPath = fullExeFilepath.parent_path();
        for(const auto& part : rttrBinDir)
        {
            if(part == ".")
            {
                continue;
            }
            prefixPath = prefixPath.parent_path();
        }
    }

    if(!prefixPath.empty())
    {
        bfs::path exePath = (rttrBinDir.is_absolute() ? rttrBinDir : prefixPath / rttrBinDir) / fullExeFilepath.filename();
        if(!bfs::is_regular_file(exePath))
            LOG.write("Warning: Executable not found with prefix path %1%. Expected: %2%\n"
                      "This may lead to file-not-found errors. Please report this!",
                      LogTarget::Stderr)
              % prefixPath % exePath;
    }
    
    return bfs::absolute(prefixPath);
}

boost::filesystem::path RttrConfig::GetSourceDir()
{
    return RTTR_SRCDIR;
}

std::string RttrConfig::ExpandPath(const std::string& path) const
{
    if(path.empty())
    {
        return prefixPath_.string();
    }
    
    bfs::path outPath;
    
    if(path[0] == '<')
    {
        static const char rttrPathId[] = "<RTTR_";

        const size_t startPos = path.find(rttrPathId);
        if(startPos > 0u)
        {
            throw std::runtime_error("<RTTR_X> placeholders only allowed at start of path");
        }
        
        const size_t endPos = path.find('>');
        if(endPos == std::string::npos)
        {
            throw std::runtime_error("Incomplete <RTTR_X> placeholder found!");
        }
        
        const std::string entry = path.substr(sizeof(rttrPathId) - 1, endPos - startPos - sizeof(rttrPathId) + 1);
        
        const auto it = pathMappings_.find(entry);
        if(it == pathMappings_.end())
        {
            throw std::runtime_error("Invalid <RTTR_X> placeholder found!");
        }
        
        outPath = bfs::path(it->second) / path.substr(endPos + 1);
    } 
    else
    {
        outPath = path;
    }
    
    if(*outPath.begin() == "~")
    {
        outPath = homePath_ / outPath.string().substr(2);
    }

    outPath = bfs::absolute(outPath, prefixPath_).lexically_normal();
    return outPath.make_preferred().string();
}

bool RttrConfig::Init()
{
    prefixPath_ = GetPrefixPath();
    if(prefixPath_.empty())
    {
        return false;
    }
    
    // Make the prefix path our working directory as all other paths are relative to that
    bfs::current_path(prefixPath_);
    homePath_ = System::getHomePath();
    
    pathMappings_.clear();
    pathMappings_["BIN"] = RTTR_BINDIR;
    pathMappings_["EXTRA_BIN"] = RTTR_EXTRA_BINDIR;
    pathMappings_["DATA"] = RTTR_DATADIR;
    pathMappings_["GAME"] = RTTR_GAMEDIR;
    pathMappings_["LIB"] = RTTR_LIBDIR;
    pathMappings_["DRIVER"] = RTTR_DRIVERDIR;
    pathMappings_["RTTR"] = RTTR_DATADIR "/RTTR";
    pathMappings_["CONFIG"] = RTTR_SETTINGSDIR;
    pathMappings_["USERDATA"] = RTTR_SETTINGSDIR;

    return true;
}
