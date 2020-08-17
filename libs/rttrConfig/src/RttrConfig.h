// Copyright (c) 2016 - 2017 Settlers Freaks (sf-team at siedler25.org)
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

#pragma once

#ifndef RttrConfig_h__
#define RttrConfig_h__

#include "s25util/Singleton.h"
#include <boost/filesystem/path.hpp>
#include <map>
#include <string>

class RttrConfig : public Singleton<RttrConfig>
{
    boost::filesystem::path prefixPath_, homePath;
    std::map<std::string, std::string> pathMappings;

public:
    bool Init();
    /// Return the prefix path for the installation
    static boost::filesystem::path GetPrefixPath();
    /// Return the path from which RTTR was compiled
    static boost::filesystem::path GetSourceDir();
    /// Expand the given path to a valid, absolute path replacing placeholders like <RTTR_BINDIR>/foo.bar
    boost::filesystem::path ExpandPath(const std::string& path) const;
};

#define RTTRCONFIG RttrConfig::inst()

#endif // RttrConfig_h__
