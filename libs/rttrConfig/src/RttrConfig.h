// Copyright (c) 2005 - 2019 Settlers Freaks (sf-team at siedler25.org)
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
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once
#ifndef libs_rttrConfig_src_RttrConfig_h
#define libs_rttrConfig_src_RttrConfig_h

#include <libutil/Singleton.h>

#include <boost/filesystem/path.hpp>

#include <map>
#include <string>

class RttrConfig : public Singleton<RttrConfig>
{
public:
    bool Init();
    /// Return the prefix path for the installation
    static boost::filesystem::path GetPrefixPath();
    /// Return the path from which RTTR was compiled
    static boost::filesystem::path GetSourceDir();
    /// Expand the given path to a valid, absolute path replacing placeholders like <RTTR_BINDIR>/foo.bar
    std::string ExpandPath(const std::string& path) const;

private:
    boost::filesystem::path prefixPath_;
    boost::filesystem::path homePath_;
    std::map<std::string, std::string> pathMappings_;
};

#define RTTRCONFIG RttrConfig::inst()

#endif // !libs_rttrConfig_src_RttrConfig_h
