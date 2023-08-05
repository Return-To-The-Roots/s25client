// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <boost/filesystem.hpp>

namespace rttr::test {
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
        const boost::filesystem::path& get() const { return folder; }
        operator const boost::filesystem::path &() const { return folder; }
    };
} // namespace rttr::test
