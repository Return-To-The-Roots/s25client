// Copyright (c) 2005 - 2020 Settlers Freaks (sf-team at siedler25.org)
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
