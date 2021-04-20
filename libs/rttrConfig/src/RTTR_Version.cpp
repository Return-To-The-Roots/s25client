// Copyright (c) 2005 - 2017 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "RTTR_Version.h"
#include "build_version_defines.h"

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
