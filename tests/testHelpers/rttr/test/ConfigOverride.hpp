// Copyright (C) 2005 - 2025 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "RttrConfig.h"

namespace rttr::test {
/// Temporarily change a RTTRCONFIG path
class ConfigOverride
{
    const std::string entry;
    boost::filesystem::path oldPath;

public:
    ConfigOverride(std::string pathEntry, const boost::filesystem::path& newPath)
        : entry(std::move(pathEntry)), oldPath(RTTRCONFIG.ExpandPath("<RTTR_" + entry + ">"))
    {
        RTTRCONFIG.overridePathMapping(entry, newPath);
    }
    ~ConfigOverride() { RTTRCONFIG.overridePathMapping(entry, oldPath); }
};
} // namespace rttr::test
