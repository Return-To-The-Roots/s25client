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

#include "CompressedData.h"
#include "FileChecksum.h"
#include "s25util/Log.h"
#include <boost/nowide/fstream.hpp>
#include <bzlib.h>
#include <cmath>
#include <memory>

bool CompressedData::DecompressToFile(const boost::filesystem::path& filePath, unsigned* checksum)
{
    boost::nowide::ofstream file(filePath, std::ios::binary);

    if(!file)
    {
        LOG.write("FATAL ERROR: can't write to %s\n") % filePath;
        return false;
    }

    auto uncompressedData = std::unique_ptr<char[]>(new char[length]);

    unsigned outLength = length;

    int err = BZ2_bzBuffToBuffDecompress(uncompressedData.get(), &outLength, &data[0], data.size(), 0, 0);
    if(err != BZ_OK)
    {
        LOG.write("FATAL ERROR: BZ2_bzBuffToBuffDecompress failed with code %d\n") % err;
        return false;
    }

    if(outLength != length)
    {
        LOG.write("FATAL ERROR: Length mismatch after decompressing. Expected: %u, got %u\n") % length % outLength;
        return false;
    }

    if(!file.write(uncompressedData.get(), length))
    {
        LOG.write("FATAL ERROR: Writing to %s failed\n") % filePath;
        return false;
    }

    if(checksum)
        *checksum = CalcChecksumOfBuffer(uncompressedData.get(), length);

    return true;
}

bool CompressedData::CompressFromFile(const boost::filesystem::path& filePath, unsigned* checksum /* = nullptr */)
{
    boost::nowide::ifstream file(filePath, std::ios::binary | std::ios::ate);
    length = static_cast<unsigned>(file.tellg());
    data.resize(static_cast<int>(std::ceil(length * 1.1)) + 600); // Buffer should be at most 1% bigger + 600 Bytes according to docu
    file.seekg(0);

    auto uncompressedData = std::unique_ptr<char[]>(new char[length]);

    if(!file.read(uncompressedData.get(), length))
    {
        LOG.write("Could not read from %s\n") % filePath;
        return false;
    }

    unsigned compressedLen = data.size();
    int err = BZ2_bzBuffToBuffCompress(&data[0], &compressedLen, uncompressedData.get(), length, 9, 0, 250);
    if(err != BZ_OK)
    {
        LOG.write("FATAL ERROR: BZ2_bzBuffToBuffCompress failed with error: %d\n") % err;
        return false;
    }
    data.resize(compressedLen);

    if(checksum)
        *checksum = CalcChecksumOfBuffer(uncompressedData.get(), length);
    return true;
}
