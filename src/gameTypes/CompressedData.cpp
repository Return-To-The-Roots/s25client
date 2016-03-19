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

#include "defines.h" // IWYU pragma: keep
#include "CompressedData.h"
#include "FileChecksum.h"
#include "libutil/src/Log.h"
#include <bzlib.h>
#include <boost/smart_ptr/scoped_array.hpp>
#include <fstream>
#include <cerrno>
#include <cmath>
#include <cstring>

// Include last!
#include "DebugNew.h" // IWYU pragma: keep

bool CompressedData::DecompressToFile(const std::string& filePath, unsigned* checksum)
{
    std::ofstream file(filePath.c_str(), std::ios::binary);

    if(!file)
    {
        LOG.lprintf("FATAL ERROR: can't write to %s: %s\n", filePath.c_str(), strerror(errno));
        return false;
    }

    boost::scoped_array<char> uncompressedData(new char[length]);

    unsigned int outLength = length;

    int err = BZ2_bzBuffToBuffDecompress(uncompressedData.get(), &outLength, &data[0], data.size(), 0, 0);
    if(err != BZ_OK)
    {
        LOG.lprintf("FATAL ERROR: BZ2_bzBuffToBuffDecompress failed with code %d\n", err);
        return false;
    }

    if(outLength != length)
    {
        LOG.lprintf("FATAL ERROR: Length mismatch after decompressing. Expected: %u, got %u\n", length, outLength);
        return false;
    }

    if(!file.write(uncompressedData.get(), length))
    {
        LOG.lprintf("FATAL ERROR: Writing to %s failed\n", filePath.c_str());
        return false;
    }

    if(checksum)
        *checksum = CalcChecksumOfBuffer(uncompressedData.get(), length);

    return true;
}

bool CompressedData::CompressFromFile(const std::string& filePath, unsigned* checksum /* = NULL */)
{
    std::ifstream file(filePath.c_str(), std::ios::binary | std::ios::ate);
    length = static_cast<unsigned>(file.tellg());
    data.resize(static_cast<int>(std::ceil(length * 1.1)) + 600); // Buffer should be at most 1% bigger + 600 Bytes according to docu
    file.seekg(0);

    boost::scoped_array<char> uncompressedData(new char[length]);

    if(!file.read(uncompressedData.get(), length))
    {
        LOG.lprintf("Could not read from %s\n", filePath.c_str());
        return false;
    }

    unsigned compressedLen = data.size();
    int err = BZ2_bzBuffToBuffCompress(&data[0], &compressedLen, uncompressedData.get(), length, 9, 0, 250);
    if(err != BZ_OK)
    {
        LOG.lprintf("FATAL ERROR: BZ2_bzBuffToBuffCompress failed with error: %d\n", err);
        return false;
    }
    data.resize(compressedLen);

    if(checksum)
        *checksum = CalcChecksumOfBuffer(uncompressedData.get(), length);
    return true;
}
