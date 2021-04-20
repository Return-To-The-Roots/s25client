// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "ListDir.h"
#include "s25util/strAlgos.h"
#include <boost/filesystem.hpp>
#include <algorithm>

namespace bfs = boost::filesystem;

// lists the files of a directory
std::vector<bfs::path> ListDir(const bfs::path& path, std::string extension, bool includeDirectories)
{
    std::vector<bfs::path> result;

    if(!bfs::exists(path))
        return result;

    if(!extension.empty())
    {
        extension = s25util::toLower(extension);
        // Add dot if missing
        if(extension.front() != '.')
            extension.insert(extension.begin(), '.');
    }

    for(const auto& it : bfs::directory_iterator(path))
    {
        if(!includeDirectories && bfs::is_directory(it.status()))
            continue;

        if(!extension.empty())
        {
            const std::string curExt = s25util::toLower(it.path().extension().string());
            if(curExt != extension)
                continue;
        }
        bfs::path curPath = it.path();
        curPath.make_preferred();
        result.emplace_back(std::move(curPath));
    }

    std::sort(result.begin(), result.end());
    return result;
}
