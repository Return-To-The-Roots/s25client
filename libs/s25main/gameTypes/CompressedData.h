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

#ifndef CompressedData_h__
#define CompressedData_h__

#include <boost/filesystem/path.hpp>
#include <string>
#include <vector>

/// Holds compressed data
struct CompressedData
{
    CompressedData() : length(0) {}
    void Clear()
    {
        length = 0;
        data.clear();
    }
    bool DecompressToFile(const boost::filesystem::path& filePath, unsigned* checksum = nullptr);
    bool CompressFromFile(const boost::filesystem::path& filePath, unsigned* checksum = nullptr);

    /// Uncompressed length
    unsigned length;
    /// Actual data
    std::vector<char> data;
};

#endif // CompressedData_h__
