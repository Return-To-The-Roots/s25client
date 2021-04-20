// Copyright (c) 2005 - 2017 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

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
