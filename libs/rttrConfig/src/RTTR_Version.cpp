// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "RTTR_Version.h"
#include "build_version_defines.h"

namespace rttr::version {

std::string getTitle()
{
    return "Return To The Roots";
}

std::string getVersion()
{
    return rttr::VERSION;
}

std::string getBuildDate()
{
    return rttr::BUILD_DATE;
}

std::string getRevision()
{
    return rttr::REVISION;
}

std::string getShortRevision()
{
    return getRevision().substr(0, 7);
}

std::string getYear()
{
    return getBuildDate().substr(0, 4);
}

std::string getReadableVersion()
{
    return "v" + getVersion() + " - " + getShortRevision();
}

} // namespace rttr::version