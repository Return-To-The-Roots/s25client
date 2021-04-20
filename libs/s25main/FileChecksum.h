// Copyright (c) 2005 - 2017 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <boost/filesystem/path.hpp>

uint32_t CalcChecksumOfFile(const boost::filesystem::path& path);
uint32_t CalcChecksumOfBuffer(const uint8_t* buffer, size_t size);

inline uint32_t CalcChecksumOfBuffer(const char* buffer, size_t size)
{
    return CalcChecksumOfBuffer(reinterpret_cast<const uint8_t*>(buffer), size);
}

template<typename T>
inline uint32_t CalcChecksumOfBuffer(const T& buffer)
{
    return CalcChecksumOfBuffer(buffer.data(), buffer.size());
}
