// Copyright (c) 2005 - 2017 Settlers Freaks (sf-team at siedler25.org)
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

#include "rttrDefines.h" // IWYU pragma: keep
#include "ListDir.h"
#include <boost/filesystem.hpp>
#include <algorithm>

///////////////////////////////////////////////////////////////////////////////
// lists the files of a directory
std::vector<std::string> ListDir(const std::string& path, std::string extension, bool includeDirectories,
                                 const std::vector<std::string>* const appendTo)
{
    std::vector<std::string> result;
    if(appendTo)
        result = *appendTo;

    bfs::path fullPath(path);

    if(!bfs::exists(fullPath))
        return result;

    if(!extension.empty())
    {
        std::transform(extension.begin(), extension.end(), extension.begin(), tolower);
        // Add dot if missing
        if(extension[0] != '.')
            extension = "." + extension;
    }

    for(bfs::directory_iterator it = bfs::directory_iterator(fullPath); it != bfs::directory_iterator(); ++it)
    {
        if(bfs::is_directory(it->status()) && !includeDirectories)
            continue;

        bfs::path curPath = it->path();
        curPath.make_preferred();

        if(!extension.empty())
        {
            std::string curExt = curPath.extension().string();
            std::transform(curExt.begin(), curExt.end(), curExt.begin(), tolower);
            if(curExt != extension)
                continue;
        }
        result.push_back(curPath.string());
    }

    std::sort(result.begin(), result.end());
    return result;
}
