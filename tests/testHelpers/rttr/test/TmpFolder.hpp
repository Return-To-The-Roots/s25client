// Copyright (c) 2016 - 2020 Settlers Freaks (sf-team at siedler25.org)
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

#include <boost/filesystem.hpp>

namespace rttr { namespace test {
    class TmpFolder
    {
        boost::filesystem::path folder;

    public:
        explicit TmpFolder(const boost::filesystem::path& parent = boost::filesystem::temp_directory_path(),
                           const boost::filesystem::path& pattern = "%%%%-%%%%-%%%%-%%%%")
        {
            do
            {
                folder = unique_path(parent / pattern);
            } while(exists(folder));
            create_directories(folder);
        }
        ~TmpFolder() { boost::filesystem::remove_all(folder); }
        boost::filesystem::path get() const { return folder; }
    };
}} // namespace rttr::test
