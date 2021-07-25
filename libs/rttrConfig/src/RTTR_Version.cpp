// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "RTTR_Version.h"
#include "build_version_defines.h"

namespace rttr { namespace version {

    std::string GetTitle() { return "Return To The Roots"; }

    std::string GetVersion() { return rttr::VERSION; }

    std::string GetBuildDate() { return rttr::BUILD_DATE; }

    std::string GetRevision() { return rttr::REVISION; }

    std::string GetShortRevision() { return GetRevision().substr(0, 7); }

    std::string GetYear() { return GetBuildDate().substr(0, 4); }

    std::string GetReadableVersion() { return "v" + GetVersion() + " - " + GetShortRevision(); }

}} // namespace rttr::version