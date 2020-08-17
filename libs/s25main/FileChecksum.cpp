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
