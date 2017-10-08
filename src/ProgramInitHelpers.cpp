// Copyright (c) 2016 - 2017 Settlers Freaks (sf-team at siedler25.org)
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

#include "defines.h" // IWYU pragma: keep
#include "ProgramInitHelpers.h"
#include "libutil/Log.h"
#include "libutil/System.h"
#include <boost/filesystem.hpp>
#include <build_paths.h>

bfs::path GetPrefixPath(const std::string& argv0)
{
    // Determine install prefix
    // Allow overwrite with RTTR_PREFIX_DIR
    bfs::path prefixPath = System::getPathFromEnvVar("RTTR_PREFIX_DIR");
    if(!prefixPath.empty())
    {
        LOG.write("Note: Prefix path manually set to %1%\n", LogTarget::Stdout) % prefixPath;
    }

    // Get path to current executable
    bfs::path fullExeFilepath = System::getExecutablePath(argv0);
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

    // Determine install prefix
    if(prefixPath.empty())
    {
        const bfs::path curBinDir = fullExeFilepath.parent_path();
        const bfs::path cfgBinDir = RTTR_BINDIR;
        // Go up one level for each entry (folder) in cfgBinDir
        prefixPath = curBinDir;
        for(bfs::path::const_iterator it = cfgBinDir.begin(); it != cfgBinDir.end(); ++it)
        {
            if(*it == ".")
                continue;
            prefixPath = prefixPath.parent_path();
        }
        if(!bfs::equivalent(curBinDir, prefixPath / cfgBinDir))
        {
            LOG.write("Could not find install prefix.\n"
                      "Current binary dir: %1%\n"
                      "Best guess for prefixed binary dir: %2%\n"
                      "Configured binary dir: %3%\n",
                      LogTarget::Stderr)
              % curBinDir % (prefixPath / cfgBinDir) % cfgBinDir;
            return "";
        }
    }
    return prefixPath;
}

bool InitWorkingDirectory(const std::string& argv0)
{
    bfs::path prefixPath = GetPrefixPath(argv0);
    if(prefixPath.empty())
        return false;
    // Make the prefix path our working directory as all other paths are relative to that
    bfs::current_path(prefixPath);
    return true;
}
