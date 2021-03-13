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

#pragma once

#include <boost/filesystem/path.hpp>
#include <string>
#include <vector>

/// Holds compressed data
struct CompressedData
{
    CompressedData(const unsigned uncompressedLength = 0) : uncompressedLength(uncompressedLength) {}
    void Clear()
    {
        uncompressedLength = 0;
        data.clear();
    }
    bool DecompressToFile(const boost::filesystem::path& filePath, unsigned* checksum = nullptr) const;
    bool CompressFromFile(const boost::filesystem::path& filePath, unsigned* checksum = nullptr);

    /// Uncompressed length
    unsigned uncompressedLength;
    /// Actual data
    std::vector<char> data;

    static std::vector<char> compress(const std::vector<char>& data);
    static std::vector<char> decompress(const std::vector<char>& data, size_t uncompressedSize);
};
