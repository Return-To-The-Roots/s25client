// Copyright (c) 2005 - 2020 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "FileChecksum.h"
#include <boost/nowide/fstream.hpp>

uint32_t CalcChecksumOfFile(const boost::filesystem::path& path)
{
    boost::nowide::ifstream file(path);
    if(!file)
        return 0;

    uint32_t checksum = 0;
    for(std::istreambuf_iterator<char> it(file), e; it != e; ++it)
        checksum += static_cast<uint8_t>(*it);

    return checksum;
}

uint32_t CalcChecksumOfBuffer(const uint8_t* buffer, size_t size)
{
    if(!buffer || size == 0)
        return 0;

    uint32_t checksum = 0;
    for(unsigned i = 0; i < size; ++i)
        checksum += buffer[i];
    return checksum;
}
