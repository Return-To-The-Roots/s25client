// Copyright (c) 2005 - 2015 Settlers Freaks (sf-team at siedler25.org)
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

///////////////////////////////////////////////////////////////////////////////
// Header
#include "defines.h" // IWYU pragma: keep
#include "ListDir.h"
#include <boost/filesystem.hpp>
#include <algorithm>

// Include last!
#include "DebugNew.h" // IWYU pragma: keep


///////////////////////////////////////////////////////////////////////////////
// lists the files of a directory
void ListDir(const std::string& path, bool directories, void (*CallBack)(const std::string& filename, void* param), void* param, std::list<std::string> *liste)
{
    namespace bfs = boost::filesystem;
    bfs::path fullPath(path);

    if(!fullPath.has_extension())
        return;

    bfs::path parentPath = fullPath.parent_path();
    std::string extension = fullPath.extension().string();
    std::transform(extension.begin(), extension.end(), extension.begin(), tolower);
    if(!bfs::exists(parentPath))
        return;

    for(bfs::directory_iterator it = bfs::directory_iterator(parentPath); it != bfs::directory_iterator(); ++it)
    {
        if(bfs::is_directory(it->status()) && !directories)
            continue;

        bfs::path curPath = it->path();
        curPath.make_preferred();

        std::string curExt = curPath.extension().string();
        std::transform(curExt.begin(), curExt.end(), curExt.begin(), tolower);
        if(curExt != extension)
            continue;

        if(CallBack)
            CallBack(curPath.string(), param);
        if(liste)
            liste->push_back(curPath.string());
    }

    if(liste)
        liste->sort();
}
