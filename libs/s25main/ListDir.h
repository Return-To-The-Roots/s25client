// Copyright (c) 2005 - 2020 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <boost/filesystem/path.hpp>
#include <string>
#include <vector>

/// List all files in the given path with a given extension
/// @extension Extension that files/folders must have (dot is added automatically to front) or empty to list all
/// @includeDirectories tells whether directories should be added too
std::vector<boost::filesystem::path> ListDir(const boost::filesystem::path& path, std::string extension,
                                             bool includeDirectories = false);
