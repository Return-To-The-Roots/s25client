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

#include "rttrDefines.h" // IWYU pragma: keep
#include "ProgramInitHelpers.h"
#include "libutil/Log.h"
#include "libutil/System.h"
#include <boost/filesystem.hpp>
#include <build_paths.h>

bfs::path GetPrefixPath()
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
    } else if(rttrBinDir.is_absolute())
        prefixPath = RTTR_INSTALL_PREFIX;
    else
    {
        // Go up one level for each entry (folder) in rttrBinDir
        prefixPath = fullExeFilepath.parent_path();
        for(bfs::path::const_iterator it = rttrBinDir.begin(); it != rttrBinDir.end(); ++it)
        {
            if(*it == ".")
                continue;
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
    return prefixPath;
}

bool InitWorkingDirectory()
{
    bfs::path prefixPath = GetPrefixPath();
    if(prefixPath.empty())
        return false;
    // Make the prefix path our working directory as all other paths are relative to that
    bfs::current_path(prefixPath);
    return true;
}
