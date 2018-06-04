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

#include "build_version_defines.h"
#include "RTTR_Version.h"
#include <cstring>

/// Copy the string from src into dst ensuring zero termination
/// Contrary to std::strncpy the destination size may be smaller than the string
template<size_t N>
void mystrncpy(char (&dst)[N], const char* src)
{
    if(N == 0u)
        return;
    size_t len = std::min(N - 1u, std::strlen(src));
    std::memcpy(dst, src, len);
    dst[len] = 0;
}

std::string RTTR_Version::GetTitle()
{
    static char title[256];
    mystrncpy(title, WINDOW_TITLE);
    return title;
}

std::string RTTR_Version::GetVersionDate()
{
    static char version[256];
    mystrncpy(version, WINDOW_VERSION);
    return version;
}

std::string RTTR_Version::GetRevision()
{
    static char revision[256];
    mystrncpy(revision, WINDOW_REVISION);
    return revision;
}

std::string RTTR_Version::GetShortRevision()
{
    static char revision[8];
    mystrncpy(revision, WINDOW_REVISION);
    return revision;
}

std::string RTTR_Version::GetYear()
{
    // nasty but works, if versioning principle changes, we should make it use date function
    static char year[5];
    mystrncpy(year, WINDOW_VERSION);
    return year;
}

std::string RTTR_Version::GetReadableVersion()
{
    return "v" + GetVersionDate() + " - " + GetShortRevision();
}
