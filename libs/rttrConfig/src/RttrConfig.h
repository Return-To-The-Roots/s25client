// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "s25util/Singleton.h"
#include <boost/filesystem/path.hpp>
#include <map>
#include <string>

class RttrConfig : public Singleton<RttrConfig>
{
    boost::filesystem::path prefixPath_, homePath;
    std::map<std::string, boost::filesystem::path> pathMappings;

public:
    bool Init();
    /// Return the prefix path for the installation
    static boost::filesystem::path GetPrefixPath();
    /// Return the path from which RTTR was compiled
    static boost::filesystem::path GetSourceDir();
    /// Expand the given path to a valid, absolute path replacing placeholders like <RTTR_BINDIR>/foo.bar
    boost::filesystem::path ExpandPath(const std::string& path) const;
    /// Overwrite a given path mapping
    void overridePathMapping(const std::string& id, const boost::filesystem::path& path);
};

#define RTTRCONFIG RttrConfig::inst()
