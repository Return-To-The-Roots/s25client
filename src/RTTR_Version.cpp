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

const char* RTTR_Version::GetTitle()
{
    static char title[256];
    std::memset(title, 0, 256);
    std::strncpy(title, WINDOW_TITLE, 256);
    return title;
}

const char* RTTR_Version::GetVersion()
{
    static char version[256];
    std::memset(version, 0, 256);
    std::strncpy(version, WINDOW_VERSION, 256);
    return version;
}

const char* RTTR_Version::GetRevision()
{
    static char revision[256];
    std::memset(revision, 0, 256);
    std::strncpy(revision, WINDOW_REVISION, 256);
    return revision;
}

const char* RTTR_Version::GetShortRevision()
{
    static char revision[8];
    std::memset(revision, 0, 8);
    std::strncpy(revision, WINDOW_REVISION, 7);
    return revision;
}

const char* RTTR_Version::GetYear()
{
    // nasty but works, if versioning principle changes, we should make it use date function
    static char year[5];
    std::memset(year, 0, 5);
    std::strncpy(year, WINDOW_VERSION, 4);
    return year;
}
