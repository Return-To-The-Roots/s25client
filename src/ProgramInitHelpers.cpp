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
#include "libutil/src/System.h"
#include "libutil/src/Log.h"
#include <build_paths.h>

#include <boost/locale.hpp>
#include <boost/filesystem.hpp>
#include <iostream>
#include <cstdlib>
#include <stdexcept>
#ifdef _WIN32
#   include <windows.h>
#   include <shellapi.h>
#endif // _WIN32


bool InitLocale()
{
    // Check and set locale (avoids errors caused by invalid locales later like #420)
    try{
        // Check for errors and use classic locale to avoid e.g. thousand separator in int2string conversions via streams
#ifdef _WIN32
        // On windows we want to enforce the encoding (mostly UTF8). Also using "" would use the default which uses "wrong" separators
        std::locale::global(boost::locale::generator().generate("C"));
#else
        // In linux / OSX this suffices
        std::locale::global(std::locale::classic());
#endif // _WIN32
        // Use also the encoding (mostly UTF8) for bfs paths: http://stackoverflow.com/questions/23393870
        bfs::path::imbue(std::locale());
    } catch(std::exception& e){
        std::cerr << "Error initializing your locale setting. ";
#ifdef _WIN32
        std::cerr << "Check your system language configuration!";
#else
        char* lcAll = getenv("LC_ALL");
        char* lang = getenv("LANG");
        std::cerr << "Check your environment for invalid settings (e.g. LC_ALL";
        if(lcAll)
            std::cerr << "=" << lcAll;
        std::cerr << " or LANG";
        if(lang)
            std::cerr << "=" << lang;
        std::cerr << ")";
#endif
        std::cerr << std::endl;
        std::cerr << e.what() << std::endl;
        return false;
    }
    return true;
}

bfs::path GetPrefixPath(const std::string& argv0)
{
    // Determine install prefix
    // Allow overwrite with RTTR_PREFIX_DIR
    bfs::path prefixPath = System::getPathFromEnvVar("RTTR_PREFIX_DIR");
    if(!prefixPath.empty())
    {
        LOG.write("Note: Prefix path manually set to %1%\n", LogTarget::Stdout) % prefixPath;
    }

    // Complete the path as it would be done by the system
    // This avoids problems if the program was not started from the working directory
    // e.g. by putting its path in PATH
    bfs::path fullExeFilepath = System::getExecutablePath(argv0);
    if(!bfs::exists(fullExeFilepath) && !prefixPath.empty())
    {
        fullExeFilepath = prefixPath / RTTR_BINDIR / bfs::path(argv0).filename();
    }
    if(!bfs::exists(fullExeFilepath))
    {
        LOG.write("Executable not at '%1%'\nStarting file path: %2%\nCompleted file path: %3%\n", LogTarget::Stderr)
            % fullExeFilepath % argv0 % System::getExecutablePath(argv0);
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
                "Configured binary dir: %3%\n", LogTarget::Stderr)
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
