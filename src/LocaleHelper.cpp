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

#include "defines.h" // IWYU pragma: keep
#include "LocaleHelper.h"
#include "libutil/src/System.h"
#include <boost/filesystem/path.hpp>
#include <boost/locale.hpp>
#include <iostream>
#include <stdexcept>

std::locale LocaleHelper::bfsDefaultLocale;

bool LocaleHelper::init()
{
    // Check and set locale (avoids errors caused by invalid locales later like #420)
    try
    {
// Check for errors and use classic locale to avoid e.g. thousand separator in int2string conversions via streams
#ifdef _WIN32
        // On windows we want to enforce the encoding (mostly UTF8). Also using "" would use the default which uses "wrong" separators
        std::locale newLocale(boost::locale::generator().generate("C"));
#else
        // In linux / OSX this suffices
        std::locale newLocale(std::locale::classic());
#endif // _WIN32
        std::locale::global(newLocale);
        // Use also the encoding (mostly UTF8) for bfs paths: http://stackoverflow.com/questions/23393870
        bfsDefaultLocale = bfs::path::imbue(std::locale());
    } catch(std::exception& e)
    {
        std::cerr << "Error initializing your locale setting. ";
#ifdef _WIN32
        std::cerr << "Check your system language configuration!";
#else
        std::string lcAll = System::getEnvVar("LC_ALL");
        std::string lang = System::getEnvVar("LANG");
        std::cerr << "Check your environment for invalid settings (e.g. LC_ALL";
        if(!lcAll.empty())
            std::cerr << "=" << lcAll;
        std::cerr << " or LANG";
        if(!lang.empty())
            std::cerr << "=" << lang;
        std::cerr << ")";
#endif
        std::cerr << std::endl;
        std::cerr << e.what() << std::endl;
        return false;
    }
    return true;
}
