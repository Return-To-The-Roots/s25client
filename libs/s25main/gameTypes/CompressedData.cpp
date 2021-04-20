// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "CompressedData.h"
#include "FileChecksum.h"
#include "helpers/format.hpp"
#include "s25util/Log.h"
#include <boost/nowide/fstream.hpp>
#include <bzlib.h>
#include <cmath>
#include <stdexcept>

bool CompressedData::DecompressToFile(const boost::filesystem::path& filePath, unsigned* checksum) const
{
    boost::nowide::ofstream file(filePath, std::ios::binary);

    if(!file)
    {
        LOG.write("FATAL ERROR: can't write to %1%\n") % filePath;
        return false;
    }

    try
    {
        const auto uncompressedData = decompress(data, uncompressedLength);

        if(!file.write(uncompressedData.data(), uncompressedData.size()))
        {
            LOG.write("FATAL ERROR: Writing to %1% failed\n") % filePath;
            return false;
        }

        if(checksum)
            *checksum = CalcChecksumOfBuffer(uncompressedData);
    } catch(const std::runtime_error& err)
    {
        LOG.write("FATAL ERROR: %1%\n") % err.what();
        return false;
    }

    return true;
}

bool CompressedData::CompressFromFile(const boost::filesystem::path& filePath, unsigned* checksum /* = nullptr */)
{
    boost::nowide::ifstream file(filePath, std::ios::binary | std::ios::ate);
    uncompressedLength = static_cast<unsigned>(file.tellg());
    file.seekg(0);

    std::vector<char> uncompressedData(uncompressedLength);
    if(!file.read(uncompressedData.data(), uncompressedLength))
    {
        LOG.write("Could not read from %1%\n") % filePath;
        return false;
    }

    try
    {
        data = compress(uncompressedData);
    } catch(const std::runtime_error& err)
    {
        LOG.write("FATAL ERROR: %1%\n") % err.what();
        return false;
    }

    if(checksum)
        *checksum = CalcChecksumOfBuffer(uncompressedData);
    return true;
}

std::vector<char> CompressedData::compress(const std::vector<char>& data)
{
    // Buffer should be at most 1% bigger + 600 Bytes according to docu
    auto compressedLen = static_cast<unsigned>(std::ceil(data.size() * 1.01)) + 600u;
    std::vector<char> compressedData(compressedLen);

    const int err = BZ2_bzBuffToBuffCompress(compressedData.data(), &compressedLen, const_cast<char*>(data.data()),
                                             data.size(), 9, 0, 250);
    if(err != BZ_OK)
        throw std::runtime_error(helpers::format("BZ2_bzBuffToBuffCompress failed with error: %1%", err));
    compressedData.resize(compressedLen);
    return compressedData;
}

std::vector<char> CompressedData::decompress(const std::vector<char>& data, size_t const uncompressedSize)
{
    std::vector<char> uncompressedData(uncompressedSize);

    unsigned outLength = uncompressedSize;

    const int err = BZ2_bzBuffToBuffDecompress(uncompressedData.data(), &outLength, const_cast<char*>(data.data()),
                                               data.size(), 0, 0);
    if(err != BZ_OK)
        throw std::runtime_error(helpers::format("BZ2_bzBuffToBuffDecompress failed with error: %1%", err));

    if(outLength != uncompressedSize)
        throw std::runtime_error(
          helpers::format("Length mismatch after decompressing. Expected: %1%, got %2%", uncompressedSize, outLength));

    return uncompressedData;
}
