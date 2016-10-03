#include "defines.h" // IWYU pragma: keep
#include "ProgramInitHelpers.h"
#include <boost/locale.hpp>
#include <boost/filesystem/path.hpp>
#include <iostream>
#include <cstdlib>
#include <stdexcept>

bool InitLocale()
{
    // Check and set locale (avoids errors caused by invalid locales later like #420)
    try{
        // Check for errors and use classic locale to avoid e.g. thousand separator in int2string conversions via streams
        // Normally "std::locale::global(std::locale::classic());" would suffice, but we want to enforce the encoding (mostly UTF8)
        std::locale::global(boost::locale::generator().generate(""));
        // Use also the encoding (mostly UTF8) for bfs paths: http://stackoverflow.com/questions/23393870
        bfs::path::imbue(std::locale());
    } catch(std::exception& e){
        std::cerr << "Error initializing your locale setting. ";
#ifdef _WIN32
        std::cerr << "Check your system language configuration!";
#else
        char* lcAll = getenv("LC_ALL");
        char* lcLang = getenv("LC_LANG");
        std::cerr << "Check your environment for invalid settings (e.g. LC_ALL";
        if(lcAll)
            std::cerr << "=" << lcAll;
        std::cerr << " or LC_LANG";
        if(lcLang)
            std::cerr << "=" << lcLang;
        std::cerr << ")";
#endif
        std::cerr << std::endl;
        std::cerr << e.what() << std::endl;
        return false;
    }
    return true;
}
