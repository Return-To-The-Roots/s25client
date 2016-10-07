#include "defines.h" // IWYU pragma: keep
#include "ProgramInitHelpers.h"
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

bool InitWorkingDirectory(const std::string& exeFilepath)
{
    bfs::path fullExeFilepath = exeFilepath;
#ifdef _WIN32
    // For windows we may be run in a path with special chars. So get the wide-char version of the filepath
    int nArgs;
    wchar_t** argList = CommandLineToArgvW(GetCommandLineW(), &nArgs);
    if(!argList || nArgs < 1)
    {
        std::cerr << "Could not get command line!" << std::endl;
        return false;
    }
    fullExeFilepath = argList[0];
    LocalFree(argList);
#endif // _WIN32
    fullExeFilepath = bfs::absolute(fullExeFilepath);
    if(!bfs::exists(fullExeFilepath))
    {
        std::cerr << "Executable not at '" << fullExeFilepath << "'" << std::endl;
        return false;
    }

    // Determine install prefix
    bfs::path prefixPath;
    // Allow overwrite with RTTR_PREFIXDIR
    const char* rttrPrefixDir = getenv("RTTR_PREFIX_DIR");
    if(rttrPrefixDir)
    {
        prefixPath = rttrPrefixDir;
        std::cout << "Note: Prefix path manually set to " << prefixPath << std::endl;
    } else
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
            std::cerr << "Could not find install prefix." << std::endl
                << "Current binary dir: " << curBinDir << std::endl
                << "Best guess for prefixed binary dir: " << (prefixPath / cfgBinDir) << std::endl
                << "Configured binary dir: " << cfgBinDir << std::endl;
            return false;
        }
    }

    // Make the prefix path our working directory as all other paths are relative to that
    bfs::current_path(prefixPath);
    return true;
}
