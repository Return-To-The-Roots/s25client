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

std::string RTTR_Version::GetTitle()
{
    return "Return To The Roots";
}

std::string RTTR_Version::GetVersionDate()
{
    return WINDOW_VERSION;
}

std::string RTTR_Version::GetRevision()
{
    return WINDOW_REVISION;
}

std::string RTTR_Version::GetShortRevision()
{
    return std::string(WINDOW_REVISION).substr(0, 7);
}

std::string RTTR_Version::GetYear()
{
    // nasty but works, if versioning principle changes, we should make it use date function
    return std::string(WINDOW_VERSION).substr(0, 4);
}

std::string RTTR_Version::GetReadableVersion()
{
    return "v" + GetVersionDate() + " - " + GetShortRevision();
}
