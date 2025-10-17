// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "RttrConfig.h"
#include "RTTR_Assert.h"
#include "s25util/Log.h"
#include "s25util/System.h"
#include <boost/filesystem.hpp>
#include <boost/filesystem/path.hpp>
#include <build_paths.h>
#include <stdexcept>

namespace bfs = boost::filesystem;

// Expected: RTTR_BINDIR, RTTR_DATADIR, RTTR_GAMEDIR, RTTR_LIBDIR, RTTR_DRIVERDIR
#if !(defined(RTTR_BINDIR) && defined(RTTR_DATADIR) && defined(RTTR_GAMEDIR) && defined(RTTR_LIBDIR) \
      && defined(RTTR_DRIVERDIR))
#    error "At least one of the RTTR_*DIR is undefined!"
#endif

// Folder for user data, formerly "SETTINGSDIR" or "CONFIG"
#ifdef RTTR_SETTINGSDIR
#    define RTTR_USERDATADIR RTTR_SETTINGSDIR
#elif !defined(RTTR_USERDATADIR)
#    if defined(_WIN32)
#        define RTTR_USERDATADIR "~/Return To The Roots"
#    elif defined(__APPLE__)
#        define RTTR_USERDATADIR "~/Library/Application Support/Return To The Roots"
#    else
#        define RTTR_USERDATADIR "~/.s25rttr"
#    endif
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
    bfs::path prefixPath = getEnvOverride("PREFIX", RTTR_INSTALL_PREFIX);
    if(!rttrBinDir.is_absolute())
    {
        // Go up one level for each entry (folder) in rttrBinDir
        prefixPath = fullExeFilepath.parent_path();
        for(const auto& part : rttrBinDir)
        {
            if(part == ".")
                continue;
            prefixPath = prefixPath.parent_path();
        }
    }

    if(!prefixPath.empty())
    {
        bfs::path exePath =
          (rttrBinDir.is_absolute() ? rttrBinDir : prefixPath / rttrBinDir) / fullExeFilepath.filename();
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

boost::filesystem::path RttrConfig::ExpandPath(const std::string& path) const
{
    if(path.empty())
        return prefixPath_;
    bfs::path outPath;
    if(path[0] == '<')
    {
        static const char rttrPathId[] = "<RTTR_";
        size_t startPos = path.find(rttrPathId);
        if(startPos > 0u)
            throw std::runtime_error("<RTTR_X> placeholders only allowed at start of path");
        size_t endPos = path.find('>');
        if(endPos == std::string::npos)
            throw std::runtime_error("Incomplete <RTTR_X> placeholder found!");
        std::string entry = path.substr(sizeof(rttrPathId) - 1, endPos - startPos - sizeof(rttrPathId) + 1);
        auto it = pathMappings.find(entry);
        if(it == pathMappings.end())
            throw std::runtime_error("Invalid <RTTR_X> placeholder found!");
        outPath = bfs::path(it->second) / path.substr(endPos + 1);
    } else
        outPath = path;
    if(*outPath.begin() == "~")
        outPath = homePath / outPath.string().substr(2);

    outPath = bfs::absolute(outPath, prefixPath_).lexically_normal();
    return outPath.make_preferred();
}

void RttrConfig::overridePathMapping(const std::string& id, const boost::filesystem::path& path)
{
    RTTR_Assert(pathMappings.count(id) > 0);
    pathMappings[id] = path;
}

bool RttrConfig::Init()
{
    prefixPath_ = GetPrefixPath();
    if(prefixPath_.empty())
        return false;
    // Make the prefix path our working directory as all other paths are relative to that
    bfs::current_path(prefixPath_);
    homePath = System::getHomePath();
    pathMappings.clear();
    pathMappings["BIN"] = getEnvOverride("BIN", RTTR_BINDIR);
    pathMappings["EXTRA_BIN"] = getEnvOverride("EXTRA_BIN", RTTR_EXTRA_BINDIR);
    pathMappings["DATA"] = getEnvOverride("DATA", RTTR_DATADIR);
    pathMappings["GAME"] = getEnvOverride("GAME", RTTR_GAMEDIR);
    pathMappings["LIB"] = getEnvOverride("LIB", RTTR_LIBDIR);
    pathMappings["DRIVER"] = getEnvOverride("DRIVER", RTTR_DRIVERDIR);
    pathMappings["RTTR"] = getEnvOverride("RTTR", RTTR_DATADIR "/RTTR");
    pathMappings["USERDATA"] = getEnvOverride("USERDATA", RTTR_USERDATADIR);
    return true;
}

bfs::path RttrConfig::getEnvOverride(const std::string& id, const bfs::path& defaultPath) {
    bfs::path path = System::getPathFromEnvVar("RTTR_" + id + "_DIR");
    if(!path.empty())
    {
        LOG.write("Note: %1% path manually set to %2%\n", LogTarget::Stdout) % id % path;
    } else
        return defaultPath;

    return path;
}
